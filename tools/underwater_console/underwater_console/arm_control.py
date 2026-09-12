"""Manual ten-axis MIT controls in motor coordinates (radians)."""
import json
import math
from pathlib import Path
import struct
import tkinter as tk
from tkinter import ttk


MODE_BUTTONS = (("失能", "disable", "Damp"), ("拖动", "enable", "Gravity"),
                ("跟随", "enter_teleop", "Teleop"), ("上位机控制", "enter_control", "Control"))
FIELDS = (("dq", "速度 / rad/s", 0.0), ("tau", "前馈 / N·m", 0.0),
          ("kp", "Kp / N·m/rad", 20.0), ("kd", "Kd / N·m·s/rad", 0.5))
REJECT_REASONS = {1: "当前模式不接收命令", 2: "Control 会话不匹配", 3: "命令序号过期",
                  4: "MIT 类型无效", 5: "参数数值无效", 6: "命令已过期"}
STOP_REASONS = {1: "命令超时", 2: "USB 已断开"}


def joint_limits():
    return json.loads(Path(__file__).with_name("joint_limits.json").read_text())["axes"]


def parse_value(text, label):
    try:
        value = float(text)
    except ValueError as exc:
        raise ValueError(f"{label}：请输入数值") from exc
    if not math.isfinite(value):
        raise ValueError(f"{label}：请输入有限数值")
    try:
        struct.pack("<f", value)
    except OverflowError as exc:
        raise ValueError(f"{label}：超出 float32 范围") from exc
    return value


class ArmControlPage(ttk.Frame):
    def __init__(self, parent, controller, command, log, limits=None):
        super().__init__(parent, padding=12)
        self.controller = controller
        self.send_mode = command
        self.log = log
        self.limits = joint_limits() if limits is None else limits
        self.rows = []
        self.latest_status = None
        self.last_error = ""
        self.mode = tk.StringVar(value="未连接")
        self.feedback = tk.StringVar(value="连接后读取实测角度；进入上位机控制后发送目标值。")
        self.stream_text = tk.StringVar(value="USB Bulk · 等待连接")
        self.gravity = tk.BooleanVar(value=True)

        bar = ttk.Frame(self)
        bar.pack(fill="x", pady=(0, 8))
        self.mode_buttons = {}
        for label, method, mode in MODE_BUTTONS:
            button = ttk.Button(bar, text=label, style="Compact.TButton", command=lambda m=method: self.switch_mode(m))
            button.pack(side="left", padx=(0, 8))
            self.mode_buttons[mode] = button
        ttk.Label(bar, textvariable=self.mode, style="Mode.TLabel").pack(side="right")
        options = ttk.Frame(self)
        options.pack(fill="x", pady=(0, 8))
        ttk.Checkbutton(options, text="重力补偿（MitWithGravity）", variable=self.gravity,
                        command=self.publish).pack(side="left")
        ttk.Button(options, text="读取当前角度", style="Compact.TButton", command=self.use_measured_angles).pack(side="right")
        ttk.Label(self, textvariable=self.stream_text).pack(anchor="w", pady=(0, 6))

        viewport = tk.Canvas(self, background="#f2f5f8", highlightthickness=0)
        scroll = ttk.Scrollbar(self, orient="vertical", command=viewport.yview)
        viewport.configure(yscrollcommand=scroll.set)
        scroll.pack(side="right", fill="y")
        # Pack footer first so it remains visible on short windows.
        footer = ttk.Frame(self)
        footer.pack(side="bottom", fill="x", pady=(8, 0))
        ttk.Label(footer, textvariable=self.feedback, wraplength=940).pack(anchor="w")
        ttk.Label(footer, text="角度使用电机坐标；滑条即时生效，输入框按 Enter 或移开焦点后生效。",
                  wraplength=940).pack(anchor="w", pady=(3, 0))
        viewport.pack(fill="both", expand=True)
        form = ttk.Frame(viewport)
        content = viewport.create_window((0, 0), window=form, anchor="nw")
        form.bind("<Configure>", lambda _: viewport.configure(scrollregion=viewport.bbox("all")))
        viewport.bind("<Configure>", lambda e: viewport.itemconfigure(content, width=e.width))
        viewport.bind("<Button-4>", lambda _: viewport.yview_scroll(-1, "units"))
        viewport.bind("<Button-5>", lambda _: viewport.yview_scroll(1, "units"))
        viewport.bind("<MouseWheel>", lambda e: viewport.yview_scroll(-int(e.delta / 120), "units"))
        form.columnconfigure(1, weight=1, minsize=160)
        headers = ("电机", "目标角度 / rad", "目标值 / rad", *(field[1] for field in FIELDS), "实测 / rad")
        for column, title in enumerate(headers):
            ttk.Label(form, text=title).grid(row=0, column=column, padx=5, pady=(0, 8), sticky="w")
        for index, spec in enumerate(self.limits):
            if index in (0, 5):
                row = 1 if index == 0 else 8
                group = ttk.Frame(form)
                group.grid(row=row, column=0, columnspan=8, sticky="ew", pady=(6, 4))
                label = "主臂 / Master · CAN1" if index == 0 else "从臂 / Slave · CAN2"
                ttk.Label(group, text=label, foreground="#095c72", font=("sans", 10, "bold")).pack(side="left")
                ttk.Separator(group).pack(side="left", fill="x", expand=True, padx=(14, 0))
            row = index + (2 if index < 5 else 4)
            title = f"{'夹爪' if index % 5 == 4 else 'J' + str(index % 5 + 1)} · 0x{index % 5 + 1:02X}"
            ttk.Label(form, text=title).grid(row=row, column=0, padx=(0, 8), pady=2, sticky="w")
            q = tk.DoubleVar(value=0.0)
            values = {field: tk.StringVar(value=f"{default:g}") for field, _, default in FIELDS}
            values["q"] = tk.StringVar(value="0.000")
            measured = tk.StringVar(value="—")
            rail = ttk.Frame(form)
            rail.grid(row=row, column=1, sticky="ew", padx=5)
            ttk.Label(rail, text=f"{spec['lower']:g}").pack(side="left")
            slider = ttk.Scale(rail, from_=spec["lower"], to=spec["upper"], variable=q,
                               command=lambda value, i=index: self.slide(i, float(value)))
            slider.pack(side="left", fill="x", expand=True, padx=5)
            ttk.Label(rail, text=f"{spec['upper']:g}").pack(side="right")
            widgets = {}
            for column, field in enumerate(("q", *(field[0] for field in FIELDS)), 2):
                entry = ttk.Entry(form, textvariable=values[field], width=9 if field == "q" else 8)
                entry.grid(row=row, column=column, padx=5, pady=1, sticky="ew")
                entry.bind("<Return>", lambda _, i=index, f=field: self.commit(i, f))
                entry.bind("<FocusOut>", lambda _, i=index, f=field: self.commit(i, f))
                widgets[field] = entry
            ttk.Label(form, textvariable=measured, width=9).grid(row=row, column=7, padx=5, sticky="e")
            self.rows.append(dict(q=q, values=values, measured=measured, widgets=widgets,
                                  slider=slider, dirty=False, seeded=False,
                                  applied={"q": 0.0, **{f: d for f, _, d in FIELDS}}))
        # Tk does not bubble child-widget wheel events to the Canvas.
        def wheel(event):
            direction = -1 if getattr(event, "num", 0) == 4 or getattr(event, "delta", 0) > 0 else 1
            viewport.yview_scroll(direction, "units")
            return "break"
        def bind_wheel(widget):
            for sequence in ("<Button-4>", "<Button-5>", "<MouseWheel>"):
                widget.bind(sequence, wheel)
            for child in widget.winfo_children():
                bind_wheel(child)
        bind_wheel(form)

    def reset(self):
        self.latest_status = None
        self.last_error = ""
        self.mode.set("未连接")
        self.stream_text.set("USB Bulk · 等待电机状态")
        self.feedback.set("连接后读取实测角度；进入上位机控制后发送目标值。")
        self.gravity.set(True)
        for row in self.rows:
            row["dirty"] = row["seeded"] = False
            row["q"].set(0)
            row["values"]["q"].set("0.000")
            row["applied"] = {"q": 0.0, **{f: d for f, _, d in FIELDS}}
            row["measured"].set("—")
            for field, _, default in FIELDS:
                row["values"][field].set(f"{default:g}")
            for widget in row["widgets"].values():
                widget.state(["!invalid"])
        self.publish()

    def slide(self, index, value):
        row = self.rows[index]
        row["dirty"] = True
        row["applied"]["q"] = value
        row["values"]["q"].set(f"{value:.3f}")
        row["widgets"]["q"].state(["!invalid"])
        self.publish()

    def commit(self, index, field):
        row = self.rows[index]
        label = f"{'主臂' if index < 5 else '从臂'} {self.limits[index]['joint']} / {field}"
        try:
            value = parse_value(row["values"][field].get(), label)
            if field == "q":
                spec = self.limits[index]
                if not spec["lower"] <= value <= spec["upper"]:
                    raise ValueError(f"{label}：角度范围为 {spec['lower']:g}～{spec['upper']:g} rad")
            # These are the SDK's MIT argument rules, not new motion limits.
            if field in ("kp", "kd") and value < 0:
                raise ValueError(f"{label}：SDK 要求增益不小于 0")
        except ValueError as exc:
            row["widgets"][field].state(["invalid"])
            self.feedback.set(f"{exc}；此项未应用，当前值 {row['applied'][field]:g}。")
            return False
        row["widgets"][field].state(["!invalid"])
        row["applied"][field] = value
        if field == "q":
            row["dirty"] = True
            row["q"].set(value)
        self.publish()
        return True

    def publish(self):
        command = self.controller.sdk.UnderwaterArmCommand()
        for field in ("q", "dq", "tau", "kp", "kd"):
            setattr(command, field, [row["applied"][field] for row in self.rows])
        command.kind = (self.controller.sdk.UnderwaterControlKind.MitWithGravity if self.gravity.get()
                        else self.controller.sdk.UnderwaterControlKind.MIT)
        self.controller.publish_arm_command(command)

    def switch_mode(self, method):
        if method == "enter_control":
            # Commit any text still being edited before starting the stream.
            for index, row in enumerate(self.rows):
                for field in row["values"]:
                    if not self.commit(index, field):
                        return
            self.feedback.set("正在请求上位机控制；设备接受后按 250 Hz 发送当前目标。")
        self.send_mode(method)

    def use_measured_angles(self):
        if self.latest_status is None:
            self.feedback.set("尚未收到电机状态。")
            return
        for row in self.rows:
            row["dirty"] = row["seeded"] = False
        self.seed_angles(self.latest_status, force=True)

    def seed_angles(self, status, force=False):
        changed = False
        for i, row in enumerate(self.rows):
            if status.fresh_mask & (1 << i) and not row["dirty"] and (force or not row["seeded"]):
                value = status.q[i]
                spec = self.limits[i]
                # Preserve measured positions outside the slider span; do not
                # silently turn them into a command to the nearest endpoint.
                row["q"].set(value)
                row["values"]["q"].set(f"{value:.4f}")
                row["applied"]["q"] = value
                row["seeded"] = True
                changed = True
                if not spec["lower"] <= value <= spec["upper"]:
                    self.feedback.set(f"{spec['arm']} {spec['joint']} 实测角度超出滑条范围：{value:.4f} rad。")
        if changed:
            self.publish()

    def update_mode(self, info):
        mode = getattr(info.mode, "name", str(info.mode))
        translated = next((label for label, _, m in MODE_BUTTONS if m == mode), mode)
        self.mode.set(translated)
        for key, button in self.mode_buttons.items():
            button.state(["pressed"] if key == mode else ["!pressed"])

    def update_stream(self, report):
        if report is None:
            return
        status = report.status
        if status is not None:
            self.latest_status = status
            self.update_mode(status)
            self.seed_angles(status)
            for i, row in enumerate(self.rows):
                suffix = "" if status.fresh_mask & (1 << i) else " *"
                row["measured"].set(f"{status.q[i]:.4f}{suffix}")
            detail = f"在线 0x{status.fresh_mask:03X} · 执行 0x{status.applied_mask:03X}"
            if status.command_reject_reason:
                detail += f" · {REJECT_REASONS.get(status.command_reject_reason, '拒绝码 ' + str(status.command_reject_reason))}"
            if status.control_stop_reason:
                detail += f" · {STOP_REASONS.get(status.control_stop_reason, '停止码 ' + str(status.control_stop_reason))}"
        else:
            detail = "等待 ArmStatus"
        self.stream_text.set(f"{'发送中' if report.sending else '未发送'} · TX {report.tx_hz:.0f} Hz / "
                             f"RX {report.rx_hz:.0f} Hz · {detail}" + (f" · {report.error}" if report.error else ""))
        if report.error != self.last_error:
            if report.error:
                self.log(report.error)
            self.last_error = report.error
