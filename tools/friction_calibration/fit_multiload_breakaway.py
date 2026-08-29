#!/usr/bin/env python3
"""Fit load-conditioned directional breakaway torque from the 90-pose run."""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent
RUNS = ROOT / "runs_breakaway_multiload_10deg"
OUTPUT = ROOT / "multiload_breakaway_fit.json"
MIN_ONSET_ELAPSED_S = 0.5
MAX_SUPPORT_POSTURE_ERROR_DEG = 5.0


def wrapped_error_deg(actual, nominal):
    return (np.asarray(actual) - np.asarray(nominal) + 180.0) % 360.0 - 180.0


def main():
    rows = []
    for path in sorted(RUNS.glob("breakaway_multiload_*.json")):
        item = json.loads(path.read_text(encoding="utf-8"))
        candidate = item.get("breakaway_candidate")
        samples = item.get("samples", [])
        if candidate is None or not samples:
            continue
        index = int(candidate["sample_index"])
        joint0 = int(item["joint"]) - 1
        nominal = np.asarray(item["nominal_posture_deg"], dtype=float)
        q0 = np.rad2deg(np.asarray(samples[0]["q_rad"], dtype=float))
        errors = wrapped_error_deg(q0, nominal)
        support_errors = np.delete(errors, [0, joint0])
        direction = int(item["direction"])
        magnitude = direction * float(candidate["command_probe_tau_nm"])
        elapsed = float(samples[index]["elapsed_s"])
        support_error = float(np.max(np.abs(support_errors)))
        accepted = bool(elapsed >= MIN_ONSET_ELAPSED_S and
            support_error <= MAX_SUPPORT_POSTURE_ERROR_DEG and magnitude >= 0.0)
        rows.append({
            "file": path.name, "joint": joint0 + 1,
            "load_level": item["load_level"], "direction": direction,
            "breakaway_magnitude_nm": magnitude,
            "absolute_gravity_load_nm": float(candidate["absolute_gravity_load_nm"]),
            "onset_elapsed_s": elapsed,
            "support_posture_error_deg": support_error,
            "accepted": accepted,
        })

    payload = {
        "schema_version": 1,
        "model": "Fs_direction(load) = intercept_nm + slope_nm_per_nm * abs(gravity_joint_nm)",
        "quality_rule": {
            "minimum_onset_elapsed_s": MIN_ONSET_ELAPSED_S,
            "maximum_support_posture_error_deg": MAX_SUPPORT_POSTURE_ERROR_DEG,
            "note": "Quality filtering is reported explicitly; rejected trials remain in raw_trials.",
        },
        "raw_trial_count": len(rows), "joints": {}, "raw_trials": rows,
    }
    for joint in (2, 3, 4):
        joint_item = {"raw_count": sum(r["joint"] == joint for r in rows), "directions": {}}
        for direction, label in ((-1, "negative"), (1, "positive")):
            selected = [r for r in rows if r["joint"] == joint and
                r["direction"] == direction and r["accepted"]]
            summary = {"accepted_count": len(selected), "load_levels": {}}
            for level in ("low", "mid", "high"):
                group = [r for r in selected if r["load_level"] == level]
                summary["load_levels"][level] = {
                    "count": len(group),
                    "median_breakaway_nm": None if not group else float(np.median(
                        [r["breakaway_magnitude_nm"] for r in group])),
                    "median_absolute_gravity_load_nm": None if not group else float(np.median(
                        [r["absolute_gravity_load_nm"] for r in group])),
                }
            if len(selected) >= 2:
                load = np.asarray([r["absolute_gravity_load_nm"] for r in selected])
                torque = np.asarray([r["breakaway_magnitude_nm"] for r in selected])
                matrix = np.column_stack((np.ones(len(load)), load))
                intercept, slope = np.linalg.lstsq(matrix, torque, rcond=None)[0]
                prediction = matrix @ np.array([intercept, slope])
                denominator = float(np.sum((torque - np.mean(torque)) ** 2))
                r2 = None if denominator == 0 else 1.0 - float(np.sum((torque - prediction) ** 2)) / denominator
                summary["fit"] = {"intercept_nm": float(intercept),
                    "slope_nm_per_nm": float(slope), "r2": r2}
            else:
                summary["fit"] = None
            joint_item["directions"][label] = summary
        payload["joints"][f"J{joint}"] = joint_item

    OUTPUT.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload["joints"], indent=2))
    print("wrote", OUTPUT)


if __name__ == "__main__":
    main()
