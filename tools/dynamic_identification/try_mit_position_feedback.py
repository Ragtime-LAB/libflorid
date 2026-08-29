#!/usr/bin/env python3
"""Minimal real-arm MIT position/feedback diagnostic; no computed torque."""
from __future__ import annotations

import csv
import os
import sys
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid


ROOT = Path(__file__).resolve().parent
URDF = (
    ROOT.parent / "static_gravity_calibration/model/identified/"
    "Ragtime_Willow.dynamic-batch-3cb7a96993a3.urdf"
)
sys.path.insert(0, str(ROOT.parent / "friction_calibration"))
import run_gravity_friction_feedforward as friction_ff

DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW MIT POSITION DIAGNOSTIC"
KP = np.array([30., 60., 60., 50., 40., 18.], dtype=np.float32)
KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.])
RATE_HZ = 500.0
PERIOD_S = 1.0 / RATE_HZ
INITIAL_HOLD_S = 2.0
MOVE_S = 2.0
TARGET_HOLD_S = 2.0
TEST_JOINT = 4
TEST_DELTA_DEG = 5.0
PRINT_PERIOD_S = 0.1
LOG_DIR = ROOT / "runs_mit_position_diagnostic"


def read_valid(reader, timeout=2.0, last_seq=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) and (last_seq is None or int(state.seq) != int(last_seq)):
            return state
        time.sleep(0.0002)
    raise TimeoutError("no valid ArmState")


def checked_state(state):
    if int(state.errors):
        raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
    q = np.asarray(state.q, dtype=float)
    dq = np.asarray(state.dq, dtype=float)
    tau = np.asarray(state.tau, dtype=float)
    if any(x.shape != (6,) for x in (q, dq, tau)) or not np.all(np.isfinite(np.r_[q, dq, tau])):
        raise RuntimeError("non-finite or malformed ArmState")
    return q, dq, tau


def command(q, dq, tau):
    result = pyflorid.JointMIT()
    result.q = np.asarray(q, dtype=np.float32)
    result.dq = np.asarray(dq, dtype=np.float32)
    result.tau = np.asarray(tau, dtype=np.float32)
    result.kp = KP
    result.kd = KD
    result.firmware_gravity = False
    return result


def quintic(u):
    return 10*u**3 - 15*u**4 + 6*u**5


def quintic_d(u, duration):
    return (30*u**2 - 60*u**3 + 30*u**4) / duration


def main():
    if not URDF.is_file():
        raise FileNotFoundError(URDF)
    model = pin.buildModelFromUrdf(str(URDF))
    data = model.createData()
    models = friction_ff.load_friction_models()
    models[1] = models[2]
    breakaway_models = friction_ff.load_breakaway_models()
    breakaway_models[1] = breakaway_models[2]
    friction_ff.FRICTION_SCALE[0] = friction_ff.FRICTION_SCALE[1]
    friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[0] = friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[1]
    friction_ff.BREAKAWAY_SCALE[0] = friction_ff.BREAKAWAY_SCALE[1]

    print("MIT POSITION DIAGNOSTIC ONLY")
    print("test: hold measured pose -> J4 +5deg -> hold -> return")
    print("kp:", KP.tolist(), "kd:", KD.tolist())
    print("firmware_gravity=False; tau=host gravity+identified friction")
    print("DISABLED PREVIEW:", not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE:
        return

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    arm.automatic_error_recovery()
    time.sleep(1.0)
    before = read_valid(arm)
    before_q, before_dq, before_tau = checked_state(before)
    print("before session q_deg:", np.round(np.rad2deg(before_q), 3))
    print("before session dq_deg_s:", np.round(np.rad2deg(before_dq), 3))
    print("before session tau_Nm:", np.round(before_tau, 4))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")

    rows = []
    try:
        control = friction_ff.start_session(arm)
        after = read_valid(control)
        start, _, _ = checked_state(after)
        print("after session q_deg:", np.round(np.rad2deg(start), 3))
        print("session angle jump deg:", np.round(np.rad2deg(start - before_q), 3))
        target = start.copy()
        moved = start.copy()
        moved[TEST_JOINT - 1] += np.deg2rad(TEST_DELTA_DEG)
        phases = (
            ("hold_start", INITIAL_HOLD_S, start, start),
            ("move_out", MOVE_S, start, moved),
            ("hold_offset", TARGET_HOLD_S, moved, moved),
            ("move_back", MOVE_S, moved, start),
            ("hold_return", 1.0, start, start),
        )
        last_seq = int(after.seq)
        last_print = 0.0
        global_start = time.monotonic()
        for phase, duration, source, destination in phases:
            phase_start = time.monotonic()
            next_tick = phase_start
            while True:
                remaining = next_tick - time.monotonic()
                if remaining > 0:
                    time.sleep(remaining)
                next_tick += PERIOD_S
                elapsed = time.monotonic() - phase_start
                u = min(1.0, elapsed / duration)
                alpha = quintic(u) if source is not destination else u
                alpha_d = quintic_d(u, duration) if source is not destination else 0.0
                target = source + alpha * (destination - source)
                target_dq = alpha_d * (destination - source)
                state = read_valid(control, timeout=0.08, last_seq=last_seq)
                last_seq = int(state.seq)
                q, dq, tau_feedback = checked_state(state)
                gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
                friction = friction_ff.friction_torque(dq, models)
                breakaway = friction_ff.breakaway_torque(
                    dq, gravity, models, breakaway_models
                )
                tau_ff = gravity + friction + breakaway
                if np.any(np.abs(tau_ff) > TAU_LIMIT_NM):
                    raise RuntimeError(f"host feedforward exceeds configured limits: {tau_ff}")
                control.write_once(command(target, target_dq, tau_ff))
                error_deg = np.rad2deg(target - q)
                now = time.monotonic()
                if now - last_print >= PRINT_PERIOD_S:
                    print(
                        f"{phase:11s} t={elapsed:4.2f}s "
                        f"q_deg={np.round(np.rad2deg(q),2)} "
                        f"err_deg={np.round(error_deg,2)} "
                        f"dq_deg_s={np.round(np.rad2deg(dq),2)} "
                        f"tau_fb={np.round(tau_feedback,3)} "
                        f"tau_ff={np.round(tau_ff,3)}"
                    )
                    last_print = now
                rows.append((
                    now - global_start, phase, int(state.seq), int(state.errors),
                    *target, *target_dq, *q, *dq, *tau_feedback, *tau_ff,
                ))
                if u >= 1.0:
                    break

        LOG_DIR.mkdir(parents=True, exist_ok=True)
        output = LOG_DIR / f"mit_position_{time.strftime('%Y%m%dT%H%M%S')}.csv"
        columns = ["time_s", "phase", "seq", "errors"]
        for prefix in ("target_q", "target_dq", "q", "dq", "tau_feedback", "tau_ff"):
            columns.extend(f"{prefix}{joint}" for joint in range(1, 7))
        temporary = output.with_suffix(".csv.tmp")
        with temporary.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(columns)
            writer.writerows(rows)
            stream.flush()
            os.fsync(stream.fileno())
        temporary.replace(output)
        print("diagnostic saved+fsynced:", output)
    finally:
        friction_ff.best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
