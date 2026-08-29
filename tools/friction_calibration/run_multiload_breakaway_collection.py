#!/usr/bin/env python3
"""Load-conditioned 2-degree Breakaway collection for Willow J2-J4."""

from __future__ import annotations

import hashlib
import json
import random
import sys
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

import run_breakaway_collection as experiment
import run_friction_collection as base

ROOT = Path(__file__).resolve().parent
ENTRY_SCRIPT = Path(__file__).resolve()
OUTPUT_DIR = ROOT / "runs_breakaway_multiload_10deg"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "ENABLE WILLOW 10 DEG MULTILOAD BREAKAWAY"
REPEATS = 5
ORDER_SEED = 20260829
MAX_RECOVERY_ATTEMPTS = 3

# Visible-motion confirmation requested by the hardware owner. The candidate
# static torque is still latched at first motion; the trial only finishes after
# ten degrees of directed travel.
experiment.BREAKAWAY_DISPLACEMENT_DEG = 10.0
experiment.MAX_EXCURSION_DEG = 15.0
experiment.MAX_RAMP_NM = base.TAU_LIMIT_NM.copy()

# Candidate q order is J1..J6 in degrees. J1 is replaced with measured startup
# encoder angle. These nine configurations are selected from the audited
# J2={30,70,110}, J3={50,90,130}, J4={-40,0,40} grid to span low/medium/high
# Pinocchio gravity load for the active joint.
LOAD_POSTURES_DEG = {
    1: {
        "low":  np.array([0., 70., 130.,  40., 0., 0.]),
        "mid":  np.array([0., 30.,  90.,   0., 0., 0.]),
        "high": np.array([0., 30., 130., -40., 0., 0.]),
    },
    2: {
        "low":  np.array([0.,  70., 130., -40., 0., 0.]),
        "mid":  np.array([0., 110.,  50.,   0., 0., 0.]),
        "high": np.array([0., 110.,  90.,   0., 0., 0.]),
    },
    3: {
        "low":  np.array([0., 110., 50., 40., 0., 0.]),
        "mid":  np.array([0.,  70.,130., 40., 0., 0.]),
        "high": np.array([0., 110., 90.,  0., 0., 0.]),
    },
}


def protocol_sha256():
    payload = {
        "base_breakaway_protocol": experiment.breakaway_protocol_sha256(),
        "load_postures_deg": {str(j): {k: v.tolist() for k, v in levels.items()}
            for j, levels in LOAD_POSTURES_DEG.items()},
        "repeats": REPEATS, "order_seed": ORDER_SEED,
        "torque_limits_nm": base.TAU_LIMIT_NM.tolist(),
    }
    return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def trials():
    result = []
    trial_id = 0
    for joint, levels in LOAD_POSTURES_DEG.items():
        block = []
        for level, posture in levels.items():
            for direction in (-1, 1):
                for repeat in range(REPEATS):
                    block.append((trial_id, joint, level, posture, direction, repeat))
                    trial_id += 1
        random.Random(ORDER_SEED + joint).shuffle(block)
        result.extend(block)
    return result


def completed_keys():
    completed = set()
    for path in OUTPUT_DIR.glob("breakaway_multiload_*.json"):
        item = json.loads(path.read_text(encoding="utf-8"))
        if (item.get("protocol_sha256") != protocol_sha256() or
                item.get("urdf_sha256") != base.file_sha256(base.URDF) or
                item.get("collector_script_sha256") != base.file_sha256(ENTRY_SCRIPT)):
            # Keep the old artifact for diagnosis, but do not count a trial
            # collected under an earlier control protocol as completed.  The
            # pending trial will be rerun and atomically replace its filename
            # only after a successful capture under the current protocol.
            print(f"WARNING: ignoring incompatible resume file; trial will rerun: {path}")
            continue
        completed.add((int(item["joint"]) - 1, item["load_level"],
            int(item["direction"]), int(item["repeat"])))
    return completed


def main():
    if "--print-protocol-sha256" in sys.argv:
        print(protocol_sha256()); return
    model = pin.buildModelFromUrdf(str(base.URDF)); data = model.createData()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    completed = completed_keys()
    pending = [x for x in trials() if (x[1], x[2], x[4], x[5]) not in completed]
    print(f"multiload breakaway total={len(trials())}, complete={len(trials())-len(pending)}, pending={len(pending)}")
    print(f"active joints: J2,J3,J4; load levels: low,mid,high; directions: +/-; repeats={REPEATS}")
    print("visible confirmation displacement:", experiment.BREAKAWAY_DISPLACEMENT_DEG, "deg")
    print("host torque boundaries J1..J6 Nm:", base.TAU_LIMIT_NM.tolist())
    for _, joint, level, posture, direction, repeat in pending:
        q = np.deg2rad(posture)
        load = abs(float(pin.computeGeneralizedGravity(model, data, q)[joint]))
        print(f"  J{joint+1} {level:4s} direction={direction:+d} repeat={repeat+1}/{REPEATS} "
              f"q234={posture[1:4].tolist()} expected_|g|={load:.3f}Nm")
    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY"); return

    base.COLLISION_CONTEXT = base.build_collision_context(model)
    arm = pyflorid.Arm.create(base.DEVICE_URI)
    if arm is None: raise RuntimeError(f"Arm.create failed for {base.DEVICE_URI}")
    arm, initial = base.acquire_startup_state(arm); j1 = float(np.asarray(initial.q)[0])
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")
    try:
        control = base.restart_mit_session(arm, model, data)
        if pending:
            first_posture = np.deg2rad(np.asarray(pending[0][3], dtype=float))
            first_posture[0] = j1
            startup_recovery = 0
            while True:
                try:
                    base.soft_start_move(control, model, data, first_posture)
                    break
                except Exception as error:
                    if not base.recoverable_error(error):
                        raise
                    startup_recovery += 1
                    if startup_recovery > MAX_RECOVERY_ATTEMPTS:
                        raise RuntimeError("soft-start recovery exhausted") from error
                    print(f"  soft-start recoverable failure; clearing fault "
                          f"{startup_recovery}/{MAX_RECOVERY_ATTEMPTS}: {error}")
                    arm, control = base.recover_arm_session(
                        arm, model, data, 900000, startup_recovery, error)
        for index, (_, joint, level, posture, direction, repeat) in enumerate(pending, 1):
            print(f"[{index}/{len(pending)}] J{joint+1} load={level} direction={direction:+d} repeat={repeat+1}/{REPEATS}")
            recovery = 0
            while True:
                try:
                    experiment.run_trial(control, model, data, joint, direction, repeat, j1,
                        park_override_deg=posture, support_posture_override=f"multiload_{level}",
                        output_dir=OUTPUT_DIR, file_tag=f"multiload_{level}",
                        collector_script=ENTRY_SCRIPT, protocol_fingerprint=protocol_sha256(),
                        extra_result={"load_level": level, "nominal_posture_deg": posture.tolist()})
                    break
                except Exception as error:
                    if not base.recoverable_error(error): raise
                    recovery += 1
                    if recovery > MAX_RECOVERY_ATTEMPTS:
                        raise RuntimeError("multiload recovery exhausted") from error
                    recovery_id = (joint + 1) * 1000 + (direction + 1) * 100 + repeat
                    arm, control = base.recover_arm_session(
                        arm, model, data, recovery_id, recovery, error)
    finally:
        base.best_effort_disable(arm); print("All axes disabled.")


if __name__ == "__main__": main()
