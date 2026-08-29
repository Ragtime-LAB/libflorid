#!/usr/bin/env python3
"""Analyze the newest real computed-torque telemetry without touching hardware."""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np

import run_computed_torque_demo as demo


ROOT = Path(__file__).resolve().parent
RUNS = ROOT / "runs_computed_torque"


def canonical_stage_id(tracking_joints, robust_layer_enabled, speed_scale):
    joints = tuple(int(joint) for joint in tracking_joints)
    if joints == (1, 2, 3, 4, 5, 6):
        joint_label = "six_axis"
    elif joints and len(set(joints)) == len(joints) and all(1 <= joint <= 6 for joint in joints):
        joint_label = "j" + "_".join(map(str, joints))
    else:
        return None
    controller_label = "robust" if robust_layer_enabled else "plain"
    speed_label = f"{float(speed_scale):.2f}".replace(".", "p")
    return f"{joint_label}_{controller_label}_{speed_label}"


def main():
    paths = sorted(RUNS.glob("computed_torque_*.npz"), key=lambda path: path.stat().st_mtime, reverse=True)
    if not paths:
        raise FileNotFoundError(f"no computed-torque telemetry in {RUNS}")
    source = paths[0]
    with np.load(source) as raw:
        data = {name: np.asarray(raw[name]) for name in raw.files}
    elapsed = data["elapsed_s"]; dt = np.diff(elapsed)
    if len(dt) == 0 or np.any(dt <= 0):
        raise ValueError("telemetry time must be strictly increasing")
    error_deg = np.rad2deg(data["q_ref"] - data["q"])
    raw_tau = data["tau_raw"] if "tau_raw" in data else data["tau_cmd"]
    command_tau = data["tau_cmd"]
    slew_delta = np.abs(raw_tau - command_tau)
    q_deg = np.rad2deg(data["q"])
    lower_margin = np.min(q_deg - demo.LOWER_DEG, axis=0)
    upper_margin = np.min(demo.UPPER_DEG - q_deg, axis=0)
    seq = np.asarray(data["seq"], dtype=np.int64)
    seq_delta = np.diff(seq)
    dropped_frames = int(np.sum(np.maximum(seq_delta - 1, 0)))
    robust_enabled = bool(np.asarray(data.get("robust_layer_enabled", False)).item())
    speed_scale = float(np.asarray(data.get("speed_scale", np.nan)).item())
    tracking_joints = np.asarray(data.get("tracking_joints", []), dtype=int).tolist()
    stage_id = str(np.asarray(data.get("stage_id", "missing")).item())
    expected_stage_id = canonical_stage_id(tracking_joints, robust_enabled, speed_scale)
    source_time = np.asarray(data.get("source_time_s", []), dtype=float)
    expected_duration = float(np.asarray(data.get("expected_duration_s", np.nan)).item())
    coverage_tolerance = (min(0.05, max(0.005, 0.01 * expected_duration))
                          if np.isfinite(expected_duration) and expected_duration > 0.0 else np.nan)
    coverage_valid = bool(
        source_time.shape == elapsed.shape and np.all(np.isfinite(source_time)) and
        np.all(np.diff(source_time) > 0.0) and np.isfinite(expected_duration) and
        source_time[0] <= coverage_tolerance and
        source_time[-1] >= expected_duration - coverage_tolerance and
        source_time[-1] <= expected_duration + coverage_tolerance
    )
    report = {
        "source": str(source), "samples": len(elapsed), "duration_s": float(elapsed[-1] - elapsed[0]),
        "feedback_rate_hz": {"mean": float(1.0 / np.mean(dt)), "p01": float(1.0 / np.quantile(dt, .99)),
            "minimum_instantaneous": float(1.0 / np.max(dt))},
        "tracking_rmse_deg": np.sqrt(np.mean(error_deg**2, axis=0)).tolist(),
        "tracking_max_abs_deg": np.max(np.abs(error_deg), axis=0).tolist(),
        "command_tau_peak_nm": np.max(np.abs(command_tau), axis=0).tolist(),
        "measured_tau_peak_nm": np.max(np.abs(data["tau_measured"]), axis=0).tolist(),
        "slew_intervention_fraction": np.mean(slew_delta > 1e-6, axis=0).tolist(),
        "minimum_limit_margin_deg": np.minimum(lower_margin, upper_margin).tolist(),
        "unique_seq": int(len(np.unique(data["seq"]))) == len(data["seq"]),
        "strictly_increasing_seq": bool(np.all(seq_delta > 0)),
        "maximum_seq_delta": int(np.max(seq_delta)) if len(seq_delta) else 0,
        "estimated_dropped_frames": dropped_frames,
        "estimated_dropped_frame_fraction": float(dropped_frames / max(len(seq) + dropped_frames, 1)),
        "firmware_error_frames": int(np.count_nonzero(data["errors"])),
        "robust_layer_enabled": robust_enabled,
        "run_completed": bool(np.asarray(data.get("run_completed", False)).item()),
        "safe_shutdown_completed": bool(np.asarray(data.get("safe_shutdown_completed", False)).item()),
        "termination_reason": str(np.asarray(data.get("termination_reason", "missing")).item()),
        "known_contact_free": bool(np.asarray(data.get("known_contact_free", False)).item()),
        "stage_id": stage_id,
        "expected_stage_id": expected_stage_id,
        "stage_identity_consistent": bool(expected_stage_id is not None and stage_id == expected_stage_id),
        "tracking_joints": tracking_joints,
        "expected_duration_s": expected_duration,
        "reference_coverage": {
            "present": bool(source_time.shape == elapsed.shape),
            "start_s": float(source_time[0]) if source_time.size else None,
            "end_s": float(source_time[-1]) if source_time.size else None,
            "tolerance_s": float(coverage_tolerance) if np.isfinite(coverage_tolerance) else None,
            "complete": coverage_valid,
        },
        "speed_scale": speed_scale,
        "design_sha256": str(np.asarray(data.get("design_sha256", "")).item()),
        "urdf_sha256": str(np.asarray(data.get("urdf_sha256", "")).item()),
        "bundle_sha256": str(np.asarray(data.get("bundle_sha256", "")).item()),
    }
    runtime_clearance = np.asarray(data.get("runtime_clearance_m", []), dtype=float)
    report["runtime_measured_collision_monitor"] = {
        "present": bool(runtime_clearance.shape == elapsed.shape),
        "finite_fraction": float(np.mean(np.isfinite(runtime_clearance))) if runtime_clearance.size else 0.0,
        "minimum_clearance_m": (float(np.nanmin(runtime_clearance))
            if runtime_clearance.size and np.any(np.isfinite(runtime_clearance)) else None),
        "threshold_m": demo.MIN_RUNTIME_MEASURED_CLEARANCE_M,
        "rate_hz": demo.RUNTIME_COLLISION_MONITOR_HZ,
    }
    for key, report_key in (("estimated_latency_ms", "estimated_latency_ms"),
                            ("state_age_ms", "state_age_ms"),
                            ("receive_jitter_us", "receive_jitter_us")):
        if key in data:
            values = np.asarray(data[key], dtype=float)
            report[report_key] = {"median": float(np.median(values)), "p95": float(np.quantile(values, .95)),
                                  "maximum": float(np.max(values))}
    report["passed_logged_safety_checks"] = bool(
        report["run_completed"] and report["safe_shutdown_completed"] and
        report["termination_reason"] == "reference_completed" and
        report["stage_identity_consistent"] and report["reference_coverage"]["complete"] and
        np.isfinite(report["expected_duration_s"]) and
        abs(report["duration_s"] - report["expected_duration_s"]) <= 0.25 and
        report["unique_seq"] and report["strictly_increasing_seq"] and
        report["estimated_dropped_frames"] == 0 and report["firmware_error_frames"] == 0 and
        report["feedback_rate_hz"]["mean"] >= demo.MIN_CONTROL_RATE_HZ and
        report["feedback_rate_hz"]["p01"] >= demo.MIN_CONTROL_RATE_HZ and
        float(np.max(dt)) <= demo.MAX_INTERFRAME_S and
        np.all(np.asarray(report["tracking_max_abs_deg"]) <= demo.MAX_FOLLOWING_ERROR_DEG) and
        np.all(np.asarray(report["command_tau_peak_nm"]) <= demo.TAU_LIMIT_NM) and
        np.all(np.asarray(report["minimum_limit_margin_deg"]) > 0.0) and
        report["runtime_measured_collision_monitor"]["present"] and
        report["runtime_measured_collision_monitor"]["finite_fraction"] == 1.0 and
        report["runtime_measured_collision_monitor"]["minimum_clearance_m"] >= demo.MIN_RUNTIME_MEASURED_CLEARANCE_M and
        ("estimated_latency_ms" not in report or report["estimated_latency_ms"]["maximum"] <= demo.MAX_ESTIMATED_LATENCY_MS) and
        ("state_age_ms" not in report or report["state_age_ms"]["maximum"] <= demo.MAX_STATE_AGE_MS) and
        ("receive_jitter_us" not in report or report["receive_jitter_us"]["maximum"] <= demo.MAX_RECEIVE_JITTER_US))
    destination = source.with_suffix(".report.json")
    destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2)); print("Wrote", destination)


if __name__ == "__main__":
    main()
