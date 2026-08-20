"""tkinter GUI for gravity-compensation tuning.

Workflow: load a URDF (Pinocchio), connect to the arm (pyflorid), start the
gravity-compensation loop (``kGravity`` feedforward on top of the URDF model),
and tune each link mass live while watching the measured vs. computed torque.
"""

from __future__ import annotations

import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import numpy as np

from .controller import GravityCompController, describe_errors
from .model import GravityModel

_DEFAULT_URI = "usb:///dev/ttyACM0"
_KG_MIN, _KG_MAX, _KG_DEFAULT = 0.0, 2.0, 0.5
_MASS_MIN, _MASS_MAX = 0.0, 20.0
_SCALE_MIN, _SCALE_MAX, _SCALE_DEFAULT = -5.0, 5.0, 1.0

_JOINT_NAMES = ("J1", "J2", "J3", "J4", "J5", "J6")


class _JointRow:
    """One row of the mass/scale tuning table."""

    __slots__ = ("id", "name", "var", "spin", "scale_var", "scale_spin")

    def __init__(self, jid: int, name: str, var: tk.DoubleVar, spin: tk.Spinbox,
                 scale_var: tk.DoubleVar, scale_spin: tk.Spinbox) -> None:
        self.id = jid
        self.name = name
        self.var = var
        self.spin = spin
        self.scale_var = scale_var
        self.scale_spin = scale_spin


class GravityTunerApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        root.title("Gravity Compensation Tuner (libflorid)")
        root.geometry("860x560")

        self.model = GravityModel()
        self.controller = GravityCompController(self.model, k_gravity=_KG_DEFAULT)

        self.urdf_path = tk.StringVar()
        self.uri = tk.StringVar(value=_DEFAULT_URI)
        self.k_gravity = tk.DoubleVar(value=_KG_DEFAULT)
        self.reverse = tk.BooleanVar(value=False)
        self.status = tk.StringVar(value="load a URDF first")
        self.status_color = "#888"
        self.fault = tk.StringVar()
        self.conn_color = "#888"

        self._joint_rows: list[_JointRow] = []
        self._conn_light = None
        self._es_light = None
        self._state_labels: list[tk.Label] = []

        self._kinited = False
        self._minited = False

        self._build_widgets()
        self._kinited = True
        self._refresh()

    # ---- widgets -------------------------------------------------------------

    def _build_widgets(self) -> None:
        s_pad = {"padx": 6, "pady": 4}

        # URDF file
        s_file = ttk.LabelFrame(self.root, text="URDF model")
        s_file.pack(fill="x", padx=8, pady=6)
        ttk.Entry(s_file, textvariable=self.urdf_path).pack(side="left", fill="x", expand=True, **s_pad)
        ttk.Button(s_file, text="Browse...", command=self._browse_urdf).pack(side="left", **s_pad)
        ttk.Button(s_file, text="Load", command=self._load_urdf).pack(side="left", **s_pad)
        ttk.Button(s_file, text="Save", command=self._save_urdf).pack(side="left", **s_pad)
        ttk.Button(s_file, text="Save As...", command=self._save_urdf_as).pack(side="left", **s_pad)

        # Device
        s_dev = ttk.LabelFrame(self.root, text="Device")
        s_dev.pack(fill="x", padx=8, pady=6)
        ttk.Entry(s_dev, textvariable=self.uri).pack(side="left", fill="x", expand=True, **s_pad)
        self._conn_light = tk.Label(s_dev, text="\u25cf", fg="#888", width=2)
        self._conn_light.pack(side="left")
        ttk.Button(s_dev, text="Connect", command=self._connect).pack(side="left", **s_pad)
        ttk.Button(s_dev, text="Disconnect", command=self._disconnect).pack(side="left", **s_pad)

        # kGravity
        s_kg = ttk.LabelFrame(self.root, text="kGravity (global feedforward scale)")
        s_kg.pack(fill="x", padx=8, pady=6)
        self._kg_scale = ttk.Scale(
            s_kg, from_=_KG_MIN, to=_KG_MAX, variable=self.k_gravity,
            command=lambda _: self._kg_changed(),
        )
        self._kg_scale.pack(side="left", fill="x", expand=True, **s_pad)
        s_kg_spin = ttk.Spinbox(
            s_kg, from_=_KG_MIN, to=_KG_MAX, increment=0.01,
            textvariable=self.k_gravity, width=6,
            command=self._kg_changed,
        )
        s_kg_spin.bind("<Return>", lambda _e: self._kg_changed())
        s_kg_spin.pack(side="left", **s_pad)

        # Start / stop
        s_ctl = ttk.Frame(self.root)
        s_ctl.pack(fill="x", padx=8, pady=6)
        self._btn_start = ttk.Button(s_ctl, text="\u25b6 Start gravity compensation", command=self._start)
        self._btn_start.pack(side="left", **s_pad)
        self._btn_stop = ttk.Button(s_ctl, text="\u25a0 Stop", command=self._stop, state="disabled")
        self._btn_stop.pack(side="left", **s_pad)
        self._reverse_btn = ttk.Checkbutton(
            s_ctl, text="Invert gravity (\u2212)", variable=self.reverse, command=self._reverse_changed
        )
        self._reverse_btn.pack(side="left", padx=12)
        self._es_state = tk.Label(s_ctl, text="E-stop: --", fg="#888")
        self._es_state.pack(side="left", padx=16)

        # Mass table
        s_mass = ttk.LabelFrame(self.root, text="Link mass & per-joint scale tuning"
                                "\nmass changes affect proximal joints' gravity torque too; scale only its own joint")
        s_mass.pack(fill="both", expand=True, padx=8, pady=6)
        self._table = s_mass
        self._mass_header()

        # Live state + status
        s_live = ttk.LabelFrame(self.root, text="Live (q, measured tau, sent feedforward tau)")
        s_live.pack(fill="x", padx=8, pady=6)
        self._live_grid = ttk.Frame(s_live)
        self._live_grid.pack(fill="x")
        s_status = ttk.Frame(self.root)
        s_status.pack(fill="x", padx=8, pady=(0, 6))
        self._status_label = tk.Label(s_status, textvariable=self.status, fg="#888", anchor="w")
        self._status_label.pack(side="left", fill="x", expand=True)
        tk.Button(s_status, text="Clear fault", command=lambda: self.fault.set("")).pack(side="right")

    def _mass_header(self) -> None:
        for s_c, s_t in ((0, "Joint"), (1, "URDF link"), (2, "Mass [kg]"), (3, "Scale")):
            ttk.Label(self._table, text=s_t, font=("", 9, "bold")).grid(
                row=0, column=s_c, sticky="w", padx=10, pady=2
            )

    # ---- UI actions -----------------------------------------------------------

    def _browse_urdf(self) -> None:
        s_path = filedialog.askopenfilename(
            title="Select URDF", filetypes=[("URDF", "*.urdf"), ("All files", "*.*")]
        )
        if s_path:
            self.urdf_path.set(s_path)
            self._load_urdf()

    def _load_urdf(self) -> None:
        if not self.urdf_path.get().strip():
            self.status.set("choose a URDF file first")
            self.status_color = "#c00"
            self._sync_status()
            return
        try:
            self.model.load(self.urdf_path.get())
        except Exception as s_e:  # noqa: BLE001
            self.status.set(f"load failed: {s_e}")
            self.status_color = "#c00"
            self._sync_status()
            return
        self._build_mass_rows()
        self.status.set(f"loaded {self.model.dof}-DOF model from {self.urdf_path.get()}")
        self.status_color = "#090"
        self._sync_status()

    def _build_mass_rows(self) -> None:
        self._minited = False
        for s_child in self._table.winfo_children():
            s_child.destroy()
        self._joint_rows.clear()
        self._mass_header()
        for s_j in self.model.joints:
            s_row = s_j["id"]
            s_var = tk.DoubleVar(value=s_j["mass"])
            s_var.trace_add("write", lambda *_a, r=s_row: self._mass_changed(r))
            s_scale_var = tk.DoubleVar(value=_SCALE_DEFAULT)
            s_scale_var.trace_add("write", lambda *_a, r=s_row: self._scale_changed(r))
            ttk.Label(self._table, text=f"J{s_row}").grid(
                row=s_row, column=0, sticky="w", padx=10, pady=3
            )
            ttk.Label(self._table, text=s_j["name"]).grid(
                row=s_row, column=1, sticky="w", padx=10, pady=3
            )
            s_spin = ttk.Spinbox(
                self._table, from_=_MASS_MIN, to=_MASS_MAX, increment=0.05,
                textvariable=s_var, width=8,
            )
            s_spin.bind("<Return>", lambda _e, r=s_row: self._mass_changed(r))
            s_spin.grid(row=s_row, column=2, sticky="w", padx=10, pady=3)
            s_scale_spin = ttk.Spinbox(
                self._table, from_=_SCALE_MIN, to=_SCALE_MAX, increment=0.05,
                textvariable=s_scale_var, width=8,
            )
            s_scale_spin.bind("<Return>", lambda _e, r=s_row: self._scale_changed(r))
            s_scale_spin.grid(row=s_row, column=3, sticky="w", padx=10, pady=3)
            self._joint_rows.append(_JointRow(s_row, s_j["name"], s_var, s_spin, s_scale_var, s_scale_spin))
        self._minited = True

    def _kg_changed(self) -> None:
        if self._kinited:
            self.controller.set_k_gravity(self.k_gravity.get())

    def _reverse_changed(self) -> None:
        if self._kinited:
            self.controller.set_reverse(self.reverse.get())

    def _mass_changed(self, row: int) -> None:
        if not self._minited:
            return
        try:
            s_kg = float(self._joint_rows[row - 1].var.get())
        except (tk.TclError, ValueError):
            return
        try:
            self.controller.set_mass(row, s_kg)
        except ValueError as s_e:
            self.status.set(f"mass J{row}: {s_e}")
            self.status_color = "#c00"
            self._sync_status()

    def _scale_changed(self, row: int) -> None:
        if not self._minited:
            return
        try:
            s_scale = float(self._joint_rows[row - 1].scale_var.get())
        except (tk.TclError, ValueError):
            return
        self.controller.set_joint_scale(row, s_scale)

    def _save_urdf(self) -> None:
        try:
            s_path = self.model.save()
        except Exception as s_e:  # noqa: BLE001
            self.status.set(f"save failed: {s_e}")
            self.status_color = "#c00"
            self._sync_status()
            return
        self.status.set(f"saved to {s_path}")
        self.status_color = "#090"
        self._sync_status()

    def _save_urdf_as(self) -> None:
        s_path = filedialog.asksaveasfilename(
            title="Save URDF as", defaultextension=".urdf",
            filetypes=[("URDF", "*.urdf"), ("All files", "*.*")],
        )
        if not s_path:
            return
        try:
            s_path = self.model.save(s_path)
        except Exception as s_e:  # noqa: BLE001
            self.status.set(f"save failed: {s_e}")
            self.status_color = "#c00"
            self._sync_status()
            return
        self.urdf_path.set(s_path)
        self.status.set(f"saved to {s_path}")
        self.status_color = "#090"
        self._sync_status()

    def _connect(self) -> None:
        try:
            self.controller.connect(self.uri.get().strip())
        except Exception as s_e:  # noqa: BLE001
            self.status.set(f"connect failed: {s_e}")
            self.status_color = "#c00"
        else:
            self.status.set(f"connected: {self.uri.get().strip()}")
            self.status_color = "#090"
        self._sync_status()

    def _disconnect(self) -> None:
        self.controller.disconnect()
        self.status.set("disconnected")
        self.status_color = "#888"
        self._sync_status()

    def _start(self) -> None:
        if not self.controller.is_connected:
            self.status.set("connect to the device first")
            self.status_color = "#c00"
            self._sync_status()
            return
        if not self.model.joints:
            self.status.set("load a URDF first")
            self.status_color = "#c00"
            self._sync_status()
            return
        try:
            self.controller.start()
        except Exception as s_e:  # noqa: BLE001
            self.status.set(f"start failed: {s_e}")
            self.status_color = "#c00"
            self._sync_status()
            return
        self.status.set(
            f"gravity compensation running, kGravity={self.k_gravity.get():.2f} "
            "\u2014 keep your hand on the E-stop"
        )
        self.status_color = "#090"
        self._sync_status()

    def _stop(self) -> None:
        self.controller.stop()
        self.status.set("stopped (brief hold applied)")
        self.status_color = "#888"
        self._sync_status()

    # ---- periodic refresh -------------------------------------------------------

    def _refresh(self) -> None:
        s_running = self.controller.is_running
        self._btn_start.configure(state="normal" if (self.controller.is_connected and not s_running) else "disabled")
        self._btn_stop.configure(state="normal" if s_running else "disabled")

        s_fault = self.controller.fault or self.fault.get()
        if self._es_light_error(s_fault):
            s_fault_msg = f"fault: {s_fault}" if self.controller.fault else "E-stop / error detected"
            self.status.set(s_fault_msg)
            self.status_color = "#c00"

        self._update_live()
        self._update_es_light(s_running)
        self._es_state.configure(
            text=f"loop: {'running' if s_running else 'idle'}  |  E-stop: {self._es_label()}"
        )
        self.root.after(100, self._refresh)

    def _es_light_error(self, s_fault: str) -> bool:
        return bool(s_fault) and ("E-stop" in s_fault or "error" in s_fault or "fault" in s_fault)

    def _es_label(self) -> str:
        s_fault = self.controller.fault or self.fault.get()
        if s_fault and "E-stop" in s_fault:
            return "\u2717 (E-stop bitset)"
        if s_fault:
            return "\u2020 (fault)"
        return "\u2713" if self.controller.is_running else "--"

    def _update_es_light(self, s_running: bool) -> None:
        if self._es_light_error(self.controller.fault or self.fault.get()):
            self._es_state.configure(fg="#c00")
        elif s_running:
            self._es_state.configure(fg="#090")
        else:
            self._es_state.configure(fg="#888")

    def _update_live(self) -> None:
        s_frame = self.controller.last_frame
        if s_frame is None:
            if not self._state_labels:
                return
            for s_lab in self._state_labels:
                s_lab.configure(text="--")
            return
        s_q, s_meas, s_sent = s_frame
        if not self._state_labels:
            self._build_live_labels(len(s_q))
        for s_i, s_lab in enumerate(self._state_labels):
            s_lab.configure(text=f"{_JOINT_NAMES[s_i] if s_i < len(_JOINT_NAMES) else s_i + 1}: "
                                 f"q={s_q[s_i]:+.2f}  t_meas={s_meas[s_i]:+6.2f}  t_ff={s_sent[s_i]:+6.2f}")

    def _build_live_labels(self, s_n: int) -> None:
        for s_i in range(s_n):
            s_lab = ttk.Label(self._live_grid, text="--", font=("TkFixedFont", 9))
            s_lab.grid(row=0, column=s_i, padx=12, pady=2)
            self._state_labels.append(s_lab)

    def _sync_status(self) -> None:
        self._status_label.configure(fg=self.status_color)

    def on_close(self) -> None:
        try:
            self.controller.stop()
        finally:
            self.root.destroy()