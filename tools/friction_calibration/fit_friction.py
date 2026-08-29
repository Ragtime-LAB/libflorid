"""Compare symmetric, asymmetric and Stribeck friction models with held-out repeats."""
from __future__ import annotations
import csv, functools, hashlib, json, os, subprocess, sys
from pathlib import Path
import numpy as np
import pinocchio as pin
from scipy.optimize import least_squares
from scipy.signal import savgol_filter

ROOT = Path(__file__).resolve().parent
RUNS = ROOT / "runs"
LOW_SPEED_RUNS = ROOT / "runs_low_speed"
BREAKAWAY_RUNS = ROOT / "runs_breakaway_2deg"
J1_RUNS = ROOT / "runs_j1"
J1_LOW_SPEED_RUNS = ROOT / "runs_j1_low_speed"
J1_BREAKAWAY_RUNS = ROOT / "runs_j1_breakaway"
BREAKAWAY_COLLECTOR = ROOT / "run_breakaway_collection.py"  # compatibility alias
OUTPUT = ROOT / "friction_fit.json"
_LOCAL_URDF = ROOT.parent / "static_gravity_calibration" / "model" / "identified" / "Ragtime_Willow.static-mass-com-calibrated.urdf"
URDF = _LOCAL_URDF if _LOCAL_URDF.exists() else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_static_mass_com_calibrated.urdf"
HELD_OUT_REPEAT = 2
MIN_SPEED_RATIO = 0.60
BOOTSTRAP_SAMPLES = 300
STRIBECK_MAX_LOW_SPEED_RAD_S = 0.025
STRIBECK_MIN_LOW_SPEED_TRIALS = 4
STRIBECK_MAX_VS_CI_RATIO = 10.0
STRIBECK_MAX_VS_CI_UPPER_RAD_S = 0.25
MAX_LOG_GAP_S = 0.020
MAX_DROPPED_FRAME_RATIO = 0.05
MAX_SEQUENCE_DELTA = 10
LOG_SCHEMA_VERSION = "3"
DEVICE_TIME_UNIT = "s"
COLLECTOR_BY_DIRECTORY = {
    "runs": ROOT / "run_friction_collection.py",
    "runs_low_speed": ROOT / "run_low_speed_stribeck_collection.py",
    "runs_j1": ROOT / "run_j1_friction_collection.py",
    "runs_j1_low_speed": ROOT / "run_j1_low_speed_collection.py",
}
BREAKAWAY_COLLECTOR_BY_DIRECTORY = {
    "runs_breakaway": ROOT / "run_breakaway_collection.py",
    "runs_breakaway_2deg": ROOT / "run_breakaway_collection.py",
    "runs_j1_breakaway": ROOT / "run_j1_breakaway_collection.py",
}
BASE_COLLECTOR = ROOT / "run_friction_collection.py"
BREAKAWAY_IMPLEMENTATION = ROOT / "run_breakaway_collection.py"
PRE_RECOVERY_FRICTION_COLLECTOR_SHA256 = "a264f9f6df4fae96803cb1057b5fedc2d14ddccb52b4eb699c12be7c2ccc333e"

def sha256(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()


@functools.lru_cache(maxsize=8)
def expected_protocol_sha256(script):
    """Ask the current disabled-by-default collector for its canonical config hash."""
    completed = subprocess.run(
        [sys.executable, str(script), "--print-protocol-sha256"], cwd=ROOT,
        check=True, capture_output=True, text=True,
    )
    lines = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
    value = lines[-1] if lines else ""
    if len(value) != 64 or any(character not in "0123456789abcdef" for character in value.lower()):
        raise RuntimeError(f"{script}: collector did not return a SHA-256 protocol fingerprint")
    return value.lower()


def estimate_acceleration(device_time, velocity):
    """Estimate ddq on an irregular device-time grid without differentiating raw jitter."""
    time_axis = np.asarray(device_time, dtype=float)
    velocity = np.asarray(velocity, dtype=float)
    if velocity.ndim != 2 or velocity.shape[0] != len(time_axis) or velocity.shape[1] != 6:
        raise ValueError("velocity must have shape (samples, 6)")
    if len(time_axis) < 5 or np.any(np.diff(time_axis) <= 0.0):
        raise ValueError("at least five strictly increasing device-time samples are required")
    uniform_time = np.linspace(time_axis[0], time_axis[-1], len(time_axis))
    uniform_velocity = np.column_stack([
        np.interp(uniform_time, time_axis, velocity[:, joint]) for joint in range(6)
    ])
    window = min(101, len(time_axis) if len(time_axis) % 2 else len(time_axis) - 1)
    window = max(window, 5)
    polynomial = min(3, window - 2)
    dt = float(uniform_time[1] - uniform_time[0])
    uniform_acceleration = savgol_filter(
        uniform_velocity, window_length=window, polyorder=polynomial,
        deriv=1, delta=dt, axis=0, mode="interp")
    return np.column_stack([
        np.interp(time_axis, uniform_time, uniform_acceleration[:, joint]) for joint in range(6)
    ])

def load_trials():
    result = {joint: [] for joint in range(1, 7)}
    model = pin.buildModelFromUrdf(str(URDF)); data = model.createData()
    paths = (sorted(RUNS.glob("trial_*.csv")) + sorted(LOW_SPEED_RUNS.glob("trial_*.csv")) +
        sorted(J1_RUNS.glob("trial_*.csv")) + sorted(J1_LOW_SPEED_RUNS.glob("trial_*.csv")))
    for path in paths:
        with path.open(newline="", encoding="utf-8") as file: rows = list(csv.DictReader(file))
        if len(rows) < 5: continue
        required = {"seq", "errors", "schema_version", "device_time_unit", "urdf_sha256",
            "protocol_sha256", "collector_script_sha256", "base_collector_script_sha256", "support_posture"}
        if not required.issubset(rows[0]):
            raise RuntimeError(f"{path}: legacy/incomplete friction log lacks {sorted(required - set(rows[0]))}; recollect it")
        if any(int(row["errors"]) != 0 for row in rows):
            raise RuntimeError(f"{path}: firmware error is present in saved friction data")
        sequences = [int(row["seq"]) for row in rows]
        if any(seq == 0 for seq in sequences) or len(sequences) != len(set(sequences)):
            raise RuntimeError(f"{path}: zero or duplicate feedback sequence number")
        sequence_delta = np.diff(np.asarray(sequences, dtype=np.int64))
        dropped_frames = int(np.sum(np.maximum(sequence_delta - 1, 0)))
        maximum_dropped = max(1, int(MAX_DROPPED_FRAME_RATIO * len(rows)))
        if (np.any(sequence_delta <= 0) or
                (len(sequence_delta) and int(np.max(sequence_delta)) > MAX_SEQUENCE_DELTA) or
                dropped_frames > maximum_dropped):
            raise RuntimeError(f"{path}: reordered or excessive dropped feedback frames")
        urdf_hashes = {row["urdf_sha256"] for row in rows}
        protocol_hashes = {row["protocol_sha256"] for row in rows}
        script_hashes = {row["collector_script_sha256"] for row in rows}
        base_script_hashes = {row["base_collector_script_sha256"] for row in rows}
        if urdf_hashes != {sha256(URDF)}:
            raise RuntimeError(f"{path}: URDF provenance is missing, mixed or stale")
        expected_script = COLLECTOR_BY_DIRECTORY[path.parent.name]
        if protocol_hashes != {expected_protocol_sha256(expected_script)}:
            raise RuntimeError(f"{path}: protocol provenance is missing, mixed or stale")
        allowed_script_hashes = {sha256(expected_script)}
        allowed_base_hashes = {sha256(BASE_COLLECTOR)}
        if path.parent == RUNS:
            # Same schema/protocol/URDF, collected immediately before bounded
            # fault and USB-session recovery was added to the collector.
            allowed_script_hashes.add(PRE_RECOVERY_FRICTION_COLLECTOR_SHA256)
            allowed_base_hashes.add(PRE_RECOVERY_FRICTION_COLLECTOR_SHA256)
        if len(script_hashes) != 1 or not script_hashes.issubset(allowed_script_hashes):
            raise RuntimeError(f"{path}: collector script provenance is missing, mixed or stale")
        if len(base_script_hashes) != 1 or not base_script_hashes.issubset(allowed_base_hashes):
            raise RuntimeError(f"{path}: base collector implementation provenance is missing, mixed or stale")
        if {row["schema_version"] for row in rows} != {LOG_SCHEMA_VERSION} or {row["device_time_unit"] for row in rows} != {DEVICE_TIME_UNIT}:
            raise RuntimeError(f"{path}: unsupported log schema or device-time unit")
        singleton_fields = ("joint", "repeat", "support_posture", "target_speed_rad_s", "trial_id")
        if any(len({row[field] for row in rows}) != 1 for field in singleton_fields):
            raise RuntimeError(f"{path}: per-trial metadata changes inside one CSV")
        joint, repeat = int(rows[0]["joint"]), int(rows[0]["repeat"])
        target_speed = float(rows[0]["target_speed_rad_s"])
        t = np.asarray([float(row["device_time"]) for row in rows])
        gaps = np.diff(t)
        if np.any(gaps <= 0.0) or float(np.max(gaps)) > MAX_LOG_GAP_S:
            raise RuntimeError(f"{path}: nonmonotonic time or feedback gap exceeds {MAX_LOG_GAP_S:g}s")
        q = np.asarray([float(row[f"q{joint}_rad"]) for row in rows])
        dq = np.asarray([float(row[f"dq{joint}_rad_s"]) for row in rows])
        q_all = np.asarray([[float(row[f"q{axis}_rad"]) for axis in range(1, 7)] for row in rows])
        dq_all = np.asarray([[float(row[f"dq{axis}_rad_s"]) for axis in range(1, 7)] for row in rows])
        measured = np.asarray([float(row[f"tau{joint}_nm"]) for row in rows])
        if not np.all(np.isfinite(np.c_[t, q_all, dq_all, measured])):
            raise RuntimeError(f"{path}: non-finite time/state/torque data")
        ddq_all = estimate_acceleration(t, dq_all)
        rigid_torque = np.asarray([
            pin.rnea(model, data, qi, dqi, ddqi)[joint - 1]
            for qi, dqi, ddqi in zip(q_all, dq_all, ddq_all)
        ])
        residual = measured - rigid_torque
        tc = t - np.mean(t); denom = float(np.sum(tc * tc))
        actual_speed = float(np.sum(tc * (q - np.mean(q))) / denom) if denom > 0 else 0.0
        if np.sign(actual_speed) != np.sign(target_speed) or abs(actual_speed) < MIN_SPEED_RATIO * abs(target_speed): continue
        median = float(np.median(residual))
        result[joint].append({"path": str(path.relative_to(ROOT)), "repeat": repeat, "target_speed": target_speed,
            "support_posture": rows[0].get("support_posture", "legacy_unspecified"),
            "urdf_sha256": next(iter(urdf_hashes)), "protocol_sha256": next(iter(protocol_hashes)),
            "speed": actual_speed, "feedback_speed_median": float(np.median(dq)), "torque": median,
            "torque_mad": float(np.median(np.abs(residual - median))), "samples": len(rows),
            "active_joint_abs_ddq_median_rad_s2": float(np.median(np.abs(ddq_all[:, joint - 1]))),
            "active_joint_abs_ddq_p95_rad_s2": float(np.percentile(np.abs(ddq_all[:, joint - 1]), 95.0)),
            "estimated_dropped_frames": dropped_frames})
    return result

def pred_symmetric(p, v): return p[0] * v + p[1] * np.sign(v) + p[2]
def pred_asymmetric(p, v):
    return np.where(v >= 0, p[4] + p[0] * v + p[1], p[4] + p[2] * v - p[3])
def pred_stribeck(p, v):
    magnitude = p[1] + (p[2] - p[1]) * np.exp(-np.square(np.abs(v) / p[3]))
    return p[0] * v + np.sign(v) * magnitude + p[4]

def metrics(actual, predicted):
    residual = actual - predicted; ss_res = float(np.sum(residual * residual)); centered = actual - np.mean(actual)
    ss_tot = float(np.sum(centered * centered))
    return {"rmse_nm": float(np.sqrt(np.mean(residual * residual))), "mae_nm": float(np.mean(np.abs(residual))),
        "r2": 1.0 - ss_res / ss_tot if ss_tot > 0 else None, "bias_nm": float(np.mean(residual)),
        "max_abs_nm": float(np.max(np.abs(residual)))}

def split_trials(items):
    """Fail closed unless repeat 2 is a genuinely different support posture."""
    train = [x for x in items if x["repeat"] != HELD_OUT_REPEAT]
    heldout = [x for x in items if x["repeat"] == HELD_OUT_REPEAT]
    bad_train = [x["path"] for x in train if x["support_posture"] != "train"]
    bad_heldout = [x["path"] for x in heldout if x["support_posture"] != "heldout"]
    if bad_train or bad_heldout:
        raise RuntimeError(
            "support-posture provenance does not match repeat partition: "
            f"bad_train={bad_train}, bad_heldout={bad_heldout}"
        )
    return train, heldout

def stribeck_eligibility(items, breakaway):
    low = [x for x in items if abs(x["speed"]) <= STRIBECK_MAX_LOW_SPEED_RAD_S]
    directions = {int(np.sign(x["speed"])) for x in low if x["speed"] != 0.0}
    levels = {round(abs(x["target_speed"]), 8) for x in low}
    breakaway_directions = {int(x["direction"]) for x in breakaway}
    reasons = []
    if len(low) < STRIBECK_MIN_LOW_SPEED_TRIALS:
        reasons.append(f"low-speed trials {len(low)} < {STRIBECK_MIN_LOW_SPEED_TRIALS}")
    if directions != {-1, 1}:
        reasons.append("low-speed trials are not bidirectional")
    if len(levels) < 2:
        reasons.append("fewer than two distinct low-speed levels")
    if breakaway_directions != {-1, 1}:
        reasons.append("training breakaway is not bidirectional")
    return not reasons, reasons


def stribeck_uncertainty_reasons(model):
    """Fail closed when Fs/vs are selected but not statistically resolved."""
    intervals = model.get("uncertainty", {}).get("parameters", {})
    reasons = []
    for name in ("fc_nm", "fs_nm", "vs_rad_s"):
        ci = np.asarray(intervals.get(name, {}).get("ci95", []), dtype=float)
        if ci.shape != (2,) or not np.all(np.isfinite(ci)) or ci[0] < 0.0 or ci[0] > ci[1]:
            reasons.append(f"{name} has invalid bootstrap CI")
    vs_ci = np.asarray(intervals.get("vs_rad_s", {}).get("ci95", []), dtype=float)
    if vs_ci.shape == (2,) and np.all(np.isfinite(vs_ci)):
        if vs_ci[0] <= 0.003 * 1.01 or vs_ci[1] >= STRIBECK_MAX_VS_CI_UPPER_RAD_S:
            reasons.append("vs confidence interval touches a numerical/data-support boundary")
        elif vs_ci[1] / max(vs_ci[0], np.finfo(float).tiny) > STRIBECK_MAX_VS_CI_RATIO:
            reasons.append(f"vs confidence ratio exceeds {STRIBECK_MAX_VS_CI_RATIO:g}")
    fc_ci = np.asarray(intervals.get("fc_nm", {}).get("ci95", []), dtype=float)
    fs_ci = np.asarray(intervals.get("fs_nm", {}).get("ci95", []), dtype=float)
    if fc_ci.shape == fs_ci.shape == (2,) and np.all(np.isfinite(np.r_[fc_ci, fs_ci])) and fs_ci[1] <= fc_ci[0]:
        reasons.append("static friction is not distinguishable above Coulomb friction")
    return reasons

def load_breakaway():
    result = {joint: [] for joint in range(1, 7)}
    paths = sorted(BREAKAWAY_RUNS.glob("breakaway_*.json")) + sorted(J1_BREAKAWAY_RUNS.glob("breakaway_*.json"))
    for path in paths:
        item = json.loads(path.read_text(encoding="utf-8"))
        if item.get("detected"):
            expected_breakaway_collector = (ROOT / "run_j1_breakaway_collection.py"
                if path.parent.name == "runs_j1_breakaway" else BREAKAWAY_COLLECTOR)
            if (item.get("urdf_sha256") != sha256(URDF) or
                    item.get("protocol_sha256") != expected_protocol_sha256(expected_breakaway_collector) or
                    item.get("collector_script_sha256") != sha256(expected_breakaway_collector) or
                    item.get("base_collector_script_sha256") != sha256(BASE_COLLECTOR) or
                    item.get("breakaway_implementation_script_sha256") != sha256(BREAKAWAY_IMPLEMENTATION) or
                    str(item.get("schema_version")) != LOG_SCHEMA_VERSION or item.get("device_time_unit") != DEVICE_TIME_UNIT):
                raise RuntimeError(f"{path}: breakaway protocol/collector/schema provenance is stale; recollect it")
            samples = item.get("samples", [])
            required = {"device_time_s", "seq", "errors", "q_rad", "dq_rad_s", "tau_measured_nm", "gravity_nm"}
            if not samples or not required.issubset(samples[0]):
                raise RuntimeError(f"{path}: legacy/incomplete breakaway samples; recollect it")
            if any(int(sample["errors"]) != 0 for sample in samples):
                raise RuntimeError(f"{path}: firmware error is present in breakaway samples")
            sequences = [int(sample["seq"]) for sample in samples]
            if any(seq == 0 for seq in sequences) or len(sequences) != len(set(sequences)):
                raise RuntimeError(f"{path}: zero or duplicate breakaway feedback sequence number")
            sequence_delta = np.diff(np.asarray(sequences, dtype=np.int64))
            dropped_frames = int(np.sum(np.maximum(sequence_delta - 1, 0)))
            maximum_dropped = max(1, int(0.001 * len(samples)))
            if np.any(sequence_delta <= 0) or (len(sequence_delta) and int(np.max(sequence_delta)) > 2) or dropped_frames > maximum_dropped:
                raise RuntimeError(f"{path}: reordered or excessive dropped breakaway feedback frames")
            times = np.asarray([float(sample["device_time_s"]) for sample in samples])
            gaps = np.diff(times)
            if np.any(gaps <= 0.0) or (len(gaps) and float(np.max(gaps)) > MAX_LOG_GAP_S):
                raise RuntimeError(f"{path}: nonmonotonic breakaway time or feedback gap exceeds {MAX_LOG_GAP_S:g}s")
            vectors = np.asarray([[*sample["q_rad"], *sample["dq_rad_s"],
                *sample["tau_measured_nm"], *sample["gravity_nm"]] for sample in samples], dtype=float)
            if vectors.shape[1] != 24 or not np.all(np.isfinite(vectors)):
                raise RuntimeError(f"{path}: malformed or non-finite breakaway state")
            candidate = item.get("breakaway_candidate") or {}
            command_probe = float(candidate.get("command_probe_tau_nm", np.nan))
            measured_residual = float(candidate.get("measured_residual_tau_nm", np.nan))
            gravity_load = float(candidate.get("gravity_load_nm", np.nan))
            if not np.isfinite([command_probe, measured_residual, gravity_load]).all() or int(np.sign(measured_residual)) != int(item["direction"]):
                raise RuntimeError(f"{path}: invalid measured breakaway residual/sign")
            if abs(measured_residual - command_probe) > max(0.5, 0.5 * abs(command_probe)):
                raise RuntimeError(f"{path}: commanded and measured breakaway residual disagree excessively")
            if not np.isclose(float(item["breakaway_tau_nm"]), measured_residual):
                raise RuntimeError(f"{path}: breakaway summary does not use first-motion measured residual")
            result[int(item["joint"])].append({"path": str(path.relative_to(ROOT)), "direction": int(item["direction"]),
                "repeat": int(item["repeat"]),
                "support_posture": item.get("support_posture", "legacy_unspecified"),
                "urdf_sha256": item["urdf_sha256"], "protocol_sha256": item["protocol_sha256"],
                "breakaway_tau_nm": float(item["breakaway_tau_nm"]),
                "gravity_load_nm": gravity_load,
                "absolute_gravity_load_nm": abs(gravity_load),
                "estimated_dropped_frames": dropped_frames})
    return result

def split_breakaway(items):
    train = [x for x in items if x["repeat"] != HELD_OUT_REPEAT]
    heldout = [x for x in items if x["repeat"] == HELD_OUT_REPEAT]
    bad_train = [x["path"] for x in train if x["support_posture"] != "train"]
    bad_heldout = [x["path"] for x in heldout if x["support_posture"] != "heldout"]
    if bad_train or bad_heldout:
        raise RuntimeError(
            "breakaway support-posture provenance does not match repeat partition: "
            f"bad_train={bad_train}, bad_heldout={bad_heldout}"
        )
    return train, heldout

def fit_model(name, velocity, torque, breakaway=()):
    if name == "symmetric":
        predictor, x0, bounds = pred_symmetric, [1.0, .2, 0.0], ([0, 0, -20], [100, 20, 20])
        names = ["fv_nm_per_rad_s", "fc_nm", "offset_nm"]
    elif name == "asymmetric":
        predictor, x0 = pred_asymmetric, [1.0, .2, 1.0, .2, 0.0]
        bounds = ([0, 0, 0, 0, -20], [100, 20, 100, 20, 20])
        names = ["fv_pos", "fc_pos", "fv_neg", "fc_neg", "offset_nm"]
    else:
        predictor, x0 = pred_stribeck, [1.0, .2, .1, .04, 0.0]
        bounds = ([0, 0, 0, .003, -20], [100, 20, 30, .5, 20])
        names = ["fv_nm_per_rad_s", "fc_nm", "fs_nm", "vs_rad_s", "offset_nm"]
        if breakaway:
            velocity = np.concatenate((velocity, np.asarray([item["direction"] * 1e-5 for item in breakaway])))
            torque = np.concatenate((torque, np.asarray([item["breakaway_tau_nm"] for item in breakaway])))
    if name == "stribeck":
        # Optimize a nonnegative static-friction increment so Fs >= Fc by construction.
        def physical(p): return np.asarray([p[0], p[1], p[1] + p[2], p[3], p[4]])
        solved = least_squares(lambda p: predictor(physical(p), velocity) - torque, x0, bounds=bounds, loss="soft_l1")
        parameters = physical(solved.x)
    else:
        solved = least_squares(lambda p: predictor(p, velocity) - torque, x0, bounds=bounds, loss="soft_l1")
        parameters = solved.x
    return predictor, parameters, {key: float(value) for key, value in zip(names, parameters)}

def bootstrap_intervals(name, velocity, torque, breakaway, *, seed):
    """Trial-level nonparametric uncertainty; each point is one complete sweep."""
    rng = np.random.default_rng(seed); draws = []
    for _ in range(BOOTSTRAP_SAMPLES):
        indices = rng.integers(0, len(velocity), len(velocity))
        sampled_breakaway = []
        for direction in (-1, 1):
            group = [item for item in breakaway if item["direction"] == direction]
            if group:
                sampled_breakaway.extend(group[index] for index in rng.integers(0, len(group), len(group)))
        try:
            _, parameters, named = fit_model(name, velocity[indices], torque[indices], sampled_breakaway)
        except (ValueError, RuntimeError):
            continue
        if np.all(np.isfinite(parameters)):
            draws.append([named[key] for key in named])
    if len(draws) < max(100, BOOTSTRAP_SAMPLES // 2):
        raise RuntimeError(f"{name}: only {len(draws)} successful bootstrap fits")
    values = np.asarray(draws); keys = list(named)
    return {"method": "trial-level dynamic bootstrap plus direction-stratified breakaway bootstrap", "requested": BOOTSTRAP_SAMPLES,
        "successful": len(draws), "parameters": {key: {
            "median": float(np.median(values[:, index])),
            "ci95": np.percentile(values[:, index], [2.5, 97.5]).tolist(),
        } for index, key in enumerate(keys)}}

def grouped_training_cv(train, breakaway):
    """Select models using training repeats only; never inspect support-posture holdout."""
    repeats = sorted({item["repeat"] for item in train})
    if len(repeats) < 2:
        raise RuntimeError("model selection needs at least two training repeats")
    results = {}
    for name in ("symmetric", "asymmetric", "stribeck"):
        if name == "stribeck":
            eligible, reasons = stribeck_eligibility(train, breakaway)
            if not eligible:
                results[name] = {"eligible": False, "reasons": reasons}
                continue
        dynamic_actual, dynamic_predicted = [], []
        breakaway_actual, breakaway_predicted = [], []
        for held_repeat in repeats:
            fold_train = [x for x in train if x["repeat"] != held_repeat]
            fold_test = [x for x in train if x["repeat"] == held_repeat]
            fold_breakaway = [x for x in breakaway if x["repeat"] != held_repeat]
            fold_breakaway_test = [x for x in breakaway if x["repeat"] == held_repeat]
            if name == "stribeck":
                eligible, reasons = stribeck_eligibility(fold_train, fold_breakaway)
                if not eligible:
                    results[name] = {"eligible": False, "reasons": [f"CV fold {held_repeat}: {reason}" for reason in reasons]}
                    break
            velocity = np.asarray([x["speed"] for x in fold_train])
            torque = np.asarray([x["torque"] for x in fold_train])
            validation_velocity = np.asarray([x["speed"] for x in fold_test])
            validation_torque = np.asarray([x["torque"] for x in fold_test])
            predictor, parameters, _ = fit_model(name, velocity, torque, fold_breakaway)
            dynamic_actual.append(validation_torque)
            dynamic_predicted.append(predictor(parameters, validation_velocity))
            if fold_breakaway_test:
                breakaway_velocity = np.asarray([x["direction"] * 1e-5 for x in fold_breakaway_test])
                breakaway_torque = np.asarray([x["breakaway_tau_nm"] for x in fold_breakaway_test])
                breakaway_actual.append(breakaway_torque)
                breakaway_predicted.append(predictor(parameters, breakaway_velocity))
        else:
            dynamic_metrics = metrics(np.concatenate(dynamic_actual), np.concatenate(dynamic_predicted))
            if breakaway_actual:
                static_metrics = metrics(np.concatenate(breakaway_actual), np.concatenate(breakaway_predicted))
                combined_actual = np.concatenate((*dynamic_actual, *breakaway_actual))
                combined_predicted = np.concatenate((*dynamic_predicted, *breakaway_predicted))
            else:
                static_metrics = None
                combined_actual = np.concatenate(dynamic_actual)
                combined_predicted = np.concatenate(dynamic_predicted)
            results[name] = {"eligible": True, "dynamic_metrics": dynamic_metrics,
                "breakaway_metrics": static_metrics,
                "selection_metrics": metrics(combined_actual, combined_predicted)}
    eligible = [name for name, result in results.items() if result.get("eligible")]
    if not eligible:
        raise RuntimeError("no friction model is eligible for training-only selection")
    return min(eligible, key=lambda name: results[name]["selection_metrics"]["rmse_nm"]), results

def main():
    trials = load_trials()
    breakaway = load_breakaway()
    payload = {"model_formulae": {"symmetric": "tau=fv*v+fc*sign(v)+offset",
        "asymmetric": "positive and negative fv/fc fitted independently plus common offset",
        "stribeck": "tau=fv*v+sign(v)*(fc+(fs-fc)*exp(-(abs(v)/vs)^2))+offset"},
        "rigid_body_subtraction": "tau_feedback - Pinocchio RNEA(q_measured,dq_measured,ddq_SG_from_device_time)",
        "urdf": str(URDF), "urdf_sha256": sha256(URDF), "held_out_repeat": HELD_OUT_REPEAT,
        "warning": None if all(len(breakaway[joint]) >= 2 for joint in range(2, 7)) else
        "Stribeck fs/vs remain provisional until at least two training breakaway tests per J2-J6 are collected", "joints": {}}
    joints_to_fit = list(range(2, 7))
    if trials[1]:
        joints_to_fit.insert(0, 1)
        payload["j1_evidence"] = "independent relative-to-startup friction sweeps"
    else:
        payload["j1_evidence"] = "not independently collected; controller must use FIGAROH dynamic fallback"
    for joint in joints_to_fit:
        items = trials[joint]; train, test = split_trials(items)
        if len(train) < 8 or len(test) < 4: raise RuntimeError(f"J{joint}: need completed repeats; train={len(train)}, heldout={len(test)}")
        vt = np.asarray([x["speed"] for x in train]); tt = np.asarray([x["torque"] for x in train])
        vv = np.asarray([x["speed"] for x in test]); tv = np.asarray([x["torque"] for x in test])
        breakaway_train, breakaway_heldout = split_breakaway(breakaway[joint])
        best, selection = grouped_training_cv(train, breakaway_train)
        models = {}
        for name in ("symmetric", "asymmetric", "stribeck"):
            if not selection[name].get("eligible"):
                models[name] = {"eligible": False, "reasons": selection[name]["reasons"]}
                continue
            predictor, parameters, named = fit_model(name, vt, tt, breakaway_train)
            uncertainty = bootstrap_intervals(name, vt, tt, breakaway_train,
                seed=20260829 + 10 * joint + ("symmetric", "asymmetric", "stribeck").index(name))
            heldout_breakaway_metrics = None
            if breakaway_heldout:
                bv = np.asarray([item["direction"] * 1e-5 for item in breakaway_heldout])
                bt = np.asarray([item["breakaway_tau_nm"] for item in breakaway_heldout])
                heldout_breakaway_metrics = metrics(bt, predictor(parameters, bv))
            models[name] = {"eligible": True, "parameters": named, "train": metrics(tt, predictor(parameters, vt)),
                "heldout": metrics(tv, predictor(parameters, vv)), "heldout_breakaway": heldout_breakaway_metrics,
                "uncertainty": uncertainty}
        if best == "stribeck":
            identifiability_reasons = stribeck_uncertainty_reasons(models["stribeck"])
            if identifiability_reasons:
                raise RuntimeError(f"J{joint}: Stribeck won grouped CV but is not identifiable: {identifiability_reasons}")
        payload["joints"][f"J{joint}"] = {"completed_trials": len(items), "train_trials": len(train), "heldout_trials": len(test),
            "selection_rule": "minimum grouped-CV RMSE across training repeats only",
            "best_by_training_grouped_cv": best, "training_grouped_cv": selection,
            "models": models, "trial_summaries": items, "breakaway_summaries": breakaway[joint]}
    temporary = OUTPUT.with_suffix(".json.tmp"); temporary.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    with temporary.open("rb") as stream: os.fsync(stream.fileno())
    temporary.replace(OUTPUT)
    print(json.dumps({j: {"best": x["best_by_training_grouped_cv"], "heldout": x["models"][x["best_by_training_grouped_cv"]]["heldout"]} for j, x in payload["joints"].items()}, indent=2))
    print("Wrote", OUTPUT)

if __name__ == "__main__": main()
