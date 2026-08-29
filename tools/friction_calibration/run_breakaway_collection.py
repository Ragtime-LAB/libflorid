#!/usr/bin/env python3
"""Guarded static breakaway torque experiment for J2-J6."""
from __future__ import annotations

import json
import hashlib
import os
import random
import sys
import time
from pathlib import Path

import numpy as np
import pyflorid

import run_friction_collection as base

ROOT = Path(__file__).resolve().parent
COLLECTOR_SCRIPT = Path(__file__).resolve()
OUTPUT_DIR = ROOT / "runs_breakaway"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "ENABLE WILLOW BREAKAWAY CALIBRATION"
REPEATS = 3
HELD_OUT_REPEAT = 2
RAMP_RATE_NM_S = np.array([0.0, 0.15, 0.15, 0.08, 0.05, 0.04])
MAX_RAMP_NM = np.array([0.0, 6.0, 6.0, 3.0, 2.0, 1.5])
BREAKAWAY_DQ_DEG_S = 0.5
BREAKAWAY_DISPLACEMENT_DEG = 0.5
CONSECUTIVE_FRAMES = 5
MAX_TRIAL_DURATION_S = 45.0
MAX_EXCURSION_DEG = 8.0
LIMIT_MARGIN_DEG = 3.0
LOWER_DEG = np.array([-170., 5., 5., -70., -85., -85.])
UPPER_DEG = np.array([170., 175., 175., 70., 85., 85.])
J2_PLATEAU_NM = 11.9
J2_PLATEAU_FRAMES = 10
MAX_RECOVERY_ATTEMPTS = 3
COMPATIBLE_PRE_RECOVERY_PROVENANCE = {
    (
        "3599b0e81a66eb6e88550ccedf955dc43ce1d51ae7f2c7beb1c7ae82677502f8",
        "7a483b919076412e2ce095a2a9991d2cb39b845894c75f630ad68e33946ca5c7",
        "87f63c0590d05baea263cdc42c6e1df2d063fa2def309ecb0283f3036bf33d46",
        "7a483b919076412e2ce095a2a9991d2cb39b845894c75f630ad68e33946ca5c7",
    ),
}


def breakaway_protocol_sha256():
    payload = {"base_protocol_sha256": base.protocol_sha256(), "repeats": REPEATS,
        "held_out_repeat": HELD_OUT_REPEAT, "ramp_rate_nm_s": RAMP_RATE_NM_S.tolist(),
        "max_ramp_nm": MAX_RAMP_NM.tolist(), "breakaway_dq_deg_s": BREAKAWAY_DQ_DEG_S,
        "breakaway_displacement_deg": BREAKAWAY_DISPLACEMENT_DEG,
        "consecutive_frames": CONSECUTIVE_FRAMES, "max_trial_duration_s": MAX_TRIAL_DURATION_S,
        "max_excursion_deg": MAX_EXCURSION_DEG, "limit_margin_deg": LIMIT_MARGIN_DEG}
    return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def trials():
    items = []
    for joint in base.ACTIVE_JOINTS:
        block = [(joint, direction, repeat) for direction in (-1, 1) for repeat in range(REPEATS)]
        training = [item for item in block if item[2] != base.HELD_OUT_REPEAT]
        heldout = [item for item in block if item[2] == base.HELD_OUT_REPEAT]
        random.Random(base.ORDER_SEED + 20 + joint).shuffle(training)
        random.Random(base.ORDER_SEED + 120 + joint).shuffle(heldout)
        items.extend(training + heldout)
    return items


def command(q, tau, active_joint):
    cmd = pyflorid.JointMIT(); kp = base.KP.copy(); kd = base.KD.copy()
    kp[active_joint] = 0.0; kd[active_joint] = 0.2
    cmd.q = np.asarray(q, dtype=np.float32); cmd.dq = np.zeros(6, dtype=np.float32)
    cmd.tau = np.asarray(tau, dtype=np.float32); cmd.kp = kp; cmd.kd = kd; cmd.firmware_gravity = False
    return cmd


def directed_motion_detected(displacement_deg, velocity_deg_s, direction):
    """Either independently directed displacement or velocity can start confirmation."""
    direction = 1 if direction > 0 else -1
    return (direction * velocity_deg_s >= BREAKAWAY_DQ_DEG_S or
        direction * displacement_deg >= BREAKAWAY_DISPLACEMENT_DEG)


def run_trial(control, model, data, joint, direction, repeat, j1):
    parks = base.VALIDATION_PARK_BY_JOINT_DEG if repeat == HELD_OUT_REPEAT else base.PARK_BY_JOINT_DEG
    support_posture = "heldout" if repeat == HELD_OUT_REPEAT else "train"
    park = np.deg2rad(parks[joint]); park[0] = j1
    base.move(control, model, data, park)
    state = base.read_valid(control); last_seq = int(state.seq); start_q = np.asarray(state.q, dtype=float)
    target = park.copy(); target[joint] = start_q[joint]
    begin = time.monotonic(); moving_frames = 0; plateau_frames = 0; peak = 0.0; detected = False; candidate = None
    applied_probe = 0.0
    samples = []
    while True:
        elapsed = time.monotonic() - begin
        ramp = min(RAMP_RATE_NM_S[joint] * elapsed, MAX_RAMP_NM[joint]) * direction
        state = base.read_valid(control, last_seq=last_seq); last_seq = int(state.seq)
        if int(state.errors): raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
        q = np.asarray(state.q, dtype=float); dq = np.asarray(state.dq, dtype=float); g = base.gravity(model, data, q)
        q_deg = np.rad2deg(q)
        if np.any(q_deg <= LOWER_DEG + LIMIT_MARGIN_DEG) or np.any(q_deg >= UPPER_DEG - LIMIT_MARGIN_DEG):
            raise RuntimeError(f"joint entered {LIMIT_MARGIN_DEG:g} deg software-limit margin: {np.round(q_deg, 2)}")
        if abs(q_deg[joint] - np.rad2deg(start_q[joint])) > MAX_EXCURSION_DEG:
            raise RuntimeError(f"J{joint + 1} exceeded {MAX_EXCURSION_DEG:g} deg breakaway excursion")
        if elapsed > MAX_TRIAL_DURATION_S:
            raise RuntimeError(f"J{joint + 1} breakaway trial exceeded {MAX_TRIAL_DURATION_S:g} s")
        plateau_frames = plateau_frames + 1 if abs(float(np.asarray(state.tau)[1])) >= J2_PLATEAU_NM else 0
        if plateau_frames >= J2_PLATEAU_FRAMES:
            raise RuntimeError(f"J2 feedback stayed above observed {J2_PLATEAU_NM:g} Nm plateau")
        displacement = float(np.rad2deg(q[joint] - start_q[joint])); velocity = float(np.rad2deg(dq[joint]))
        source_timestamp_us = int(getattr(state, "source_timestamp_us", 0))
        device_time_s = source_timestamp_us * 1e-6 if source_timestamp_us > 0 else float(state.time) * 1e-3
        peak = max(peak, abs(applied_probe)); samples.append({
            "elapsed_s": elapsed, "device_time_s": device_time_s,
            "seq": int(state.seq), "errors": int(state.errors),
            "ramp_nm": applied_probe, "next_ramp_nm": ramp,
            "displacement_deg": displacement, "velocity_deg_s": velocity,
            "q_rad": q.tolist(), "dq_rad_s": dq.tolist(),
            "tau_measured_nm": np.asarray(state.tau, dtype=float).tolist(),
            "gravity_nm": g.tolist(),
        })
        moving = directed_motion_detected(displacement, velocity, direction)
        if moving:
            if moving_frames == 0:
                candidate = {
                    "sample_index": len(samples) - 1,
                    "command_probe_tau_nm": float(applied_probe),
                    "measured_residual_tau_nm": float(np.asarray(state.tau, dtype=float)[joint] - g[joint]),
                }
            moving_frames += 1
        else:
            moving_frames = 0; candidate = None
        if moving_frames >= CONSECUTIVE_FRAMES:
            detected = True; break
        if abs(ramp) >= MAX_RAMP_NM[joint]: break
        tau = g.copy(); tau[joint] += ramp; control.write_once(command(target, tau, joint))
        applied_probe = float(ramp)
    selected_tau = None if candidate is None else candidate["measured_residual_tau_nm"]
    result = {"joint": joint + 1, "direction": direction, "repeat": repeat,
        "support_posture": support_posture, "detected": detected,
        "urdf_sha256": base.file_sha256(base.URDF), "protocol_sha256": breakaway_protocol_sha256(),
        "collector_script_sha256": base.file_sha256(COLLECTOR_SCRIPT),
        "base_collector_script_sha256": base.file_sha256(Path(base.__file__).resolve()),
        "breakaway_implementation_script_sha256": base.file_sha256(Path(__file__).resolve()),
        "schema_version": base.LOG_SCHEMA_VERSION, "device_time_unit": base.DEVICE_TIME_UNIT,
        "breakaway_tau_nm": selected_tau, "breakaway_candidate": candidate,
        "confirmation_peak_probe_tau_nm": direction * peak,
        "q_start_rad": float(start_q[joint]), "samples": samples}
    name = f"breakaway_J{joint + 1}_{direction:+d}_r{repeat}.json"; final = OUTPUT_DIR / name; temp = final.with_suffix(".json.tmp")
    temp.write_text(json.dumps(result, indent=2), encoding="utf-8")
    with temp.open("rb") as stream: os.fsync(stream.fileno())
    temp.replace(final)
    if hasattr(os, "O_DIRECTORY"):
        directory_fd = os.open(OUTPUT_DIR, os.O_RDONLY | os.O_DIRECTORY)
        try: os.fsync(directory_fd)
        finally: os.close(directory_fd)
    base.move(control, model, data, park)


def main():
    if "--print-protocol-sha256" in sys.argv:
        print(breakaway_protocol_sha256())
        return
    import pinocchio as pin
    model = pin.buildModelFromUrdf(str(base.URDF)); data = model.createData(); OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    completed = set()
    for path in OUTPUT_DIR.glob("breakaway_*.json"):
        item = json.loads(path.read_text(encoding="utf-8"))
        provenance = (item.get("protocol_sha256"), item.get("collector_script_sha256"),
            item.get("base_collector_script_sha256"), item.get("breakaway_implementation_script_sha256"))
        current_provenance = (breakaway_protocol_sha256(), base.file_sha256(COLLECTOR_SCRIPT),
            base.file_sha256(Path(base.__file__).resolve()), base.file_sha256(Path(__file__).resolve()))
        if ((provenance != current_provenance and provenance not in COMPATIBLE_PRE_RECOVERY_PROVENANCE) or
                item.get("urdf_sha256") != base.file_sha256(base.URDF) or
                str(item.get("schema_version")) != str(base.LOG_SCHEMA_VERSION) or
                item.get("device_time_unit") != base.DEVICE_TIME_UNIT):
            raise RuntimeError(f"stale/incompatible existing breakaway blocks resume; archive and recollect: {path}")
        if provenance != current_provenance:
            print(f"compatible pre-recovery breakaway accepted: {path.name}")
        completed.add((int(item["joint"]) - 1, int(item["direction"]), int(item["repeat"])))
    pending = [(j, d, r) for j, d, r in trials() if (j, d, r) not in completed]
    print(f"breakaway trials total={len(trials())}, complete={len(trials())-len(pending)}, pending={len(pending)}")
    print("active joint kp=0, kd=0.2; host gravity remains enabled")
    print(f"guards: timeout={MAX_TRIAL_DURATION_S:g}s, excursion={MAX_EXCURSION_DEG:g}deg, limit margin={LIMIT_MARGIN_DEG:g}deg, J2 plateau={J2_PLATEAU_NM:g}Nm")
    print("DISABLED preview:", not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE: return
    base.COLLISION_CONTEXT = base.build_collision_context(model)
    arm = pyflorid.Arm.create(base.DEVICE_URI)
    if arm is None: raise RuntimeError(f"Arm.create failed for {base.DEVICE_URI}")
    arm, initial = base.acquire_startup_state(arm); j1 = float(np.asarray(initial.q)[0])
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE: raise RuntimeError("confirmation mismatch")
    try:
        control = base.restart_mit_session(arm, model, data)
        for index, (joint, direction, repeat) in enumerate(pending, 1):
            print(f"[{index}/{len(pending)}] J{joint+1} direction={direction:+d} repeat={repeat+1}/{REPEATS}")
            recovery_attempt = 0
            while True:
                try:
                    run_trial(control, model, data, joint, direction, repeat, j1)
                    break
                except Exception as error:
                    if not base.recoverable_error(error):
                        raise
                    recovery_attempt += 1
                    if recovery_attempt > MAX_RECOVERY_ATTEMPTS:
                        raise RuntimeError(
                            f"breakaway J{joint+1} direction={direction:+d} recovery exhausted") from error
                    recovery_id = (joint + 1) * 100 + (direction + 1) * 10 + repeat
                    arm, control = base.recover_arm_session(
                        arm, model, data, recovery_id, recovery_attempt, error)
                    print("  retrying the same breakaway trial")
    finally:
        base.best_effort_disable(arm); print("All axes disabled.")


if __name__ == "__main__": main()
