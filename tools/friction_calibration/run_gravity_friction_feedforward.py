"""Willow MIT gravity + identified friction feedforward, with no position stiffness."""

from __future__ import annotations

import hashlib
import json
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
URDF = ROOT.parent / "static_gravity_calibration" / "model" / "identified" / "Ragtime_Willow.static-mass-com-calibrated.urdf"
FRICTION_FIT = ROOT / "friction_fit.json"
DEVICE_URI = "usb:///dev/ttyACM0"

ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "ENABLE GRAVITY AND FRICTION FEEDFORWARD"
# Per-axis scale J1..J6. J1 has no fitted model and remains zero.
FRICTION_SCALE = np.array([0.0, 0.6, 0.62, 0.55, 0.6, 0.6], dtype=float)
# Optional hand-tuned viscous addition [Nm/(rad/s)] after scaling the fitted
# model. Start with J3 only; increase gradually if fast motion still feels
# heavier than slow motion. This is separate from the identified parameters.
EXTRA_VISCOUS_NM_PER_RAD_S = np.array(
    [0.0, 0.0245, 0.028, 0.0035, 0.0035, 0.0035], dtype=float
)
FRICTION_DIRECTION_VEL_RAD_S = 0.01 # smooth Coulomb sign around zero velocity
J1_GRAVITY_ZERO = True
KD = np.zeros(6, dtype=np.float32)
TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.], dtype=float)
RAMP_SECONDS = 3.0
STATE_TIMEOUT_S = 0.25
PRINT_PERIOD_S = 0.5
MAX_RECOVERY_ATTEMPTS = 3
RECONNECT_ATTEMPTS = 15
RECONNECT_DELAY_S = 2.0


def load_friction_models():
    payload = json.loads(FRICTION_FIT.read_text(encoding="utf-8"))
    if payload.get("urdf_sha256") != hashlib.sha256(URDF.read_bytes()).hexdigest():
        raise RuntimeError("friction fit and gravity URDF provenance do not match")
    result = {}
    for name, item in payload.get("joints", {}).items():
        joint = int(name[1:])
        selected = item["best_by_training_grouped_cv"]
        result[joint] = (selected, item["models"][selected]["parameters"])
    if set(result) != {2, 3, 4, 5, 6}:
        raise RuntimeError(f"expected fitted J2..J6, got {sorted(result)}")
    return result


def friction_torque(dq, models):
    result = np.zeros(6, dtype=float)
    for joint, (model, p) in models.items():
        velocity = float(dq[joint - 1])
        direction = float(np.tanh(velocity / FRICTION_DIRECTION_VEL_RAD_S))
        if model == "symmetric":
            value = p["fv_nm_per_rad_s"] * velocity + p["fc_nm"] * direction + p["offset_nm"]
        elif model == "asymmetric":
            viscous = p["fv_pos"] * velocity if direction >= 0 else p["fv_neg"] * velocity
            coulomb = (p["fc_pos"] if direction >= 0 else p["fc_neg"]) * direction
            value = viscous + coulomb + p["offset_nm"]
        elif model == "stribeck":
            magnitude = p["fc_nm"] + (p["fs_nm"] - p["fc_nm"]) * np.exp(
                -(abs(velocity) / p["vs_rad_s"]) ** 2)
            value = p["fv_nm_per_rad_s"] * velocity + direction * magnitude + p["offset_nm"]
        else:
            raise RuntimeError(f"unsupported friction model for J{joint}: {model}")
        result[joint - 1] = FRICTION_SCALE[joint - 1] * value
    result[0] = 0.0
    result += EXTRA_VISCOUS_NM_PER_RAD_S * np.asarray(dq, dtype=float)
    return result


def read_valid(reader, timeout_s=2.0, last_seq=None):
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) and (last_seq is None or int(state.seq) != int(last_seq)):
            return state
        time.sleep(0.0005)
    raise TimeoutError("no valid ArmState")


def command(tau):
    result = pyflorid.JointMIT()
    result.q = np.zeros(6, dtype=np.float32)
    result.dq = np.zeros(6, dtype=np.float32)
    result.tau = np.asarray(tau, dtype=np.float32)
    result.kp = np.zeros(6, dtype=np.float32)
    result.kd = KD
    result.firmware_gravity = False
    return result


def best_effort_disable(arm):
    if arm is None:
        return
    try:
        arm.disable()
    except Exception as error:
        print("disable failed:", error)


def start_session(arm):
    arm.automatic_error_recovery()
    time.sleep(1.0)
    state = read_valid(arm, 2.0)
    if int(state.errors):
        raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
    arm.enable()
    control = arm.start_joint_mit_control()
    if control is None:
        raise RuntimeError("start_joint_mit_control returned None")
    return control


def recover_session(arm, error):
    print("recovering control session:", error)
    best_effort_disable(arm)
    time.sleep(1.0)
    try:
        return arm, start_session(arm)
    except Exception as same_error:
        print("same-session recovery failed:", same_error)
    last_error = None
    for attempt in range(1, RECONNECT_ATTEMPTS + 1):
        replacement = None
        try:
            replacement = pyflorid.Arm.create(DEVICE_URI)
            if replacement is None:
                raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
            control = start_session(replacement)
            print(f"USB reconnected on attempt {attempt}")
            return replacement, control
        except Exception as reconnect_error:
            last_error = reconnect_error
            best_effort_disable(replacement)
            print(f"reconnect {attempt}/{RECONNECT_ATTEMPTS} failed: {reconnect_error}")
            time.sleep(RECONNECT_DELAY_S)
    raise RuntimeError(f"automatic reconnect exhausted: {last_error}") from last_error


def recoverable(error):
    text = str(error).lower()
    return isinstance(error, (TimeoutError, OSError, IOError)) or any(token in text for token in (
        "firmware errors=", "usb", "input/output", "no valid armstate", "timeout", "disconnected"))


def main():
    if not URDF.is_file() or not FRICTION_FIT.is_file():
        raise FileNotFoundError("identified URDF or friction_fit.json is missing")
    model = pin.buildModelFromUrdf(str(URDF)); data = model.createData()
    models = load_friction_models()
    print("URDF:", URDF)
    print("friction fit:", FRICTION_FIT)
    print("models:", {f"J{j}": name for j, (name, _) in models.items()})
    print("MIT: kp=0, kd=", KD.tolist(), "firmware_gravity=False")
    print("per-axis friction scale J1..J6:", FRICTION_SCALE.tolist())
    print("extra viscous Nm/(rad/s) J1..J6:", EXTRA_VISCOUS_NM_PER_RAD_S.tolist())
    print("tau = Pinocchio gravity + per-axis scaled identified friction")
    print("No commanded motion: this test only reacts to measured q and dq.")
    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY")
        return

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    arm.automatic_error_recovery(); time.sleep(1.0)
    initial = read_valid(arm, 2.0)
    print("initial q_deg:", np.round(np.rad2deg(np.asarray(initial.q, dtype=float)), 3))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}" while supporting the arm: ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch; hardware remains disabled")

    recovery_count = 0
    try:
        control = start_session(arm)
        ramp_start = last_rx = last_print = time.monotonic()
        last_seq = None; rx_count = tx_count = 0
        while True:
            try:
                state = read_valid(control, STATE_TIMEOUT_S, last_seq)
                now = time.monotonic(); last_seq = int(state.seq); last_rx = now; rx_count += 1
                if int(state.errors):
                    raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
                q = np.asarray(state.q, dtype=float); dq = np.asarray(state.dq, dtype=float)
                if q.shape != (6,) or dq.shape != (6,) or not np.all(np.isfinite(np.r_[q, dq])):
                    raise RuntimeError("non-finite or malformed q/dq")
                gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
                if J1_GRAVITY_ZERO:
                    gravity[0] = 0.0
                friction = friction_torque(dq, models)
                requested = gravity + friction
                if np.any(np.abs(requested) > TAU_LIMIT_NM):
                    raise RuntimeError(f"feedforward exceeds hardware torque limits: {requested}")
                alpha = min(1.0, (now - ramp_start) / RAMP_SECONDS)
                sent = alpha * requested
                control.write_once(command(sent)); tx_count += 1
                recovery_count = 0
                if now - last_print >= PRINT_PERIOD_S:
                    elapsed = max(now - last_print, 1e-9)
                    print("q_deg=", np.round(np.rad2deg(q), 2),
                          "dq=", np.round(dq, 3),
                          "g=", np.round(gravity, 3),
                          "fric=", np.round(friction, 3),
                          "tau=", np.round(sent, 3),
                          f"rx={rx_count/elapsed:.1f}Hz tx={tx_count/elapsed:.1f}Hz")
                    last_print = now; rx_count = tx_count = 0
            except Exception as error:
                if not recoverable(error):
                    raise
                recovery_count += 1
                if recovery_count > MAX_RECOVERY_ATTEMPTS:
                    raise RuntimeError("runtime recovery exhausted") from error
                arm, control = recover_session(arm, error)
                ramp_start = last_rx = last_print = time.monotonic()
                last_seq = None; rx_count = tx_count = 0
    finally:
        best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
