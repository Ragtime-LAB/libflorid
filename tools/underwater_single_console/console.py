"""Single-arm desktop: Tk owns widgets; command and upgrade workers own blocking calls."""
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from pathlib import Path
from queue import Queue, Empty
import math
import threading
import time
import tkinter as tk
from tkinter import ttk, filedialog, font

from firmware import image_version


class Session:
    def __init__(self, device):
        self.device = device
        self.condition = threading.Condition()
        self.active = 0
        self.closing = False

    @contextmanager
    def use(self):
        with self.condition:
            if self.closing:
                raise RuntimeError("连接正在关闭")
            self.active += 1
        try:
            yield self.device
        finally:
            with self.condition:
                self.active -= 1
                self.condition.notify_all()

    def retire(self):
        with self.condition:
            self.closing = True
        self.device.stop_sending()

    def close(self):
        self.retire()
        with self.condition:
            self.condition.wait_for(lambda: self.active == 0)
        self.device.close()


class Console:
    def __init__(self, root, sdk):
        self.root, self.sdk = root, sdk
        self.session = None
        self.generation = 0
        self.events = Queue()
        self.commands = ThreadPoolExecutor(max_workers=1, thread_name_prefix="single-ui-command")
        self.urgent = ThreadPoolExecutor(max_workers=1, thread_name_prefix="single-ui-stop")
        self.upgrades = ThreadPoolExecutor(max_workers=1, thread_name_prefix="single-ui-upgrade")
        self.uris = {}
        self.last_sequence = None
        self.last_rx = self.rate_time = time.monotonic()
        self.rx_count = self.tx_previous = self.rx_previous = 0
        self.edited = set()
        self.initialized = set()
        self.closed = False
        self.updating_sliders = False
        self._style()
        self._layout()
        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.root.after(33, self.refresh)

    def _style(self):
        s_available = set(font.families(self.root))
        s_ui_font = next((s_name for s_name in ("Noto Sans CJK SC", "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI")
                         if s_name in s_available), font.nametofont("TkDefaultFont").actual("family"))
        s_mono_font = next((s_name for s_name in ("Consolas", "DejaVu Sans Mono") if s_name in s_available), s_ui_font)
        self.ui_font = s_ui_font
        self.root.title("Underwater Single · 单臂控制台")
        self.root.geometry("1180x900")
        self.root.minsize(880, 540)
        font.nametofont("TkDefaultFont").configure(family=s_ui_font, size=10)
        font.nametofont("TkTextFont").configure(family=s_ui_font, size=10)
        style = ttk.Style(self.root)
        style.theme_use("clam")
        style.configure("TFrame", background="#f3f6f6")
        style.configure("TLabel", background="#f3f6f6", foreground="#17343d")
        style.configure("TButton", padding=(12, 7), font=(s_ui_font, 10))
        style.configure("Accent.TButton", background="#126a76", foreground="white")
        style.configure("Stop.TButton", background="#9b3434", foreground="white")
        style.configure("Title.TLabel", font=(s_ui_font, 19, "bold"))
        style.configure("Mode.TLabel", font=(s_ui_font, 12, "bold"), foreground="#126a76")
        style.configure("Treeview", rowheight=34, font=(s_mono_font, 10))
        style.configure("Treeview.Heading", font=(s_ui_font, 10, "bold"))
        style.configure("TNotebook.Tab", padding=(19, 9))
        style.map("TEntry", fieldbackground=[("invalid", "#ffecec")], foreground=[("invalid", "#a42e2e")])

    def _layout(self):
        outer = ttk.Frame(self.root, padding=20)
        outer.pack(fill="both", expand=True)
        header = ttk.Frame(outer)
        header.pack(fill="x", pady=(0, 12))
        ttk.Label(header, text="Underwater Single", style="Title.TLabel").pack(side="left")
        self.mode = ttk.Label(header, text="未连接", style="Mode.TLabel")
        self.mode.pack(side="right")
        connection = ttk.Frame(outer)
        connection.pack(fill="x")
        self.transport = tk.StringVar(value="USB Bulk")
        ttk.Combobox(connection, textvariable=self.transport, values=("USB Bulk", "UART7 / RS232"), state="readonly", width=15).pack(side="left")
        self.device_uri = tk.StringVar()
        self.device_list = ttk.Combobox(connection, textvariable=self.device_uri, width=32)
        self.device_list.pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(connection, text="搜索", command=self.discover).pack(side="left", padx=(0, 8))
        ttk.Button(connection, text="连接", style="Accent.TButton", command=self.connect).pack(side="left")
        ttk.Button(connection, text="断开", command=self.disconnect).pack(side="left", padx=(8, 0))
        self.identity = ttk.Label(outer, wraplength=1100, justify="left", text="USB Vendor Bulk 或 UART7（PE8 TX / PE7 RX）921600 8N1；连接不会使能电机。")
        self.identity.pack(anchor="w", pady=(8, 3))
        ttk.Label(outer, text="断开不主动发送 Damp；需要停机时请先点击“失能”。Control 停发超过 100 ms 后由固件退出。", foreground="#53676c").pack(anchor="w", pady=(0, 12))
        actions = ttk.Frame(outer)
        actions.pack(fill="x", pady=(0, 12))
        ttk.Button(actions, text="失能 · Damp", style="Stop.TButton", command=lambda: self.call("失能", lambda d: d.disable())).pack(side="left")
        ttk.Button(actions, text="拖动 · Drag", command=lambda: self.call("拖动", lambda d: d.drag())).pack(side="left", padx=8)
        ttk.Button(actions, text="上位机控制 · Control", style="Accent.TButton", command=self.start_control).pack(side="left")
        self.metrics = ttk.Label(actions, text="命令目标 250 Hz   状态目标 250 Hz")
        self.metrics.pack(side="right")
        self.notice = tk.StringVar(value="就绪。请选择一台设备；主臂 / 从臂角色由固件报告。")
        self.notice_label = ttk.Label(outer, textvariable=self.notice, wraplength=1080, foreground="#9b3434")
        self.notice_label.pack(fill="x", pady=(0, 10))
        body = ttk.Frame(outer)
        body.pack(fill="both", expand=True)
        self.canvas = tk.Canvas(body, background="#f3f6f6", highlightthickness=0)
        vertical = ttk.Scrollbar(body, orient="vertical", command=self.canvas.yview)
        horizontal = ttk.Scrollbar(body, orient="horizontal", command=self.canvas.xview)
        vertical.pack(side="right", fill="y")
        horizontal.pack(side="bottom", fill="x")
        self.canvas.pack(side="left", fill="both", expand=True)
        self.canvas.configure(yscrollcommand=vertical.set, xscrollcommand=horizontal.set)
        outer = ttk.Frame(self.canvas)
        window = self.canvas.create_window((0, 0), window=outer, anchor="nw")
        def resize(event=None):
            width = max(860, self.canvas.winfo_width())
            self.canvas.itemconfigure(window, width=width, height=max(outer.winfo_reqheight(), self.canvas.winfo_height()))
            self.canvas.configure(scrollregion=self.canvas.bbox("all"))
            self.identity.configure(wraplength=max(400, self.root.winfo_width()-50))
            self.notice_label.configure(wraplength=max(400, self.root.winfo_width()-50))
        self.canvas.bind("<Configure>", resize)
        outer.bind("<Configure>", resize)
        self.root.bind_all("<MouseWheel>", lambda event: self.canvas.yview_scroll(-1 if event.delta>0 else 1, "units"))
        self.root.bind_all("<Button-4>", lambda event: self.canvas.yview_scroll(-1, "units"))
        self.root.bind_all("<Button-5>", lambda event: self.canvas.yview_scroll(1, "units"))
        tabs = ttk.Notebook(outer)
        tabs.pack(fill="both", expand=True)
        self.tabs = tabs
        status = ttk.Frame(tabs, padding=16)
        control = ttk.Frame(tabs, padding=16)
        settings = ttk.Frame(tabs, padding=16)
        upgrade = ttk.Frame(tabs, padding=16)
        for widget, label in [(status, "电机状态"), (control, "手动控制"), (settings, "设备设置"), (upgrade, "固件升级")]:
            tabs.add(widget, text=label)
        columns = ("motor", "q", "dq", "tau", "temp", "state", "age")
        self.table = ttk.Treeview(status, columns=columns, show="headings", height=4)
        names = ("执行器 / CAN ID", "位置 rad", "速度 rad/s", "力矩 Nm", "温度 °C", "原始状态", "反馈年龄 ms")
        for key, name in zip(columns, names):
            self.table.heading(key, text=name)
            self.table.column(key, width=160 if key=="motor" else 125 if key=="age" else 100, minwidth=150 if key=="motor" else 90, anchor="center", stretch=True)
        self.table.tag_configure("stale", foreground="#8e6060")
        self.table.pack(fill="x")
        for i in range(4):
            self.table.insert("", "end", iid=str(i), values=(self.motor_name(i), "—", "—", "—", "—", "—", "—"))
        self.masks = ttk.Label(status, text="available —    fresh —    enabled —    applied —")
        self.masks.pack(anchor="w", pady=(16, 8))
        self.reasons = ttk.Label(status, text="状态未到达", wraplength=1020)
        self.reasons.pack(anchor="w")
        ttk.Label(status, text="反馈为电机坐标；applied 仅表示 CAN 提交成功，不代表执行确认。", foreground="#53676c").pack(anchor="w", pady=(16, 0))
        self.values = [[tk.StringVar(value="0") for _ in range(5)] for _ in range(4)]
        self.scales = []
        self.entries = [[None]*5 for _ in range(4)]
        for col, text in enumerate(("执行器", "目标角度滑条", "q / rad", "dq / rad/s", "Kp", "Kd", "tau / Nm")):
            ttk.Label(control, text=text).grid(row=0, column=col, padx=6, pady=(0, 12))
        for i in range(4):
            ttk.Label(control, text=self.motor_name(i)).grid(row=i+1, column=0, sticky="w", pady=12)
            low, high = (-6.28, 6.28) if i == 0 else (-2.2, 2.2) if i < 3 else (-math.pi, math.pi)
            scale = ttk.Scale(control, from_=low, to=high, command=lambda x, i=i: self.slider(i, x))
            scale.grid(row=i+1, column=1, sticky="ew", padx=10)
            self.scales.append(scale)
            for j in range(5):
                entry = ttk.Entry(control, textvariable=self.values[i][j], width=11)
                entry.grid(row=i+1, column=j+2, padx=5)
                self.entries[i][j] = entry
                entry.bind("<Return>", lambda _, i=i: self.commit(i))
                entry.bind("<FocusOut>", lambda _, i=i: self.commit(i))
                entry.bind("<KeyPress>", lambda _, i=i: self.edited.add(i))
        control.columnconfigure(1, weight=1)
        options = ttk.Frame(control)
        options.grid(row=5, column=0, columnspan=7, sticky="ew", pady=16)
        self.gravity_enabled = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="MIT＋重力补偿", variable=self.gravity_enabled, command=lambda: self.commit()).pack(side="left")
        ttk.Button(options, text="读取当前角度到目标", command=self.capture_angles).pack(side="left", padx=12)
        ttk.Label(control, text="初始 Kp / Kd / 外加力矩均为 0。滑条参考 URDF；数值输入不自动截断。\n夹爪滑条 ±π 为显示范围，夹爪不叠加模型重力。按 Enter 或移开焦点应用输入。", foreground="#53676c").grid(row=6, column=0, columnspan=7, sticky="w")
        ttk.Label(settings, text="三关节重力补偿比例", font=("Noto Sans CJK SC", 13, "bold")).pack(anchor="w")
        self.gravity = [tk.StringVar(value="1") for _ in range(3)]
        fields = ttk.Frame(settings)
        fields.pack(anchor="w", pady=20)
        for i, value in enumerate(self.gravity):
            ttk.Label(fields, text=f"joint{i+1}").grid(row=0, column=i, padx=12)
            ttk.Entry(fields, textvariable=value, width=15).grid(row=1, column=i, padx=12, pady=8)
        buttons = ttk.Frame(settings)
        buttons.pack(anchor="w")
        ttk.Button(buttons, text="读取实际值", command=lambda: self.call("读取设置", lambda d: d.settings())).pack(side="left")
        ttk.Button(buttons, text="填入 1.0", command=lambda: [v.set("1") for v in self.gravity]).pack(side="left", padx=8)
        ttk.Button(buttons, text="应用并保存", style="Accent.TButton", command=self.save_settings).pack(side="left")
        self.storage = ttk.Label(settings, text="未读取")
        self.storage.pack(anchor="w", pady=16)
        ttk.Label(settings, text="只保存前三关节 gravity_scale；不改写电机 ID、零位或摩擦参数。\n保存不隐式切换模式；NVS 成功写入后在控制周期应用。", foreground="#53676c").pack(anchor="w")
        ttk.Label(upgrade, text="USB 固件升级", font=("Noto Sans CJK SC", 13, "bold")).pack(anchor="w")
        self.image_path = tk.StringVar()
        row = ttk.Frame(upgrade)
        row.pack(fill="x", pady=16)
        ttk.Entry(row, textvariable=self.image_path).pack(side="left", fill="x", expand=True)
        ttk.Button(row, text="选择签名镜像", command=self.choose_image).pack(side="left", padx=(8, 0))
        self.image_info = ttk.Label(upgrade, text="请选择对应主 / 从臂的 zephyr.signed.bin；不上传 bootloader 或普通 bin。")
        self.image_info.pack(anchor="w")
        bar = ttk.Frame(upgrade)
        bar.pack(anchor="w", pady=16)
        ttk.Button(bar, text="上传镜像", style="Accent.TButton", command=self.upload).pack(side="left")
        ttk.Button(bar, text="取消上传", command=lambda: self.call("取消上传", lambda d: d.cancel_upload())).pack(side="left", padx=8)
        ttk.Button(bar, text="重启并确认版本", command=self.reboot).pack(side="left")
        self.progress = ttk.Progressbar(upgrade, maximum=100)
        self.progress.pack(fill="x", pady=8)
        self.upgrade_status = ttk.Label(upgrade, text="串口连接不提供升级；请连接 USB Bulk。", wraplength=1000)
        self.upgrade_status.pack(anchor="w")
        ttk.Label(upgrade, text="固件会请求 Damp 并等待失能反馈后开始升级。上传完成仅表示暂存，需显式重启。", foreground="#53676c").pack(anchor="w", pady=12)
        ttk.Label(outer, text="事件日志").pack(anchor="w", pady=(12, 4))
        self.log = tk.Text(outer, height=5, font=(self.ui_font, 9), background="#e7eef0", foreground="#24444c", relief="flat", state="disabled")
        self.log.pack(fill="x")

    @staticmethod
    def motor_name(i):
        return f"{'joint'+str(i+1) if i<3 else '夹爪'}  ·  ID {i+1}"

    def submit(self, kind, action, *, parallel=False, generation=None):
        generation = self.generation if generation is None else generation
        def run():
            try:
                result = action()
                if self.closed and kind == "连接":
                    result.close()
                else:
                    self.events.put((generation, kind, result, None))
            except Exception as error:
                self.events.put((generation, kind, getattr(error, "snapshot", None), str(error)))
        (self.urgent if kind in ("失能", "取消上传") else self.upgrades if parallel else self.commands).submit(run)

    def call(self, kind, action, parallel=False):
        session = self.session
        if session is None:
            self.notice.set("请先连接设备。")
            return
        def run():
            with session.use() as device:
                return action(device)
        self.submit(kind, run, parallel=parallel)

    def discover(self):
        kind = self.transport.get()
        def search():
            if kind == "USB Bulk":
                return [(f"{d.display_name}  {d.uri}", d.uri) for d in self.sdk.discover_devices(probe=False)]
            from serial.tools import list_ports
            return [(f"{d.device}  {d.description}", "serial://" + d.device) for d in list_ports.comports()]
        self.submit("搜索", search)

    def connect(self):
        selected = self.device_uri.get().strip()
        uri = self.uris.get(selected, selected)
        if self.transport.get() != "USB Bulk" and not uri.startswith("serial://"):
            uri = "serial://" + uri
        if not selected:
            self.notice.set("请选择设备或输入完整 URI。")
            return
        self.disconnect()
        self.submit("连接", lambda: self.sdk.UnderwaterSingle.connect(uri))
        self.notice.set("正在连接并读取设备身份…")

    def attach(self, device):
        self.session = Session(device)
        self.edited.clear()
        self.initialized.clear()
        self.last_sequence = None
        self.last_rx = self.rate_time = time.monotonic()
        self.tx_previous, self.rx_previous = device.tx_count, device.rx_count
        info = device.info()
        role = {1: "主臂 / 蓝灯", 2: "从臂 / 绿灯"}.get(info["role"], "未知角色")
        self.identity.configure(text=f"{role}   ·   {info['transport'].upper()}   ·   固件 {info['firmware']}   ·   UID {info['uid']}   ·   boot {info['boot_id']:X}")
        self.identity.configure(foreground="#235fba" if info["role"]==1 else "#22804d")
        self.notice.set("已连接，尚未请求使能。")
        self.call("读取设置", lambda d: d.settings())

    def disconnect(self):
        session, self.session = self.session, None
        self.generation += 1
        if session:
            session.retire()
            self.submit("断开", session.close)
        self.metrics.configure(text="连接已关闭")
        self.reasons.configure(text="未连接；表中保留的是上次快照，不再更新。")
        for i in range(4):
            self.table.item(str(i), tags=("stale",))
        self.mode.configure(text="未连接")

    def command_values(self):
        rows = []
        names = ("q", "dq", "Kp", "Kd", "tau")
        for i, row in enumerate(self.values):
            values = []
            for j, value in enumerate(row):
                self.entries[i][j].state(["!invalid"])
                try:
                    number = float(value.get())
                    if not math.isfinite(number) or (j in (2, 3) and number < 0):
                        raise ValueError()
                except ValueError:
                    self.entries[i][j].state(["invalid"])
                    raise ValueError(f"{self.motor_name(i)} / {names[j]}：请输入有限数值，Kp / Kd 不能为负。本次输入未应用。")
                values.append(number)
            rows.append(values)
        return [tuple(row[i] for row in rows) for i in range(5)]

    def slider(self, i, value):
        if self.updating_sliders:
            return
        self.edited.add(i)
        self.values[i][0].set(f"{float(value):.4f}")
        self.commit()

    def sync_sliders(self):
        self.updating_sliders = True
        try:
            for i, scale in enumerate(self.scales):
                scale.set(float(self.values[i][0].get()))
        finally:
            self.updating_sliders = False

    def commit(self, i=None):
        try:
            values = self.command_values()
            self.sync_sliders()
            if self.session:
                with self.session.use() as device:
                    device.update_command(*values, kind=2 if self.gravity_enabled.get() else 1)
        except Exception as error:
            self.notice.set(str(error))

    def start_control(self):
        try:
            values = self.command_values()
        except Exception as error:
            self.notice.set(str(error))
            return
        kind = 2 if self.gravity_enabled.get() else 1
        self.call("开始 Control", lambda d: d.start_control(*values, kind=kind))

    def capture_angles(self):
        if not self.session:
            return
        state = self.session.device.read_state()
        if state:
            for i in range(4):
                if state.fresh_mask & (1 << i):
                    self.values[i][0].set(f"{state.q[i]:.6f}")
                    self.edited.add(i)
            self.commit()

    def save_settings(self):
        try:
            gravity = tuple(float(v.get()) for v in self.gravity)
            if not all(math.isfinite(x) for x in gravity):
                raise ValueError("重力比例必须为有限数值。")
        except ValueError as error:
            self.notice.set(str(error))
            return
        self.call("保存设置", lambda d: d.settings(gravity))

    def choose_image(self):
        name = filedialog.askopenfilename(filetypes=[("MCUboot 签名镜像", "*.bin"), ("所有文件", "*")])
        if name:
            self.image_path.set(name)
            try:
                with open(name, "rb") as f:
                    version = image_version(f.read(32))
                self.image_info.configure(text=f"镜像版本：{version}" if version else "未识别 MCUboot 镜像头。")
            except OSError as error:
                self.notice.set(str(error))

    def upload(self):
        path = self.image_path.get()
        if not self.session or self.session.device.kind != "usb":
            self.notice.set("固件升级需连接 USB Bulk；UART7 不提供升级。")
            return
        self.call("开始上传", lambda d: d.start_upload(Path(path).read_bytes()), parallel=True)

    def reboot(self):
        if not self.session or self.session.device.kind != "usb":
            self.notice.set("重启与版本确认需连接 USB Bulk。")
            return
        try:
            with open(self.image_path.get(), "rb") as f:
                parsed = image_version(f.read(32))
            if parsed is None:
                raise ValueError("请选择要验证的 MCUboot 签名镜像。")
            version = self.sdk.Version()
            version.major, version.minor, version.patch = parsed.major, parsed.minor, parsed.patch
        except Exception as error:
            self.notice.set(str(error))
            return
        self.call("重启确认", lambda d: d.reboot_and_wait(version), parallel=True)

    def event(self, kind, result, error):
        message = f"{kind}失败：{error}" if error else f"{kind}完成"
        self.notice.set(message)
        self.log.configure(state="normal")
        self.log.insert("end", time.strftime("%H:%M:%S ") + message + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")
        if error:
            if result and "storage_error" in result:
                self.storage.configure(text=f"写入未成功；实际 revision={result['revision']}，storage_error={result['storage_error']}。请读取实际值核对。")
            return
        if kind == "搜索":
            self.uris = dict(result)
            self.device_list.configure(values=list(self.uris))
            if result:
                self.device_uri.set(result[0][0])
            else:
                self.notice.set("未发现设备。串口也可直接输入 /dev/ttyUSB0 或 COM3。")
        elif kind == "连接":
            self.attach(result)
        elif kind in ("读取设置", "保存设置"):
            for value, number in zip(self.gravity, result["gravity_scale"]):
                value.set(f"{number:.6g}")
            self.storage.configure(text=f"revision={result['revision']}   storage_error={result['storage_error']}")
        elif kind == "开始上传":
            self.notice.set(f"上传提交结果：{result}")
        elif kind == "重启确认":
            self.notice.set(f"重启验证结果：{result.state} / {result.error}")
            if self.session:
                info = self.session.device.info()
                self.identity.configure(text=f"{ {1:'主臂 / 蓝灯',2:'从臂 / 绿灯'}.get(info['role'],'未知角色')}  ·  {info['transport'].upper()}  ·  固件 {info['firmware']}  ·  UID {info['uid']}  ·  boot {info['boot_id']:X}")

    def refresh(self):
        if self.closed:
            return
        while True:
            try:
                generation, kind, result, error = self.events.get_nowait()
            except Empty:
                break
            if generation == self.generation:
                self.event(kind, result, error)
            elif kind == "连接" and result is not None:
                self.commands.submit(result.close)
        if self.session:
            device = self.session.device
            state = device.read_state()
            now = time.monotonic()
            if state:
                if state.sequence != self.last_sequence:
                    self.last_sequence = state.sequence
                    self.rx_count += 1
                    self.last_rx = now
                self.mode.configure(text=f"{ {0:'Damp',1:'Drag',3:'Control'}.get(state.mode,state.mode)}  ·  控制源 { {0:'无',1:'USB',2:'UART7'}.get(state.source,state.source)}")
                for i in range(4):
                    fresh = bool(state.fresh_mask & (1 << i))
                    self.table.item(str(i), tags=() if fresh else ("stale",), values=(self.motor_name(i), f"{state.q[i]:.5f}", f"{state.dq[i]:.4f}", f"{state.tau[i]:.4f}", state.temperature[i], state.motor_status[i], state.age_ms[i] if state.age_ms[i] != 0xffffffff else "未收到"))
                    if fresh and i not in self.initialized and i not in self.edited:
                        self.values[i][0].set(f"{state.q[i]:.6f}")
                        self.initialized.add(i)
                        self.updating_sliders = True
                        self.scales[i].set(state.q[i])
                        self.updating_sliders = False
                self.masks.configure(text=f"available 0x{state.available_mask:02X}    fresh 0x{state.fresh_mask:02X}    enabled 0x{state.enabled_mask:02X}    applied 0x{state.applied_mask:02X}    状态序号 {state.sequence}    回显命令 {state.applied_sequence}")
                stop = {0:"无",1:"命令间隔超过100ms，重新进入Control恢复",3:"输出失败，请检查CAN/反馈并显式请求模式",4:"使能超时，请检查电机反馈"}.get(state.stop_reason,state.stop_reason)
                reject = {0:"无",1:"非Control",2:"会话不匹配",3:"序号重复/倒序",4:"kind错误",5:"数值错误",6:"命令过期",7:"另一链路持有Control，请先Damp"}.get(state.reject_reason,state.reject_reason)
                self.reasons.configure(text=f"停止原因：{stop}    最近拒绝：{reject}    状态交付距今 {(now-self.last_rx)*1000:.0f} ms")
            if now - self.rate_time >= 1:
                elapsed = now - self.rate_time
                self.metrics.configure(text=f"命令提交 {(device.tx_count-self.tx_previous)/elapsed:.0f} Hz   状态交付 {(device.rx_count-self.rx_previous)/elapsed:.0f} Hz")
                if device.kind == "serial":
                    self.metrics.configure(text=self.metrics.cget("text") + f"  主机 RX CRC {device.backend.crc_errors} / 格式 {device.backend.format_errors}")
                self.tx_previous = device.tx_count
                self.rx_previous = device.rx_count
                self.rx_count = 0
                self.rate_time = now
                if device.last_error:
                    self.notice.set("命令发送停止：" + device.last_error)
                if device.kind == "usb":
                    p = device.firmware_progress()
                    self.upgrade_status.configure(text=f"{p.state} / {p.error}   {p.acknowledged_bytes} B")
                    if p.total_bytes:
                        self.progress["value"] = 100*p.acknowledged_bytes/p.total_bytes
        self.root.after(33, self.refresh)

    def close(self):
        self.closed = True
        self.disconnect()
        while True:
            try:
                _, kind, result, _ = self.events.get_nowait()
            except Empty:
                break
            if kind == "连接" and result is not None:
                self.commands.submit(result.close)
        self.commands.shutdown(wait=False)
        self.upgrades.shutdown(wait=False)
        self.urgent.shutdown(wait=False)
        self.root.destroy()
