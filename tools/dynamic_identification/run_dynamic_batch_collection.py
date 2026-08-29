"""Resumable multi-design Willow dynamics dataset acquisition."""

from __future__ import annotations

import hashlib
import json
import os
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

import run_dynamic_collection as base


ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "batch_designs/manifest.json"
RUNS_ROOT = ROOT / "runs_dynamic_batch"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW HALF HOUR DYNAMIC DATASET"
RUNS_PER_BLOCK = 6
BLOCK_HOLD_S = 20.0
MAX_RUN_ATTEMPTS = 3


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_manifest():
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if payload.get("schema_version") != 1 or payload.get("runs") != len(payload.get("entries", [])):
        raise ValueError("malformed dynamic batch manifest")
    for entry in payload["entries"]:
        path = MANIFEST.parent / entry["file"]
        if not path.is_file() or sha256(path) != entry["sha256"]:
            raise RuntimeError(f"batch design missing or changed: {path}")
    return payload


def initialize_models():
    base.FRICTION_MODELS = base.friction_ff.load_friction_models()
    base.FRICTION_MODELS[1] = base.FRICTION_MODELS[2]
    base.BREAKAWAY_MODELS = base.friction_ff.load_breakaway_models()
    base.BREAKAWAY_MODELS[1] = base.BREAKAWAY_MODELS[2]
    base.friction_ff.FRICTION_SCALE[0] = base.friction_ff.FRICTION_SCALE[1]
    base.friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[0] = base.friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[1]
    base.friction_ff.BREAKAWAY_SCALE[0] = base.friction_ff.BREAKAWAY_SCALE[1]


def hold(control, model, data, pose, duration_s):
    state = base.read_valid(control, 2.0);last_seq = int(state.seq)
    deadline = time.monotonic() + duration_s;frames = 0;begin = time.monotonic()
    while time.monotonic() < deadline:
        state, *_ = base.cycle(control, model, data, pose, np.zeros(6), last_seq)
        last_seq = int(state.seq);frames += 1
    elapsed = time.monotonic() - begin
    print(f"block hold complete: {elapsed:.1f}s at {frames/elapsed:.1f} Hz")


def write_progress(path, manifest_hash, completed):
    payload = {
        "manifest_sha256": manifest_hash,
        "completed_run_indices": sorted(completed),
        "completed": len(completed),
        "updated_local": time.strftime("%Y-%m-%dT%H:%M:%S"),
    }
    temporary = path.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    with temporary.open("rb") as stream:os.fsync(stream.fileno())
    temporary.replace(path)


def main():
    payload = load_manifest();manifest_hash = sha256(MANIFEST)
    run_dir = RUNS_ROOT / manifest_hash[:12];run_dir.mkdir(parents=True, exist_ok=True)
    progress_path = run_dir / "progress.json"
    completed = set()
    if progress_path.exists():
        progress = json.loads(progress_path.read_text(encoding="utf-8"))
        if progress.get("manifest_sha256") != manifest_hash:
            raise RuntimeError("resume progress belongs to a different manifest")
        completed = {int(value) for value in progress.get("completed_run_indices", [])}
    entries = [entry for entry in payload["entries"] if int(entry["run_index"]) not in completed]
    active_minutes = sum(float(entry["duration_s"]) for entry in entries) / 60.0
    cooldown_minutes = (len(entries) // RUNS_PER_BLOCK) * BLOCK_HOLD_S / 60.0
    print(f"batch={payload['runs']} complete={len(completed)} pending={len(entries)}")
    print(f"remaining active={active_minutes:.1f}min block holds={cooldown_minutes:.1f}min plus smooth transitions")
    print("J3 design minimum:", payload["j3_min_deg"], "deg")
    print("HPP-FCL precheck:", payload["hppfcl_check_hz"], "Hz, minimum accepted clearance:",
          payload["minimum_model_clearance_m"] * 1000, "mm")
    print("output:", run_dir)
    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY: set ENABLE_HARDWARE=True after reviewing the manifest")
        return

    initialize_models();model = pin.buildModelFromUrdf(str(base.URDF));data = model.createData()
    arm = pyflorid.Arm.create(base.DEVICE_URI)
    if arm is None:raise RuntimeError(f"Arm.create failed for {base.DEVICE_URI}")
    arm.automatic_error_recovery();time.sleep(1.0)
    initial_state = base.read_valid(arm, 2.0);startup = np.asarray(initial_state.q, dtype=float).copy()
    print("measured batch startup q_deg:", np.round(np.rad2deg(startup), 3))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")
    try:
        control = base.friction_ff.start_session(arm)
        for pending_number, entry in enumerate(entries, start=1):
            index = int(entry["run_index"]);design_path = MANIFEST.parent / entry["file"]
            design_data = np.load(design_path)
            t, q_ref, dq_ref, ddq_ref = (np.asarray(design_data[name]) for name in ("t", "q", "dq", "ddq"))
            final = run_dir / f"run_{index:03d}_{entry['sha256'][:12]}.csv"
            temp = final.with_suffix(".csv.tmp")
            print(f"[{pending_number}/{len(entries)}] design={index:03d} sha={entry['sha256'][:12]}")
            for attempt in range(1, MAX_RUN_ATTEMPTS + 1):
                try:
                    base.acquire_once(control, model, data, t, q_ref, dq_ref, ddq_ref, temp,
                                      entry["sha256"], sha256(base.URDF))
                    temp.replace(final);completed.add(index)
                    write_progress(progress_path, manifest_hash, completed)
                    print("saved+fsynced:", final)
                    break
                except Exception as error:
                    if temp.exists():temp.unlink()
                    if attempt >= MAX_RUN_ATTEMPTS or not base.recoverable_acquisition_error(error):raise
                    print(f"recoverable run failure attempt {attempt}/{MAX_RUN_ATTEMPTS}: {error}")
                    arm, control = base.friction_ff.recover_session(arm, error)
                    print("fault cleared; returning to measured batch startup pose")
                    base.move(control, model, data, startup)
                    print("restarting the same design from sample zero")
            if len(completed) % RUNS_PER_BLOCK == 0 and pending_number < len(entries):
                print("block boundary: returning to batch startup pose")
                base.move(control, model, data, startup)
                hold(control, model, data, startup, BLOCK_HOLD_S)
        print("batch complete; returning to measured startup pose")
        base.move(control, model, data, startup)
        print("batch startup pose restored")
    finally:
        base.friction_ff.best_effort_disable(arm);print("All axes disabled.")


if __name__ == "__main__":
    main()
