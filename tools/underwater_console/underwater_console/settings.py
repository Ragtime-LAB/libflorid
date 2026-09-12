"""Editable device tuning values; firmware defaults are read, never invented."""
import math
import struct
import tkinter as tk
from tkinter import messagebox, ttk

# (label, SDK field, start index, cell count); None means a scalar field.
GROUPS = (
    ("遥操映射", (
        ("角度比例", "angle_scale", 0, 5),
        ("角度偏置 / rad", "angle_offset_rad", 0, 5),
        ("Kp / Nm/rad", "kp", 0, 5),
        ("Kd / Nm·s/rad", "kd", 0, 5),
    )),
    ("重力", (
        ("主臂比例", "gravity_scale", 0, 4),
        ("从臂比例", "gravity_scale", 4, 4),
    )),
    ("主臂摩擦", tuple((label, field, 0, 5) for label, field in (
        ("正向库仑 / Nm", "friction_coulomb_positive_nm"),
        ("反向库仑 / Nm", "friction_coulomb_negative_nm"),
        ("黏性 / Nm·s/rad", "friction_viscous_nm_per_rad_s"),
        ("速度死区 / rad/s", "friction_deadband_rad_s"),
        ("过渡速度 / rad/s", "friction_transition_rad_s"),
        ("滤波时间常数 / s", "friction_filter_time_constant_s"),
    ))),
    ("从臂摩擦", tuple((label, field, 5, 5) for label, field in (
        ("正向库仑 / Nm", "friction_coulomb_positive_nm"),
        ("反向库仑 / Nm", "friction_coulomb_negative_nm"),
        ("黏性 / Nm·s/rad", "friction_viscous_nm_per_rad_s"),
        ("速度死区 / rad/s", "friction_deadband_rad_s"),
        ("过渡速度 / rad/s", "friction_transition_rad_s"),
        ("滤波时间常数 / s", "friction_filter_time_constant_s"),
    ))),
    ("力反馈", (
        ("高负载增益", "force_gain", 0, 5),
        ("低负载增益", "force_low_gain", 0, 5),
        ("速度滤波 / s", "force_velocity_filter_s", 0, 5),
        ("负载滤波 / s", "force_load_filter_s", 0, 5),
        ("低负载阈值 / Nm", "force_low_load_nm", None, 1),
        ("高负载阈值 / Nm", "force_high_load_nm", None, 1),
    )),
)


def format_float32(value):
    """Shortest significant-digit display that preserves the device float bits."""
    encoded = struct.pack("<f", value)
    for precision in range(1, 10):
        text = format(value, f".{precision}g")
        try:
            if struct.pack("<f", float(text)) == encoded:
                return text
        except OverflowError:
            pass
    return format(value, ".9g")


def parse_settings(sdk, cells, buzzer_alarm_enabled=True):
    """Return a complete SDK DTO; preserve signed values without clipping."""
    settings = sdk.UnderwaterSettings()
    settings.buzzer_alarm_enabled = buzzer_alarm_enabled
    arrays = {}
    for (field, index), (variable, label) in cells.items():
        try:
            value = float(variable.get())
        except ValueError as exc:
            raise ValueError(f"{label}：请输入数值") from exc
        if not math.isfinite(value):
            raise ValueError(f"{label}：必须为有限数值")
        try:
            struct.pack("<f", value)
        except OverflowError as exc:
            raise ValueError(f"{label}：超出设备 float32 数值范围") from exc
        if index is None:
            setattr(settings, field, value)
        else:
            if field not in arrays:
                arrays[field] = list(getattr(settings, field))
            arrays[field][index] = value
    for field, values in arrays.items():
        setattr(settings, field, values)
    return settings


class SettingsPage(ttk.Frame):
    def __init__(self, parent, controller, log):
        super().__init__(parent, padding=12)
        self.controller = controller
        self.log = log
        self.cells = {}
        self.buzzer_alarm_enabled = tk.BooleanVar(value=True)
        self.source = tk.StringVar(value="连接后读取设备设置。")
        self.feedback = tk.StringVar(value="")
        actions = ttk.Frame(self)
        actions.pack(fill="x")
        ttk.Button(actions, text="读取设备", command=lambda: self.read("settings")).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="恢复默认到表格", command=lambda: self.read("default_settings")).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="应用并保存", command=self.apply).pack(side="left")
        ttk.Label(self, textvariable=self.source).pack(anchor="w", pady=(8, 4))
        ttk.Label(self, textvariable=self.feedback, wraplength=830).pack(anchor="w", pady=(0, 6))
        ttk.Checkbutton(self, text="电机掉线蜂鸣报警", variable=self.buzzer_alarm_enabled).pack(anchor="w", pady=(0, 8))
        tabs = ttk.Notebook(self)
        tabs.pack(fill="both", expand=True)
        for group, rows in GROUPS:
            panel = ttk.Frame(tabs, padding=(12, 8))
            tabs.add(panel, text=f" {group} ")
            viewport = tk.Canvas(panel, background="#f2f5f8", highlightthickness=0, height=180)
            scrollbar = ttk.Scrollbar(panel, orient="vertical", command=viewport.yview)
            viewport.configure(yscrollcommand=scrollbar.set)
            scrollbar.pack(side="right", fill="y")
            viewport.pack(side="left", fill="both", expand=True)
            form = ttk.Frame(viewport)
            content = viewport.create_window((0, 0), window=form, anchor="nw")
            form.bind("<Configure>", lambda event, view=viewport: view.configure(scrollregion=view.bbox("all")))
            viewport.bind("<Configure>", lambda event, view=viewport, item=content: view.itemconfigure(item, width=event.width))
            for column, title in enumerate(("参数", "joint1", "joint2", "joint3", "joint4", "夹爪")):
                ttk.Label(form, text=title).grid(row=0, column=column, padx=5, pady=(0, 6), sticky="w")
            for row, (label, field, start, count) in enumerate(rows, 1):
                ttk.Label(form, text=label).grid(row=row, column=0, padx=(0, 12), pady=4, sticky="w")
                for column in range(count):
                    index = None if start is None else start + column
                    variable = tk.StringVar()
                    self.cells[field, index] = (variable, f"{group} / {label} / {column + 1}")
                    entry = ttk.Entry(form, textvariable=variable, width=10)
                    entry.grid(row=row, column=column + 1, padx=4, pady=4, sticky="ew")
            for column in range(1, 6):
                form.columnconfigure(column, weight=1)
        ttk.Label(self, text="对齐轨迹、零点/轴方向和已辨识自负载系数保留在固件中。", wraplength=830).pack(anchor="w", pady=(8, 0))

    def reset(self):
        self.buzzer_alarm_enabled.set(True)
        self.source.set("连接后读取设备设置。")
        self.feedback.set("")
        for variable, _ in self.cells.values():
            variable.set("")

    def read(self, method):
        self.feedback.set("正在读取…")
        self.controller.command(method)

    def update_result(self, result, action):
        status = getattr(result.error, "name", str(result.error))
        message = f"{action}: {status} status={result.device_status}"
        if result.reason:
            message += f" · {result.reason}"
        self.feedback.set(message)
        self.log(message)
        snapshot = result.snapshot
        if snapshot is not None:
            source = getattr(snapshot.source, "name", str(snapshot.source))
            prefix = "显示默认值（尚未写入）" if action == "default_settings" else "设备参数"
            self.source.set(f"{prefix} · revision={snapshot.revision} · source={source} · storage_error={snapshot.storage_error}")
        # Rejected writes retain the user's edits. The response still shows
        # the device's revision/storage reason above.
        if result and snapshot is not None:
            self.buzzer_alarm_enabled.set(snapshot.settings.buzzer_alarm_enabled)
            for (field, index), (variable, _) in self.cells.items():
                value = getattr(snapshot.settings, field)
                variable.set(format_float32(value if index is None else value[index]))

    def apply(self):
        try:
            settings = parse_settings(self.controller.sdk, self.cells, self.buzzer_alarm_enabled.get())
        except (ValueError, TypeError) as exc:
            self.feedback.set(str(exc))
            self.log(str(exc))
            return
        if not messagebox.askokcancel("失能并保存设备设置",
                "即将让两臂进入 Damp 并失能。\n确认本次在线电机失能后，参数将保存到 Flash 并生效。\n\n继续吗？",
                parent=self.winfo_toplevel()):
            self.feedback.set("已取消；未发送失能或设置请求。")
            return
        self.feedback.set("正在请求失能，随后保存设置…")
        self.controller.apply_settings(settings)
