#!/usr/bin/env python3
"""Acquire the existing 48 Willow multisine paths at a 120 deg/s peak.

The geometric paths are unchanged from ``batch_designs/manifest.json``; only
time is rescaled.  The tau field contains identified gravity, friction,
breakaway and rigid-body feedforward.  Per-axis torque is deliberately clipped
to the configured motor limits and both the unclipped and applied values are
logged so saturated samples can be excluded during identification.
"""
from __future__ import annotations

import csv
import hashlib
import json
import os
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

import run_10cycle_sine_computed_torque as control_lib


ROOT = Path(__file__).resolve().parent
SOURCE_MANIFEST = ROOT / "batch_designs/manifest.json"
RUNS_ROOT = ROOT / "runs_dynamic_batch_120deg_s"
TARGET_GLOBAL_PEAK_SPEED_DEG_S = 120.0
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW 48 DESIGN 120 DEG S DATASET"

RATE_HZ = 500.0
MIN_CONTROL_RATE_HZ = 300.0
RATE_WINDOW_S = 1.0
WARN_INTERFRAME_S = 0.020
HARD_MAX_INTERFRAME_S = 0.250
# Identification is based on measured q/dq/tau, so moderate reference tracking
# error is a data-quality warning, not a reason to throw away an otherwise
# valid excitation.  Only a gross loss of tracking triggers recovery.
WARN_FOLLOWING_ERROR_DEG = np.array([20., 20., 20., 25., 25., 25.])
HARD_MAX_FOLLOWING_ERROR_DEG = np.array([60., 60., 60., 60., 60., 60.])
TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.])
LOG_FSYNC_PERIOD_S = 0.25
RETURN_TO_MEASURED_START = True
MAX_DESIGN_ATTEMPTS = 3
MAX_RECOVERY_TO_START_ATTEMPTS = 3


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_manifest() -> dict:
    payload = json.loads(SOURCE_MANIFEST.read_text(encoding="utf-8"))
    entries = payload.get("entries", [])
    if payload.get("schema_version") != 1 or payload.get("runs") != 48 or len(entries) != 48:
        raise RuntimeError("expected the original 48-design manifest")
    for entry in entries:
        path = SOURCE_MANIFEST.parent / entry["file"]
        if not path.is_file() or sha256(path) != entry["sha256"]:
            raise RuntimeError(f"source design missing or changed: {path}")
    return payload


def scaled_design(entry: dict):
    path = SOURCE_MANIFEST.parent / entry["file"]
    with np.load(path) as design:
        t = np.asarray(design["t"], dtype=float)
        q = np.asarray(design["q"], dtype=float)
        dq = np.asarray(design["dq"], dtype=float)
        ddq = np.asarray(design["ddq"], dtype=float)
    peak = float(np.max(np.abs(np.rad2deg(dq))))
    if not np.isfinite(peak) or peak <= 0.0:
        raise RuntimeError(f"invalid source peak speed in {path}")
    scale = TARGET_GLOBAL_PEAK_SPEED_DEG_S / peak
    return (t - t[0]) / scale, q, dq * scale, ddq * scale * scale, scale, peak


def choose_encoder_branch(q_ref: np.ndarray, measured_q: np.ndarray):
    offsets = 2.0 * np.pi * np.round((np.asarray(measured_q) - q_ref[0]) / (2.0 * np.pi))
    return q_ref + offsets[None, :], offsets


def initialize_models():
    friction_models, breakaway_models, actuator_inertia = control_lib.load_models_and_actuator_inertia()
    model = pin.buildModelFromUrdf(str(control_lib.URDF))
    return model, model.createData(), friction_models, breakaway_models, actuator_inertia


def torque_terms(model, data, q, dq, ddq_ref, friction_models, breakaway_models,
                 actuator_inertia):
    gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
    rigid = np.asarray(pin.rnea(model, data, q, dq, ddq_ref), dtype=float)
    actuator = actuator_inertia * ddq_ref
    dynamic = rigid - gravity + actuator
    friction, breakaway = control_lib.friction_terms(
        q, dq, model, data, friction_models, breakaway_models
    )
    requested = (
        gravity + friction + breakaway
        + control_lib.DYNAMIC_TORQUE_SCALE * dynamic
    )
    applied = np.clip(requested, -TAU_LIMIT_NM, TAU_LIMIT_NM)
    saturated = np.abs(requested) > TAU_LIMIT_NM
    return gravity, friction, breakaway, dynamic, requested, applied, saturated


VECTOR_FIELDS = (
    "q_ref_deg", "dq_ref_deg_s", "ddq_ref_deg_s2", "q_deg", "dq_deg_s",
    "tau_measured_nm", "gravity_nm", "friction_nm", "breakaway_nm",
    "dynamic_scaled_nm", "tau_requested_nm", "tau_applied_nm", "saturated",
    "kp", "kd",
)


class RunLogger:
    def __init__(self, path: Path, run_index: int):
        self.path = path
        self.run_index = run_index
        self.stream = path.open("w", encoding="utf-8", newline="", buffering=1024 * 1024)
        self.writer = csv.writer(self.stream)
        header = [
            "elapsed_s", "host_time_s", "seq", "source_timestamp_us", "errors",
            "run_index", "command_written",
        ]
        for field in VECTOR_FIELDS:
            header.extend(f"{field}_j{joint}" for joint in range(1, 7))
        self.writer.writerow(header)
        self.rows = 0
        self.last_fsync = 0.0
        self.flush(True)

    def write(self, elapsed, state, q_ref, dq_ref, ddq_ref, q, dq, tau_measured,
              gravity, friction, breakaway, dynamic_scaled, requested, applied,
              saturated, command_written=True):
        row = [
            elapsed, time.time(), int(state.seq),
            int(getattr(state, "source_timestamp_us", 0)), int(state.errors),
            self.run_index, int(bool(command_written)),
        ]
        vectors = (
            np.rad2deg(q_ref), np.rad2deg(dq_ref), np.rad2deg(ddq_ref),
            np.rad2deg(q), np.rad2deg(dq), tau_measured, gravity, friction,
            breakaway, dynamic_scaled, requested, applied,
            np.asarray(saturated, dtype=int), control_lib.TRACK_MIT_KP,
            control_lib.TRACK_MIT_KD,
        )
        for vector in vectors:
            row.extend(np.asarray(vector).tolist())
        self.writer.writerow(row)
        self.rows += 1
        self.flush(False)

    def flush(self, force=False):
        now = time.monotonic()
        if force or now - self.last_fsync >= LOG_FSYNC_PERIOD_S:
            self.stream.flush()
            # Never issue a blocking disk fsync inside the 500 Hz control loop.
            # A full fsync is still performed at close, before the run is marked
            # complete in progress.json.
            if force:
                os.fsync(self.stream.fileno())
            self.last_fsync = now

    def close(self):
        if not self.stream.closed:
            self.flush(True)
            self.stream.close()


def run_reference(control, model, data, t, q_ref, dq_ref, ddq_ref,
                  friction_models, breakaway_models, actuator_inertia, logger):
    state, drained = control_lib.drain_feedback_queue(control)
    if drained > 1:
        print(f"  reference: discarded {drained - 1} stale feedback frames")
    last_seq = int(state.seq)
    started = time.monotonic()
    last_frame = None
    rate_started = started
    rate_frames = 0
    sample_period = float(np.median(np.diff(t)))
    index = 0
    saturation_rows = 0
    saturation_axes = np.zeros(6, dtype=int)
    while index < len(t):
        elapsed = time.monotonic() - started
        desired = min(int(elapsed / sample_period), len(t) - 1)
        state = control_lib.read_valid(control, last_seq=last_seq)
        now = time.monotonic()
        last_seq = int(state.seq)
        if last_frame is not None:
            interframe = now - last_frame
            if interframe > HARD_MAX_INTERFRAME_S:
                raise RuntimeError(
                    f"feedback gap {interframe:.4f}s exceeds hard "
                    f"{HARD_MAX_INTERFRAME_S:.4f}s"
                )
            if interframe > WARN_INTERFRAME_S:
                print(
                    f"  WARNING feedback scheduling gap {interframe*1000:.1f}ms; "
                    "new feedback frame is valid, continuing"
                )
        last_frame = now
        q, dq, tau_measured = control_lib.checked_state(state)
        qr, dqr, ddqr = q_ref[desired], dq_ref[desired], ddq_ref[desired]
        terms = torque_terms(
            model, data, q, dq, ddqr, friction_models, breakaway_models,
            actuator_inertia,
        )
        gravity, friction, breakaway, dynamic, requested, applied, saturated = terms
        dynamic_scaled = control_lib.DYNAMIC_TORQUE_SCALE * dynamic
        error_deg = np.abs(np.rad2deg(qr - q))
        if np.any(error_deg > HARD_MAX_FOLLOWING_ERROR_DEG):
            logger.write(
                elapsed, state, qr, dqr, ddqr, q, dq, tau_measured, gravity,
                friction, breakaway, dynamic_scaled, requested, applied,
                saturated, command_written=False,
            )
            raise RuntimeError(f"gross following error exceeded before command: {np.round(error_deg, 2)}deg")
        control.write_once(control_lib.mit_command(
            qr, dqr, applied, control_lib.TRACK_MIT_KP, control_lib.TRACK_MIT_KD
        ))
        logger.write(
            elapsed, state, qr, dqr, ddqr, q, dq, tau_measured, gravity,
            friction, breakaway, dynamic_scaled, requested, applied, saturated,
        )
        if np.any(saturated):
            saturation_rows += 1
            saturation_axes += saturated.astype(int)
        index = desired + 1
        rate_frames += 1
        if now - rate_started >= RATE_WINDOW_S:
            rate = rate_frames / (now - rate_started)
            warning = " WARNING_TRACKING" if np.any(error_deg > WARN_FOLLOWING_ERROR_DEG) else ""
            print(
                f"  rate={rate:.1f}Hz t={elapsed:.1f}/{t[-1]:.1f}s "
                f"max_error={np.max(error_deg):.2f}deg "
                f"tau={np.round(applied, 2)}Nm clip={saturated.astype(int).tolist()}"
                f"{warning}"
            )
            if rate < MIN_CONTROL_RATE_HZ:
                raise RuntimeError(f"control rate {rate:.1f}Hz below {MIN_CONTROL_RATE_HZ:g}Hz")
            rate_started = now
            rate_frames = 0
    return saturation_rows, saturation_axes


def write_progress(path: Path, manifest_hash: str, completed: set[int]):
    payload = {
        "source_manifest_sha256": manifest_hash,
        "target_global_peak_speed_deg_s": TARGET_GLOBAL_PEAK_SPEED_DEG_S,
        "completed_run_indices": sorted(completed),
        "completed": len(completed),
        "updated_local": time.strftime("%Y-%m-%dT%H:%M:%S"),
    }
    temporary = path.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    with temporary.open("rb") as stream:
        os.fsync(stream.fileno())
    temporary.replace(path)


def recover_to_batch_start(arm, control, error, model, data, startup,
                           friction_models, breakaway_models, actuator_inertia):
    """Clear faults/reconnect, drain feedback, then MIT-interpolate to startup."""
    last_error = error
    for recovery_attempt in range(1, MAX_RECOVERY_TO_START_ATTEMPTS + 1):
        print(
            f"  automatic recovery {recovery_attempt}/{MAX_RECOVERY_TO_START_ATTEMPTS}: "
            f"{last_error}"
        )
        try:
            arm, control = control_lib.friction_ff.recover_session(arm, last_error)
            latest, drained = control_lib.drain_feedback_queue(control)
            measured_q, _, _ = control_lib.checked_state(latest)
            print(f"  recovery discarded {max(0, drained-1)} stale feedback frames")
            print("  recovery measured q_deg:", np.round(np.rad2deg(measured_q), 3))
            startup_target = startup + 2.0 * np.pi * np.round(
                (measured_q - startup) / (2.0 * np.pi)
            )
            print("  MIT returning to batch startup q_deg:", np.round(np.rad2deg(startup_target), 3))
            old_hold = control_lib.TARGET_HOLD_S
            old_speed = control_lib.MAX_ENTRY_SPEED_DEG_S
            control_lib.TARGET_HOLD_S = max(old_hold, 2.0)
            control_lib.MAX_ENTRY_SPEED_DEG_S = max(old_speed, 12.0)
            try:
                control_lib.soft_move(
                    control, model, data, startup_target, friction_models,
                    breakaway_models, actuator_inertia,
                )
            finally:
                control_lib.TARGET_HOLD_S = old_hold
                control_lib.MAX_ENTRY_SPEED_DEG_S = old_speed
            latest, drained = control_lib.drain_feedback_queue(control)
            recovered_q, _, _ = control_lib.checked_state(latest)
            print(f"  post-return discarded {max(0, drained-1)} stale feedback frames")
            print("  recovery complete q_deg:", np.round(np.rad2deg(recovered_q), 3))
            return arm, control
        except Exception as recovery_error:
            last_error = recovery_error
            print(f"  recovery-to-start attempt failed: {recovery_error}")
    raise RuntimeError("automatic recovery-to-start exhausted") from last_error


def main():
    payload = load_manifest()
    manifest_hash = sha256(SOURCE_MANIFEST)
    run_dir = RUNS_ROOT / f"{manifest_hash[:12]}_120deg_s"
    run_dir.mkdir(parents=True, exist_ok=True)
    progress_path = run_dir / "progress.json"
    completed: set[int] = set()
    if progress_path.exists():
        progress = json.loads(progress_path.read_text(encoding="utf-8"))
        if (
            progress.get("source_manifest_sha256") != manifest_hash
            or float(progress.get("target_global_peak_speed_deg_s", -1))
            != TARGET_GLOBAL_PEAK_SPEED_DEG_S
        ):
            raise RuntimeError("resume progress belongs to another batch configuration")
        completed = {int(value) for value in progress.get("completed_run_indices", [])}

    pending = [entry for entry in payload["entries"] if int(entry["run_index"]) not in completed]
    scaled = [scaled_design(entry) for entry in pending]
    active_s = sum(float(item[0][-1]) for item in scaled)
    peak_acc = max(float(np.max(np.abs(np.rad2deg(item[3])))) for item in scaled) if scaled else 0.0
    print("120 deg/s DYNAMIC DATASET")
    print(f"batch=48 complete={len(completed)} pending={len(pending)}")
    print(f"remaining active excitation={active_s/60.0:.2f}min plus MIT transitions")
    print(f"global peak speed={TARGET_GLOBAL_PEAK_SPEED_DEG_S:.1f}deg/s, peak acceleration={peak_acc:.1f}deg/s^2")
    print("geometric q paths are byte-derived from the HPP-FCL-audited 48 designs; only time is rescaled")
    print("tau clipping J1..J6:", TAU_LIMIT_NM.tolist(), "Nm")
    print("output:", run_dir)
    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY")
        return

    model, data, friction_models, breakaway_models, actuator_inertia = initialize_models()
    arm = pyflorid.Arm.create(control_lib.DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {control_lib.DEVICE_URI}")
    arm.automatic_error_recovery()
    time.sleep(1.0)
    initial, drained = control_lib.drain_feedback_queue(arm)
    startup, _, _ = control_lib.checked_state(initial)
    print(f"startup: discarded {max(0, drained-1)} stale feedback frames")
    print("measured startup q_deg:", np.round(np.rad2deg(startup), 3))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")

    control = None
    try:
        control = control_lib.friction_ff.start_session(arm)
        for pending_number, (entry, design) in enumerate(zip(pending, scaled), start=1):
            index = int(entry["run_index"])
            t, q_source, dq_ref, ddq_ref, speed_scale, source_peak = design
            final = run_dir / f"run_{index:03d}_{entry['sha256'][:12]}_120deg_s.csv"
            metadata = final.with_suffix(".json")
            for design_attempt in range(1, MAX_DESIGN_ATTEMPTS + 1):
                latest, drained = control_lib.drain_feedback_queue(control)
                measured_q, _, _ = control_lib.checked_state(latest)
                q_ref, branch_offsets = choose_encoder_branch(q_source, measured_q)
                partial = final.with_name(
                    final.stem + f".attempt{design_attempt}.partial.csv"
                )
                print(
                    f"[{pending_number}/{len(pending)}] design={index:03d} "
                    f"attempt={design_attempt}/{MAX_DESIGN_ATTEMPTS} "
                    f"duration={t[-1]:.2f}s speed_scale={speed_scale:.3f} "
                    f"source_peak={source_peak:.2f}deg/s stale={max(0, drained-1)}"
                )
                print("  encoder branch offsets deg:", np.rad2deg(branch_offsets).astype(int).tolist())
                logger = RunLogger(partial, index)
                try:
                    control_lib.soft_move(
                        control, model, data, q_ref[0], friction_models,
                        breakaway_models, actuator_inertia,
                    )
                    saturation_rows, saturation_axes = run_reference(
                        control, model, data, t, q_ref, dq_ref, ddq_ref,
                        friction_models, breakaway_models, actuator_inertia, logger,
                    )
                except Exception as error:
                    logger.close()
                    failed = partial.with_name(
                        partial.name.replace(".partial.csv", f".failed_{time.strftime('%Y%m%dT%H%M%S')}.csv")
                    )
                    partial.replace(failed)
                    print(f"  design attempt failed; partial telemetry preserved: {failed}")
                    if design_attempt >= MAX_DESIGN_ATTEMPTS:
                        raise RuntimeError(
                            f"design {index:03d} exhausted {MAX_DESIGN_ATTEMPTS} attempts"
                        ) from error
                    arm, control = recover_to_batch_start(
                        arm, control, error, model, data, startup,
                        friction_models, breakaway_models, actuator_inertia,
                    )
                    print("  retrying the same design from sample zero")
                    continue
                else:
                    logger.close()
                    partial.replace(final)
                    break
            metadata.write_text(json.dumps({
                "run_index": index,
                "source_design": entry["file"],
                "source_design_sha256": entry["sha256"],
                "source_manifest_sha256": manifest_hash,
                "target_global_peak_speed_deg_s": TARGET_GLOBAL_PEAK_SPEED_DEG_S,
                "source_global_peak_speed_deg_s": source_peak,
                "time_scale": speed_scale,
                "duration_s": float(t[-1]),
                "encoder_branch_offsets_deg": np.rad2deg(branch_offsets).tolist(),
                "csv_rows": logger.rows,
                "saturated_rows": saturation_rows,
                "saturated_rows_by_axis": saturation_axes.tolist(),
                "hppfcl_path_note": "q path unchanged from source audited design; only t/dq/ddq rescaled",
            }, indent=2), encoding="utf-8")
            completed.add(index)
            write_progress(progress_path, manifest_hash, completed)
            print(
                f"  saved+fsynced rows={logger.rows} saturated_rows={saturation_rows} "
                f"completed={len(completed)}/48"
            )

        if RETURN_TO_MEASURED_START:
            print("all 48 designs saved; returning to measured startup with MIT position bridge")
            latest, _ = control_lib.drain_feedback_queue(control)
            measured_q, _, _ = control_lib.checked_state(latest)
            startup_target = startup + 2.0 * np.pi * np.round((measured_q - startup) / (2.0 * np.pi))
            try:
                control_lib.soft_move(
                    control, model, data, startup_target, friction_models,
                    breakaway_models, actuator_inertia,
                )
                print("batch startup pose restored")
            except Exception as error:
                print(f"return failed after all data were already saved: {error}")
    finally:
        control_lib.friction_ff.best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
