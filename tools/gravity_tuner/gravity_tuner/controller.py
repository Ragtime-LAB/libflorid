"""Background gravity-compensation control loop on top of pyflorid.

Sends ``JointMIT`` frames with pure gravity feedforward torque every received
state frame:
    ``tau[i] = sign * kGravity * scale[i] * g_i(q, base_gravity, masses)``
with ``q = dq = kp = kd = 0`` and ``firmware_gravity = False`` (host-side
compensation). ``sign`` is the invert/reverse toggle, ``scale[i]`` is the
per-joint scale factor. The loop is owned by a dedicated thread so the GUI
stays responsive; parameters are shared under a lock.
"""

from __future__ import annotations

import threading
import time

import numpy as np

import pyflorid

_ERROR_BITS = {
    0: "joint position limits",
    1: "cartesian position limits",
    2: "self collision",
    3: "joint velocity",
    4: "cartesian velocity",
    5: "force control safety",
    6: "joint reflex",
    7: "cartesian reflex",
    8: "comm constraints",
    9: "E-stop",
    10: "watchdog timeout",
}

_HOLD_SEC = 0.5
_HOLD_KP = 50.0
_HOLD_KD = 5.0


def describe_errors(bits: int) -> str:
    """Human-readable description of the firmware error bitset."""
    s_names = [s_name for s_bit, s_name in _ERROR_BITS.items() if bits & (1 << s_bit)]
    if not s_names:
        return f"unknown error bits 0x{int(bits) & 0xFFFFFFFF:x}"
    return ", ".join(s_names)


class GravityCompController:
    """Owns the arm connection and the gravity-compensation control thread."""

    def __init__(self, model, *, k_gravity: float = 0.5) -> None:
        self._model = model
        self._lock = threading.Lock()
        self._k_gravity = float(k_gravity)
        self._reverse = False
        self._running = False
        self._arm = None
        self._ac = None
        self._thread: threading.Thread | None = None
        self._fault: str | None = None
        self._last = None  # (q, measured_tau, sent_tau), float32 copies

    # ---- connection --------------------------------------------------------

    def connect(self, uri: str):
        s_arm = pyflorid.Arm.create(uri)
        if s_arm is None:
            raise ValueError(f"cannot create Arm from URI {uri!r}")
        self._arm = s_arm
        return s_arm

    def disconnect(self) -> None:
        self.stop()
        if self._arm is not None:
            try:
                self._arm.stop()
            finally:
                self._arm = None
        self._ac = None

    @property
    def arm(self):
        return self._arm

    @property
    def is_connected(self) -> bool:
        return self._arm is not None

    @property
    def is_running(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    @property
    def fault(self) -> str | None:
        return self._fault

    @property
    def last_frame(self):
        """(q[6], measured_tau[6], sent_tau[6]) as float32 arrays, or None."""
        with self._lock:
            return self._last

    # ---- parameters --------------------------------------------------------

    def set_k_gravity(self, value: float) -> None:
        with self._lock:
            self._k_gravity = float(value)

    def set_mass(self, joint: int, mass_kg: float) -> None:
        self._model.set_mass(joint, mass_kg)

    def set_reverse(self, value: bool) -> None:
        with self._lock:
            self._reverse = bool(value)

    def reverse(self) -> bool:
        with self._lock:
            return self._reverse

    def set_joint_scale(self, joint: int, scale: float) -> None:
        self._model.set_scale(joint, scale)

    def joint_scale(self, joint: int) -> float:
        return self._model.scale(joint)

    # ---- control life cycle -------------------------------------------------

    def start(self) -> None:
        if self._arm is None:
            raise RuntimeError("not connected")
        if self.is_running:
            return
        self._fault = None
        self._arm.enable()
        self._ac = self._arm.start_joint_mit_control()
        self._running = True
        self._thread = threading.Thread(target=self._run, name="gravity-comp", daemon=True)
        self._thread.start()

    def start_with(self, ac) -> None:
        """Start with an injectable active-control handle (for tests)."""
        if self.is_running:
            return
        self._fault = None
        self._ac = ac
        self._running = True
        self._thread = threading.Thread(target=self._run, name="gravity-comp", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._running = False
        s_thread, self._thread = self._thread, None
        if s_thread is not None:
            s_thread.join(timeout=2.0)
        self._ac = None

    # ---- internals -----------------------------------------------------------

    def _run(self) -> None:
        s_hold_q = None
        s_hold_g = None
        try:
            while self._running:
                s_state = self._ac.read_once()
                if s_state.seq == 0:
                    # read_once() is non-blocking: an empty dequeue yields an
                    # all-zero ArmState (seq==0). Skip it and yield instead of
                    # spamming zero-torque frames as fast as the CPU allows.
                    time.sleep(0.001)
                    continue
                s_hold_q = np.asarray(s_state.q, dtype=np.float32).copy()
                s_hold_g = np.asarray(s_state.base_gravity, dtype=np.float32).copy()
                if s_state.errors:
                    self._fault = describe_errors(s_state.errors)
                    return
                self._send_gravity(s_state)
        except Exception as s_e:  # noqa: BLE001
            self._fault = f"control loop error: {s_e}"
            return
        finally:
            self._running = False
            if s_hold_q is not None and not self._fault:
                self._send_hold(s_hold_q, s_hold_g)

    def _feedforward(self, s_q, s_base_g) -> np.ndarray:
        """Full feedforward torque: tau = ±kGravity · joint_scale · g(q, base_gravity)."""
        with self._lock:
            s_k = self._k_gravity
            s_rev = self._reverse
        s_scale = self._model.scales()
        s_tau = self._model.gravity_torque(
            np.asarray(s_q, dtype=np.float32),
            np.asarray(s_base_g, dtype=np.float32),
            s_k,
        )
        s_tau = s_tau * s_scale
        if s_rev:
            s_tau = -s_tau
        return s_tau

    def _send_gravity(self, s_state) -> None:
        s_tau = self._feedforward(s_state.q, s_state.base_gravity)
        s_cmd = pyflorid.JointMIT()  # q/dq/kp/kd already 0
        s_cmd.tau = s_tau
        s_cmd.firmware_gravity = False
        self._ac.write_once(s_cmd)
        with self._lock:
            self._last = (
                np.asarray(s_state.q, dtype=np.float32).copy(),
                np.asarray(s_state.tau, dtype=np.float32).copy(),
                s_tau.copy(),
            )

    def _send_hold(self, s_q: np.ndarray, s_base_g: np.ndarray) -> None:
        """Brief mild-PD + gravity hold so the arm does not free-drop on stop."""
        s_tau = self._feedforward(s_q, s_base_g)
        s_cmd = pyflorid.JointMIT()
        s_cmd.q = s_q
        s_cmd.kp = np.full(6, _HOLD_KP, dtype=np.float32)
        s_cmd.kd = np.full(6, _HOLD_KD, dtype=np.float32)
        s_cmd.tau = s_tau
        s_cmd.firmware_gravity = False
        s_deadline = time.monotonic() + _HOLD_SEC
        while self._ac is not None and time.monotonic() < s_deadline:
            self._ac.write_once(s_cmd)
            time.sleep(0.005)