"""Small Tk/ttk desktop console backed by the real Underwater SDK."""
import time
import tkinter as tk
from tkinter import filedialog, ttk
from queue import Empty

from .controller import ConsoleController
from .settings import SettingsPage
from .arm_control import ArmControlPage


def name(value):
    return getattr(value, "name", str(value))


class UnderwaterApp:
    def __init__(self, root, controller=None, auto_scan=True):
        self.root = root
        self.controller = controller or ConsoleController()
        self.devices = []
        self.last_rpc_message = None
        self.last_progress = None
        self.device_name = "Underwater"
        self.awaiting_reboot = False
        self.expected_version = None
        self.closing = False
        root.title("Underwater · USB 控制台")
        root.geometry("1160x900")
        root.minsize(1020, 680)
        style = ttk.Style(root)
        if "clam" in style.theme_names():
            style.theme_use("clam")
        style.configure("TFrame", background="#f2f5f8")
        style.configure("TLabel", background="#f2f5f8", foreground="#243747", font=("sans", 10))
        style.configure("Title.TLabel", font=("sans", 20, "bold"), foreground="#095c72")
        style.configure("Mode.TLabel", font=("sans", 17, "bold"), foreground="#095c72")
        style.configure("TButton", padding=(14, 8), font=("sans", 10))
        style.configure("Compact.TButton", padding=(10, 5), font=("sans", 10))
        style.configure("TLabelframe", background="#f2f5f8")
        style.configure("TLabelframe.Label", background="#f2f5f8", foreground="#243747")
        style.map("TEntry", fieldbackground=[("invalid", "#ffe6e6")])
        style.map("TButton", foreground=[("pressed", "#ffffff")],
                  background=[("pressed", "#095c72")])
        outer = ttk.Frame(root, padding=16)
        outer.pack(fill="both", expand=True)
        outer.columnconfigure(0, weight=1)
        outer.rowconfigure(3, weight=1)
        heading = ttk.Frame(outer)
        heading.grid(row=0, column=0, sticky="ew")
        ttk.Label(heading, text="Underwater", style="Title.TLabel").pack(side="left")
        self.connection = tk.StringVar(value="未连接 · USB Bulk")
        ttk.Label(heading, textvariable=self.connection).pack(side="right", pady=8)
        ttk.Label(outer, text="机械臂主从控制 / 固件升级 / 设备设置").grid(row=1, column=0, sticky="w", pady=(2, 8))

        connection_bar = ttk.Frame(outer)
        connection_bar.grid(row=2, column=0, sticky="ew", pady=(0, 8))
        self.choice = ttk.Combobox(connection_bar, state="readonly", width=30)
        self.choice.pack(side="left", fill="x", expand=True, padx=(0, 8))
        ttk.Button(connection_bar, text="搜索 USB", command=self.search).pack(side="left", padx=3)
        ttk.Button(connection_bar, text="连接", command=self.connect).pack(side="left", padx=3)
        ttk.Button(connection_bar, text="断开", command=self.controller.disconnect).pack(side="left", padx=3)

        tabs = ttk.Notebook(outer)
        tabs.grid(row=3, column=0, sticky="nsew")
        self.arm_page = ArmControlPage(tabs, self.controller, self.command, self.log)
        tabs.add(self.arm_page, text="  机械臂控制  ")
        control = ttk.Frame(tabs, padding=18)
        upgrade = ttk.Frame(tabs, padding=18)
        tabs.add(control, text="  设备状态 / 零点  ")
        tabs.add(upgrade, text="  固件升级  ")
        self.settings_page = SettingsPage(tabs, self.controller, self.log)
        tabs.add(self.settings_page, text="  设备设置  ")
        self.mode = tk.StringVar(value="—")
        self.reason = tk.StringVar(value="请选择设备并连接。")
        self.summary = tk.StringVar(value="设备状态尚未读取")
        ttk.Label(control, text="当前模式").pack(anchor="w")
        ttk.Label(control, textvariable=self.mode, style="Mode.TLabel").pack(anchor="w", pady=(5, 8))
        ttk.Label(control, textvariable=self.summary).pack(anchor="w", pady=(0, 8))
        ttk.Label(control, textvariable=self.reason, wraplength=830).pack(anchor="w", pady=(0, 18))
        buttons = ttk.Frame(control)
        buttons.pack(fill="x", pady=(0, 18))
        for label, method in (("使能 · Gravity", "enable"), ("失能 · Damp", "disable"),
                              ("进入遥操", "enter_teleop"), ("刷新状态", "status")):
            ttk.Button(buttons, text=label, command=lambda method=method: self.command(method)).pack(side="left", padx=(0, 8))
        self.zeroing_busy = False
        self.zeroing_ready = False
        self.zeroing_button = ttk.Button(buttons, text="设置零点", command=self.zeroing, state="disabled")
        self.zeroing_button.pack(side="left", padx=(0, 8))
        ttk.Label(control, text="设置零点：先点击失能，在 Damp 下摆好两臂姿态，再将当前位置写为零点。").pack(anchor="w", pady=(0, 8))
        columns = ("arm", "available", "enabled", "disabling")
        self.table = ttk.Treeview(control, columns=columns, show="headings", height=2, selectmode="none")
        for col, title in zip(columns, ("机械臂", "在线反馈", "已使能", "等待失能确认")):
            self.table.heading(col, text=title)
            self.table.column(col, width=190, anchor="center")
        self.table.pack(fill="x")
        self.table.insert("", "end", iid="master", values=("主臂 / Master", "—", "—", "—"))
        self.table.insert("", "end", iid="slave", values=("从臂 / Slave", "—", "—", "—"))
        ttk.Label(control, text="位图 0～3：joint1～4；位 4：夹爪。接收请求与完成使能 / 对齐分别显示。").pack(anchor="w", pady=(10, 4))
        ttk.Label(control, text="Gravity / Teleop 断开后保持当前模式；Control 在命令超时或 USB 断开后失能。").pack(anchor="w")

        ttk.Label(upgrade, text="本地固件文件（zephyr.signed.bin）").pack(anchor="w")
        file_row = ttk.Frame(upgrade)
        file_row.pack(fill="x", pady=(6, 12))
        self.image = tk.StringVar()
        self.image.trace_add("write", self.inspect_image)
        ttk.Entry(file_row, textvariable=self.image).pack(side="left", fill="x", expand=True, padx=(0, 8))
        ttk.Button(file_row, text="选择文件…", command=self.choose_file).pack(side="left")
        options = ttk.Frame(upgrade)
        options.pack(fill="x", pady=(0, 12))
        self.do_reboot = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="写入后重启", variable=self.do_reboot).pack(side="left", padx=(0, 22))
        self.image_version = tk.StringVar(value="固件版本：选择文件后自动读取")
        ttk.Label(options, textvariable=self.image_version).pack(side="left")
        actions = ttk.Frame(upgrade)
        actions.pack(fill="x", pady=14)
        ttk.Button(actions, text="开始升级", command=self.upload).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="取消传输", command=self.controller.cancel_upload).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="查询启动状态", command=self.boot_queries).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="重启设备", command=lambda: self.command("reboot")).pack(side="left")
        self.progress = ttk.Progressbar(upgrade, mode="determinate", maximum=100)
        self.progress.pack(fill="x", pady=(0, 8))
        self.upload_status = tk.StringVar(value="等待选择固件")
        self.boot_status = tk.StringVar(value="尚未读取启动槽信息")
        self.running_version = tk.StringVar(value="尚未读取运行版本")
        ttk.Label(upgrade, textvariable=self.upload_status, wraplength=820).pack(anchor="w")
        ttk.Label(upgrade, textvariable=self.boot_status, wraplength=820).pack(anchor="w", pady=8)
        ttk.Label(upgrade, textvariable=self.running_version, wraplength=820).pack(anchor="w")
        ttk.Label(upgrade, text="设备会先进入 Damp 并确认本次在线电机失能，再写入 / 重启；Busy 或失败原因显示在下方。", wraplength=820).pack(anchor="w", pady=(5, 0))

        ttk.Label(outer, text="操作反馈").grid(row=4, column=0, sticky="w", pady=(14, 6))
        self.log_box = tk.Text(outer, height=3, relief="flat", background="#e7edf2", foreground="#243747",
                               font=("monospace", 10), padx=10, pady=8, wrap="word", state="disabled")
        self.log_box.grid(row=5, column=0, sticky="ew")
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(50, self.events)
        root.after(1000, self.poll)
        if auto_scan:
            self.search()

    def log(self, text):
        self.log_box.configure(state="normal")
        self.log_box.insert("end", f"{time.strftime('%H:%M:%S')}  {text}\n")
        if int(self.log_box.index("end-1c").split(".")[0]) > 400:
            self.log_box.delete("1.0", "100.0")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")

    def search(self):
        self.log("正在搜索 USB 设备…")
        self.controller.discover()

    def connect(self):
        selected = self.choice.current()
        if selected < 0:
            self.log("请先搜索并选择一个 USB 设备。")
            return
        self.connection.set("正在连接…")
        self.controller.connect(self.devices[selected])

    def refresh_zeroing_button(self):
        enabled = self.controller.connected and self.zeroing_ready and not self.zeroing_busy
        self.zeroing_button.configure(state="normal" if enabled else "disabled")

    def zeroing(self):
        if not self.zeroing_ready or self.zeroing_busy:
            self.log("请先失能并等待 Damp 状态，再设置零点。")
            return
        self.zeroing_busy = True
        self.zeroing_ready = False
        self.refresh_zeroing_button()
        self.controller.zeroing()

    def command(self, method):
        if method != "status":
            self.zeroing_ready = False
            self.refresh_zeroing_button()
        self.log(f"提交 {method}，等待设备处理…")
        self.controller.command(method)

    def choose_file(self):
        path = filedialog.askopenfilename(title="选择 Underwater 固件", filetypes=(("固件镜像", "*.bin"), ("所有文件", "*")))
        if path:
            self.image.set(path)

    def upload(self):
        self.log("提交固件升级请求…")
        self.controller.upload(self.image.get(), self.do_reboot.get())

    def inspect_image(self, *_):
        path = self.image.get()
        self.image_version.set("固件版本：正在读取…" if path else "固件版本：选择文件后自动读取")
        if path:
            self.controller.inspect_image(path)

    def boot_queries(self):
        self.controller.command("boot_status")
        self.controller.command("device_info")

    def update_status(self, result, action):
        self.zeroing_ready = self.controller.zeroing_allowed(result)
        self.refresh_zeroing_button()
        info = result.info
        if action in ("enable", "disable", "enter_teleop", "enter_control"):
            self.arm_page.feedback.set((info.reason if info is not None else "") or
                ("模式请求已处理。" if result else f"请求失败：{name(result.error)}"))
        if result:
            self.connection.set(f"已连接 · {self.device_name}")
            if self.awaiting_reboot:
                self.awaiting_reboot = False
                self.boot_queries()
        if info is not None:
            self.arm_page.update_mode(info)
            self.mode.set(name(info.mode))
            self.summary.set(f"运行 {info.uptime_ms / 1000:.1f} s   使能中 0x{info.enabling_arms:X}   "
                             f"重力补偿 0x{info.gravity_arms:X}   升级中 {info.upgrading}")
            self.reason.set(info.reason or "—")
            for arm, label in (("master", "主臂 / Master"), ("slave", "从臂 / Slave")):
                self.table.item(arm, values=(label,
                    f"0x{getattr(info, arm + '_available_mask'):02X}",
                    f"0x{getattr(info, arm + '_enabled_mask'):02X}",
                    f"0x{getattr(info, arm + '_disable_pending_mask'):02X}"))
        text = f"{action}: {'已处理' if result else '失败'} | {name(result.error)} | status={result.device_status}"
        if info is not None and info.reason:
            text += f" | {info.reason}"
        if action != "status" or not result:
            if action != "status" or text != self.last_rpc_message:
                self.log(text)
        self.last_rpc_message = text
        if not result and info is None:
            self.connection.set("未收到设备响应 · 等待恢复")
            self.reason.set(f"状态读取失败，表格为上次反馈：{name(result.error)}")

    def handle_event(self, event):
        if event.generation is not None and event.generation != self.controller.generation:
            return
        value = event.value
        if event.kind == "devices":
            self.devices = value
            self.choice["values"] = [f"{d.display_name or 'Underwater'} · {d.serial_number or d.uri}" for d in value]
            self.choice.set("")
            if len(value) == 1:
                self.choice.current(0)
            self.log(f"找到 {len(value)} 个 USB 设备，请选择后连接。")
        elif event.kind == "connected":
            self.arm_page.reset()
            self.zeroing_busy = self.zeroing_ready = False
            self.refresh_zeroing_button()
            self.settings_page.reset()
            self.expected_version = None
            self.awaiting_reboot = False
            self.device_name = value.display_name or "Underwater"
            self.connection.set(f"已连接 · {self.device_name}")
            self.log(f"连接成功：{value.uri}")
            self.controller.poll()
            self.boot_queries()
            self.controller.command("settings")
        elif event.kind == "disconnected":
            self.arm_page.reset()
            self.zeroing_busy = self.zeroing_ready = False
            self.refresh_zeroing_button()
            self.settings_page.reset()
            self.expected_version = None
            self.awaiting_reboot = False
            self.connection.set("未连接 · USB Bulk")
            self.reason.set("连接已关闭；Control 按固件现有断连规则失能。")
            self.log("已断开 USB 连接。")
        elif event.kind in ("status", "enable", "disable", "enter_teleop", "enter_control", "settings_mode", "zeroing"):
            self.update_status(value, "status" if event.kind == "settings_mode" else event.kind)
            if event.kind == "zeroing" and value:
                # Saved motor coordinates changed; reseed from subsequent status.
                for row in self.arm_page.rows:
                    row["dirty"] = row["seeded"] = False
        elif event.kind == "zeroing_done":
            self.zeroing_busy = False
            self.zeroing_ready = False
            self.refresh_zeroing_button()
            self.controller.command("status", coalesce=True)
        elif event.kind == "zeroing_error":
            self.log(str(value))
        elif event.kind in ("settings", "default_settings", "set_settings"):
            self.settings_page.update_result(value, event.kind)
        elif event.kind in ("settings_wait", "settings_error"):
            self.settings_page.feedback.set(str(value))
            self.log(str(value))
        elif event.kind == "progress":
            fraction = 100 * value.acknowledged_bytes / value.total_bytes if value.total_bytes else 0
            self.progress["value"] = fraction
            state = "写入完成（已暂存）" if value.state == self.controller.sdk.FirmwareUploadState.Completed else name(value.state)
            text = f"{state} · {value.acknowledged_bytes} / {value.total_bytes} 字节 ({fraction:.1f}%)"
            self.upload_status.set(f"{text} · {name(value.error)} · status={value.device_status}")
            if not value.active and value.state != self.controller.sdk.FirmwareUploadState.Idle:
                if self.last_progress != text:
                    self.log(self.upload_status.get())
            self.last_progress = text
        elif event.kind == "upload_start":
            result, version = value
            self.log(f"升级请求：{name(result)} · 固件版本：{version if version else '未读到 MCUboot 版本信息'}")
        elif event.kind == "image_info":
            path, version = value
            if path == self.image.get():
                self.image_version.set(f"固件版本：{version if version else '未读到 MCUboot 版本信息'}")
        elif event.kind == "image_info_error":
            path, error = value
            if path == self.image.get():
                self.image_version.set("固件版本：文件读取失败")
                self.log(f"读取固件版本失败：{error}")
        elif event.kind == "boot_status":
            self.boot_status.set(f"{name(value.error)} status={value.device_status} · active={value.active_slot} "
                f"confirmed={value.confirmed_slot} pending={value.pending_slot} · "
                f"slot0=0x{value.slot0_address:08X}/{value.slot0_size} slot1=0x{value.slot1_address:08X}/{value.slot1_size}")
            self.log(self.boot_status.get())
        elif event.kind in ("reboot", "image_reboot"):
            if event.kind == "image_reboot":
                value, version = value
                self.expected_version = version if value.departure_observed else None
            if value.departure_observed:
                self.awaiting_reboot = True
                self.connection.set("重启中 · 等待 USB 自动重连")
            self.log(f"重启：{name(value.error)} status={value.device_status} · "
                f"请求已接受={value.request_accepted} USB离开={value.departure_observed}。"
                "等待同设备自动重连；持续无响应时重新搜索并连接，可查询启动状态确认运行槽。")
        elif event.kind == "device_info":
            if value.info is not None:
                version = value.info.fw_version
                self.running_version.set(f"运行版本 {version.major}.{version.minor}.{version.patch} · "
                    f"{value.info.board_name} · {value.info.serial_number}")
                if self.expected_version is not None:
                    match = "与固件一致" if self.expected_version.matches(version) else f"与固件 {self.expected_version} 不一致"
                    self.running_version.set(f"{self.running_version.get()} · 版本{match}")
            else:
                self.running_version.set(f"运行版本读取失败：{name(value.error)} status={value.device_status}")
            self.log(self.running_version.get())
        elif event.kind == "cancel_firmware_upload":
            self.log("已请求取消传输，等待终态。")
        elif event.kind == "error":
            self.log(str(value))
            if not self.controller.connected:
                self.connection.set("未连接 · USB Bulk")
        elif event.kind == "message":
            self.log(str(value))

    def events(self):
        if self.closing:
            return
        try:
            while True:
                self.handle_event(self.controller.events.get_nowait())
        except Empty:
            pass
        self.arm_page.update_stream(self.controller.arm_stream_report())
        self.root.after(50, self.events)

    def poll(self):
        if self.closing:
            return
        self.controller.poll()
        self.root.after(1000, self.poll)

    def close(self):
        self.closing = True
        self.controller.shutdown()
        self.root.destroy()
