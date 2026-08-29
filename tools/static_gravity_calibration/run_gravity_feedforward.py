"""Guarded MIT gravity feedforward plus small velocity damping."""

from __future__ import annotations

import hashlib
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
URDF = ROOT / "model" / "identified" / "Ragtime_Willow.static-mass-com-calibrated.urdf"
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True
SOFT_LIMITS_VALIDATED = False
CONFIRMATION_PHRASE = "ENABLE GRAVITY FEEDFORWARD"

# Hardware owner confirmed identity SDK <-> latest URDF mapping.
JOINT_INDEX = np.arange(6)
Q_SIGN = np.ones(6)
Q_OFFSET_RAD = np.zeros(6)
TAU_SIGN = np.ones(6)
J1_GRAVITY_ZERO = True
KD = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32)
# Confirmed hardware torque limits. These are final guards, not targets.
TAU_LIMIT_NM = np.array([40.0, 40.0, 40.0, 12.0, 12.0, 12.0], dtype=np.float32)
SOFT_LOWER_RAD = np.deg2rad([-180.0, 2.0, 2.0, -72.0, -87.0, -87.0])
SOFT_UPPER_RAD = np.deg2rad([180.0, 177.0, 177.0, 72.0, 87.0, 87.0])
RAMP_SECONDS = 5.0
STATE_TIMEOUT_S = 0.25
PRINT_PERIOD_S = 0.5


def valid_state(reader, timeout_s: float = 1.0):
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) != 0:
            return state
        time.sleep(0.0005)
    raise TimeoutError("no valid state received")


def make_command(tau: np.ndarray):
    command = pyflorid.JointMIT()
    command.q = np.zeros(6, dtype=np.float32)
    command.dq = np.zeros(6, dtype=np.float32)
    command.tau = np.asarray(tau, dtype=np.float32)
    command.kp = np.zeros(6, dtype=np.float32)
    command.kd = KD
    command.firmware_gravity = False
    return command


def main() -> None:
    if not URDF.is_file():
        raise FileNotFoundError(f"run fit_static_mass_and_export_urdf.py first: {URDF}")
    model = pin.buildModelFromUrdf(str(URDF))
    data = model.createData()
    print("URDF:", URDF)
    print("SHA256:", hashlib.sha256(URDF.read_bytes()).hexdigest())
    print("MIT: kp=0, dq_target=0, tau=gravity, small kd, firmware_gravity=False")
    print("KD:", KD.tolist())
    print("tau limits Nm:", TAU_LIMIT_NM.tolist())
    print("J1 gravity forced to zero:", J1_GRAVITY_ZERO)
    print("DISABLED preview:", not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE:
        return

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    initial = valid_state(arm, 2.0)
    initial_q_sdk = np.asarray(initial.q, dtype=float)
    initial_q_model = initial_q_sdk[JOINT_INDEX] * Q_SIGN + Q_OFFSET_RAD
    initial_g = np.asarray(pin.computeGeneralizedGravity(model, data, initial_q_model), dtype=float) * TAU_SIGN
    if J1_GRAVITY_ZERO:
        initial_g[0] = 0.0
    print("initial q deg:", np.round(np.rad2deg(initial_q_sdk), 3))
    print("initial Pinocchio g(q) Nm:", np.round(initial_g, 4))
    if np.any(np.abs(initial_g) > TAU_LIMIT_NM):
        raise RuntimeError("initial gravity torque exceeds configured torque limits")
    phrase = input(f'Type exactly "{CONFIRMATION_PHRASE}" while supporting the arm: ')
    if phrase != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch; hardware remains disabled")

    try:
        arm.enable()
        control = arm.start_joint_mit_control()
        if control is None:
            raise RuntimeError("start_joint_mit_control returned None")
        started = last_rx = last_print = time.monotonic()
        last_seq = None
        rx_count = tx_count = 0
        while True:
            state = control.read_once()
            now = time.monotonic()
            if int(state.seq) == 0:
                if now - last_rx > STATE_TIMEOUT_S:
                    raise TimeoutError("state watchdog expired")
                continue
            if last_seq != int(state.seq):
                last_rx, last_seq = now, int(state.seq)
                rx_count += 1
            if int(state.errors) != 0:
                raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
            q_sdk = np.asarray(state.q, dtype=float)
            dq_sdk = np.asarray(state.dq, dtype=float)
            if not np.all(np.isfinite(q_sdk)) or not np.all(np.isfinite(dq_sdk)):
                raise RuntimeError("non-finite q/dq")

            q_model = q_sdk[JOINT_INDEX] * Q_SIGN + Q_OFFSET_RAD
            gravity_model = np.asarray(pin.computeGeneralizedGravity(model, data, q_model), dtype=float)
            tau_sdk = gravity_model * TAU_SIGN
            if J1_GRAVITY_ZERO:
                tau_sdk[0] = 0.0
            alpha = min(1.0, (now - started) / RAMP_SECONDS)
            tau_sdk = np.clip(alpha * tau_sdk, -TAU_LIMIT_NM, TAU_LIMIT_NM)
            control.write_once(make_command(tau_sdk))
            tx_count += 1
            if now - last_print >= PRINT_PERIOD_S:
                elapsed = max(now - last_print, 1e-9)
                print(
                    "q_deg=", np.round(np.rad2deg(q_sdk), 2),
                    "g_tau_Nm=", np.round(tau_sdk, 3),
                    f"rx={rx_count / elapsed:.1f}Hz tx={tx_count / elapsed:.1f}Hz",
                )
                last_print, rx_count, tx_count = now, 0, 0
    finally:
        try:
            arm.disable()
            print("All axes disabled.")
        except Exception as error:
            print("CRITICAL disable failure:", error)


if __name__ == "__main__":
    main()
