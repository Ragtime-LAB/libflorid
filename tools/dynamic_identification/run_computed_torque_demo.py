#!/usr/bin/env python3
"""Guarded pure-torque computed-control demo using measured q/dq and Pinocchio."""
from __future__ import annotations

import json
import hashlib
import os
import time
import multiprocessing as mp
import queue
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
_LOCAL_URDF = ROOT.parent / "static_gravity_calibration/model/identified/Ragtime_Willow.static-mass-com-calibrated.urdf"
URDF = _LOCAL_URDF if _LOCAL_URDF.exists() else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_static_mass_com_calibrated.urdf"
DYNAMIC_URDF = ROOT / "Ragtime_Willow_identified.urdf"
BUNDLE = ROOT / "controller_bundle.json"
_LOCAL_DESIGN = ROOT / "dynamic_validation_design.npz"
DESIGN = _LOCAL_DESIGN if _LOCAL_DESIGN.exists() else ROOT.parents[1] / "data/dynamic_validation_design.npz"
FRICTION_FIT = ROOT.parent / "friction_calibration/friction_fit.json"
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = False
# Set true only for a deliberately supervised, physically contact-free run
# intended for residual-threshold calibration.  It is recorded, not inferred.
KNOWN_CONTACT_FREE_RUN = False
SPEED_SCALE = 0.30
TRACKING_JOINTS = (1, 2, 3, 4, 5, 6)
CONFIRMATION_PHRASE = "RUN GUARDED WILLOW COMPUTED TORQUE"
KP_ACC = np.array([36., 36., 49., 64., 64., 81.])
KD_ACC = np.array([10., 10., 12., 14., 14., 16.])
CONFIGURED_TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.])
# Current Willow firmware configures J2/J3 as J4340 and leaves the other
# joints on DmMotor's J4310 default. float_to_uint saturates at these ranges.
MIT_ENCODING_LIMIT_NM = np.array([10., 28., 28., 10., 10., 10.])
TAU_LIMIT_NM = np.minimum(CONFIGURED_TAU_LIMIT_NM, MIT_ENCODING_LIMIT_NM)
TAU_SLEW_NM_S = np.array([100., 100., 100., 40., 40., 40.])
CORRECTION_RAMP_S = 1.5
FRICTION_DIRECTION_VEL_RAD_S = 0.01
MAX_FOLLOWING_ERROR_DEG = np.array([15., 15., 15., 18., 18., 20.])
LOWER_DEG = np.array([-170., 5., 5., -70., -85., -85.])
UPPER_DEG = np.array([170., 175., 175., 70., 85., 85.])
LOG_DIR = ROOT / "runs_computed_torque"
MIN_CONTROL_RATE_HZ = 300.0
RATE_WINDOW_S = 1.0
STATE_TIMEOUT_S = 0.08
MAX_INTERFRAME_S = 0.020
MAX_STATE_AGE_MS = 20.0
MAX_ESTIMATED_LATENCY_MS = 20.0
MAX_RECEIVE_JITTER_US = 5000.0
MIN_TRANSITION_CLEARANCE_M = 0.005
RUNTIME_COLLISION_MONITOR_HZ = 10.0
MIN_RUNTIME_MEASURED_CLEARANCE_M = 0.003
MAX_RUNTIME_COLLISION_RESULT_AGE_S = 0.5
ROBUST_LAYER_ENABLED = False
REQUIRE_PLAIN_REPORT_FOR_ROBUST = False
REQUIRE_PRIOR_PLAIN_SPEED_SCALE = None
REQUIRE_PRIOR_STAGE_ID = "j2_plain_0p20"
REQUIRE_PRIOR_TRACKING_JOINTS = (2,)
REQUIRE_KNOWN_PAYLOAD_TORQUE_VALIDATION = True
KNOWN_PAYLOAD_VALIDATION = ROOT / "known_payload_torque_validation.json"
ST_LAMBDA = np.array([10., 10., 12., 14., 14., 16.])
ST_K1 = np.array([1.0, 1.2, 1.4, 1.5, 1.5, 1.6])
ST_K2 = np.array([0.5, 0.7, 0.8, 0.9, 0.9, 1.0])
ST_BOUNDARY = np.deg2rad(np.array([3., 3., 3., 4., 4., 5.]))
ST_INTEGRAL_LIMIT = np.array([3., 3., 4., 4., 4., 5.])
ST_ACCEL_LIMIT = np.array([4., 4., 5., 5., 5., 6.])
ST_WEIGHT = 0.30
ENDPOINT_BLEND_S = 2.0
STOP_KD_NM_PER_RAD_S = np.array([0.8, 1.4, 1.2, 0.8, 0.6, 0.5])
STOP_MAX_DQ_DEG_S = 1.0
STOP_STABLE_S = 0.5
STOP_TIMEOUT_S = 5.0


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def stage_id():
    joint_label = "six_axis" if tuple(TRACKING_JOINTS) == (1, 2, 3, 4, 5, 6) else "j" + "_".join(map(str, TRACKING_JOINTS))
    controller_label = "robust" if ROBUST_LAYER_ENABLED else "plain"
    speed_label = f"{SPEED_SCALE:.2f}".replace(".", "p")
    return f"{joint_label}_{controller_label}_{speed_label}"


def validate_runtime_design_contract(bundle, design_hash):
    """Recheck safety-critical scalar fields instead of trusting passed=true."""
    audit = bundle.get("dynamic_design_safety_audit", {})
    validation = audit.get("designs", {}).get("validation", {})
    required_flags = ("passed", "position_limits_passed", "velocity_limits_passed",
                      "acceleration_limits_passed", "nominal_torque_limits_passed",
                      "collision_free_every_sample", "clearance_passed")
    if (not audit.get("passed") or validation.get("sha256") != design_hash or
            not all(validation.get(name) is True for name in required_flags)):
        raise ValueError("validation trajectory lacks a complete safety audit")
    required_clearance = float(validation.get("minimum_required_clearance_m", np.nan))
    actual_clearance = float(validation.get("minimum_nonadjacent_distance_m", np.nan))
    if (not np.isfinite(required_clearance) or required_clearance < MIN_TRANSITION_CLEARANCE_M or
            not np.isfinite(actual_clearance) or actual_clearance < required_clearance):
        raise ValueError("validation trajectory clearance contract is missing or insufficient")
    expected_effective = np.asarray(audit.get("effective_torque_limit_nm", []), dtype=float)
    expected_interface = np.asarray(
        bundle.get("torque_interface", {}).get("effective_command_range_nm", []), dtype=float)
    if (expected_effective.shape != (6,) or expected_interface.shape != (6,) or
            not np.array_equal(expected_effective, TAU_LIMIT_NM) or
            not np.array_equal(expected_interface, TAU_LIMIT_NM)):
        raise ValueError("bundle torque range differs from the runtime MIT encoding contract")
    return validation


def validate_controller_artifacts():
    if not BUNDLE.exists() or not DYNAMIC_URDF.exists():
        raise FileNotFoundError("identified URDF and gated controller_bundle.json are required for real torque control")
    bundle = json.loads(BUNDLE.read_text(encoding="utf-8"))
    if bundle.get("schema_version") != 2 or bundle.get("joint_order") != [f"J{i}" for i in range(1, 7)]:
        raise ValueError("unsupported controller bundle schema or joint order")
    if bundle.get("urdf_sha256") != sha256(DYNAMIC_URDF):
        raise ValueError("identified URDF hash differs from the gated controller bundle")
    validate_runtime_design_contract(bundle, sha256(DESIGN))
    sources = bundle.get("source_sha256", {})
    if sources.get("dynamic_validation_design") != sha256(DESIGN):
        raise ValueError("controller bundle validation-design provenance is missing or stale")
    if (not KNOWN_PAYLOAD_VALIDATION.exists() or
            sources.get("known_payload_torque_validation") != sha256(KNOWN_PAYLOAD_VALIDATION)):
        raise ValueError("known-payload torque validation is missing or differs from the gated bundle")
    return bundle


def read_valid(reader, timeout=STATE_TIMEOUT_S, last_seq=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) != 0 and (last_seq is None or int(state.seq) != int(last_seq)):
            return state
        time.sleep(0.0002)
    raise TimeoutError("no valid ArmState")


def checked_state(state, target=None):
    if int(state.errors):
        raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
    q = np.asarray(state.q, dtype=float)
    dq = np.asarray(state.dq, dtype=float)
    tau = np.asarray(state.tau, dtype=float)
    if q.shape != (6,) or dq.shape != (6,) or tau.shape != (6,) or not np.all(np.isfinite(np.r_[q, dq, tau])):
        raise RuntimeError("non-finite or malformed measured state")
    q_deg = np.rad2deg(q)
    if np.any(q_deg <= LOWER_DEG) or np.any(q_deg >= UPPER_DEG):
        raise RuntimeError("measured joint crossed software limit")
    if target is not None:
        target = np.asarray(target, dtype=float)
        if target.shape != (6,) or not np.all(np.isfinite(target)):
            raise RuntimeError("non-finite or malformed target")
        target_deg = np.rad2deg(target)
        if np.any(target_deg <= LOWER_DEG) or np.any(target_deg >= UPPER_DEG):
            raise RuntimeError("target crossed software limit")
        error_deg = np.abs(np.rad2deg(target - q))
        if np.any(error_deg > MAX_FOLLOWING_ERROR_DEG):
            raise RuntimeError(f"following error exceeded: {np.round(error_deg, 2)} deg")
    return q, dq, tau


def checked_command_torque(tau, label="command"):
    tau = np.asarray(tau, dtype=float)
    if tau.shape != (6,) or not np.all(np.isfinite(tau)):
        raise RuntimeError(f"{label} torque is non-finite or malformed")
    if np.any(np.abs(tau) > TAU_LIMIT_NM):
        raise RuntimeError(f"{label} torque exceeds effective MIT limits: {np.round(tau, 3)} Nm")
    return tau


def audit_start_transition(model, urdf_path, start, target):
    """Audit the actual measured start-to-reference bridge, not just the design."""
    start = np.asarray(start, dtype=float); target = np.asarray(target, dtype=float)
    steps = max(2, int(np.ceil(np.max(np.abs(np.rad2deg(target - start))))) + 1)
    geometry = pin.buildGeomFromUrdf(model, str(urdf_path), pin.GeometryType.COLLISION, [str(Path(urdf_path).parent)])
    geometry.addAllCollisionPairs()
    if not geometry.geometryObjects or not geometry.collisionPairs:
        raise RuntimeError("identified URDF has no collision geometry for start-transition audit")
    model_data = model.createData(); geometry_data = pin.GeometryData(geometry)
    minimum = float("inf"); closest = None
    for sample, q in enumerate(np.linspace(start, target, steps)):
        target_deg = np.rad2deg(q)
        if np.any(target_deg <= LOWER_DEG) or np.any(target_deg >= UPPER_DEG):
            raise RuntimeError("start transition crosses a software limit")
        pin.computeDistances(model, model_data, geometry, geometry_data, q)
        for pair_index, pair in enumerate(geometry.collisionPairs):
            first = geometry.geometryObjects[pair.first]; second = geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                continue
            distance = float(geometry_data.distanceResults[pair_index].min_distance)
            if distance < minimum:
                minimum = distance; closest = (sample, first.name, second.name)
    if not np.isfinite(minimum) or minimum < MIN_TRANSITION_CLEARANCE_M:
        raise RuntimeError(f"start transition clearance {minimum:.6g}m below {MIN_TRANSITION_CLEARANCE_M:g}m; closest={closest}")
    return {"samples": steps, "minimum_distance_m": minimum, "closest": closest}


def audit_reference_trajectory(model, urdf_path, configurations):
    """Re-audit the complete runtime reference after applying the measured J1 offset."""
    geometry = pin.buildGeomFromUrdf(model, str(urdf_path), pin.GeometryType.COLLISION, [str(Path(urdf_path).parent)])
    geometry.addAllCollisionPairs()
    if not geometry.geometryObjects or not geometry.collisionPairs:
        raise RuntimeError("identified URDF has no collision geometry for runtime-reference audit")
    model_data = model.createData(); geometry_data = pin.GeometryData(geometry)
    minimum = float("inf"); closest = None
    for sample, q in enumerate(np.asarray(configurations, dtype=float)):
        q_deg = np.rad2deg(q)
        if np.any(q_deg <= LOWER_DEG) or np.any(q_deg >= UPPER_DEG):
            raise RuntimeError(f"runtime reference sample {sample} crosses a software limit")
        pin.computeDistances(model, model_data, geometry, geometry_data, q)
        for pair_index, pair in enumerate(geometry.collisionPairs):
            first = geometry.geometryObjects[pair.first]; second = geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                continue
            distance = float(geometry_data.distanceResults[pair_index].min_distance)
            if distance < minimum:
                minimum = distance; closest = (sample, first.name, second.name)
    if not np.isfinite(minimum) or minimum < MIN_TRANSITION_CLEARANCE_M:
        raise RuntimeError(f"runtime reference clearance {minimum:.6g}m below {MIN_TRANSITION_CLEARANCE_M:g}m; closest={closest}")
    return {"samples": len(configurations), "minimum_distance_m": minimum, "closest": closest}


def _collision_monitor_worker(urdf_path, input_queue, output_queue, stop_event):
    """Compute mesh distance out of process so the 500 Hz loop never blocks on HPP-FCL."""
    try:
        model = pin.buildModelFromUrdf(str(urdf_path))
        geometry = pin.buildGeomFromUrdf(model, str(urdf_path), pin.GeometryType.COLLISION,
            [str(Path(urdf_path).parent)])
        geometry.addAllCollisionPairs()
        adjacent_pairs = [pair for pair in geometry.collisionPairs
            if abs(int(geometry.geometryObjects[pair.first].parentJoint) -
                   int(geometry.geometryObjects[pair.second].parentJoint)) <= 1]
        for pair in adjacent_pairs:
            geometry.removeCollisionPair(pair)
        model_data = model.createData(); geometry_data = pin.GeometryData(geometry)
        while not stop_event.is_set():
            try:
                timestamp, configuration = input_queue.get(timeout=0.1)
                while True:
                    timestamp, configuration = input_queue.get_nowait()
            except queue.Empty:
                if "configuration" not in locals():
                    continue
            pin.computeDistances(model, model_data, geometry, geometry_data, np.asarray(configuration, dtype=float))
            minimum = float("inf"); closest = None
            for pair_index, pair in enumerate(geometry.collisionPairs):
                first = geometry.geometryObjects[pair.first]; second = geometry.geometryObjects[pair.second]
                if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                    continue
                distance = float(geometry_data.distanceResults[pair_index].min_distance)
                if distance < minimum:
                    minimum = distance; closest = (first.name, second.name)
            result = (float(timestamp), time.monotonic(), minimum, closest, None)
            try:
                output_queue.put_nowait(result)
            except queue.Full:
                try: output_queue.get_nowait()
                except queue.Empty: pass
                output_queue.put_nowait(result)
            del configuration
    except BaseException as exc:
        try: output_queue.put_nowait((0.0, time.monotonic(), np.nan, None, f"{type(exc).__name__}: {exc}"))
        except queue.Full: pass


class RuntimeCollisionMonitor:
    def __init__(self, urdf_path):
        context = mp.get_context("fork")
        self.input = context.Queue(maxsize=1); self.output = context.Queue(maxsize=1)
        self.stop_event = context.Event()
        self.process = context.Process(target=_collision_monitor_worker,
            args=(str(urdf_path), self.input, self.output, self.stop_event), daemon=True)
        self.process.start(); self.last_submit = 0.0; self.latest = None

    def submit_and_check(self, configuration, *, force=False, allow_stale=False):
        now = time.monotonic()
        if force or now - self.last_submit >= 1.0 / RUNTIME_COLLISION_MONITOR_HZ:
            try: self.input.put_nowait((now, np.asarray(configuration, dtype=float).copy()))
            except queue.Full: pass
            self.last_submit = now
        try:
            while True: self.latest = self.output.get_nowait()
        except queue.Empty:
            pass
        if not self.process.is_alive() and (self.latest is None or self.latest[4] is None):
            raise RuntimeError("runtime collision monitor process stopped")
        if self.latest is not None:
            source_time, completed_time, clearance, closest, error = self.latest
            if error:
                raise RuntimeError(f"runtime collision monitor failed: {error}")
            if not allow_stale and now - source_time > MAX_RUNTIME_COLLISION_RESULT_AGE_S:
                raise RuntimeError(f"runtime collision result is stale by {now-source_time:.3f}s")
            if not np.isfinite(clearance) or clearance < MIN_RUNTIME_MEASURED_CLEARANCE_M:
                raise RuntimeError(f"measured clearance {clearance:.6g}m below runtime threshold; closest={closest}")
            return float(clearance)
        return None

    def wait_initial(self, configuration, timeout=3.0):
        deadline = time.monotonic() + timeout
        warmed = False
        while time.monotonic() < deadline:
            result = self.submit_and_check(configuration, force=True, allow_stale=True)
            if result is not None:
                if warmed:
                    # The first mesh query may build BVHs and take much longer
                    # than steady state.  Require a second fresh result before
                    # the arm can be enabled.
                    source_time = self.latest[0]
                    if time.monotonic() - source_time <= MAX_RUNTIME_COLLISION_RESULT_AGE_S:
                        return result
                else:
                    warmed = True
                self.latest = None; self.last_submit = 0.0
            time.sleep(0.01)
        raise TimeoutError("no initial runtime collision result")

    def close(self):
        self.stop_event.set(); self.process.join(timeout=1.0)
        if self.process.is_alive(): self.process.terminate(); self.process.join(timeout=1.0)


def friction_models(bundle_payload=None):
    if bundle_payload is not None:
        payload = bundle_payload; result = {}
        j1 = payload["figaroh_joint1_friction_fallback"]
        result[1] = {"best_by_training_grouped_cv": "symmetric", "models": {"symmetric": {"parameters": j1}}}
        for name, item in payload["independent_friction_models_J2_to_J6"].items():
            joint = int(name[1:]); model = item["selected_model"]
            result[joint] = {"best_by_training_grouped_cv": model, "models": {model: {"parameters": item["parameters"]}}}
        expected_sources = payload.get("friction_source_by_joint")
        if expected_sources is not None:
            actual_independent = {f"J{joint}" for joint in result if joint != 1 or "J1" in payload["independent_friction_models_J2_to_J6"]}
            declared_independent = {name for name, source in expected_sources.items() if source.startswith("independent")}
            if actual_independent != declared_independent:
                raise ValueError(f"friction source declaration mismatch: actual={actual_independent}, declared={declared_independent}")
        return result
    if not FRICTION_FIT.exists():
        return {}
    payload = json.loads(FRICTION_FIT.read_text(encoding="utf-8"))
    return {int(name[1:]): item for name, item in payload.get("joints", {}).items()}


def friction_torque(dq, models, direction_hint=None):
    result = np.zeros(6)
    hint = np.asarray(dq if direction_hint is None else direction_hint, dtype=float)
    for joint, item in models.items():
        model = item["best_by_training_grouped_cv"]
        p = item["models"][model]["parameters"]
        v = float(dq[joint - 1])
        motion_hint = v if abs(v) >= FRICTION_DIRECTION_VEL_RAD_S else float(hint[joint - 1])
        direction = float(np.tanh(motion_hint / FRICTION_DIRECTION_VEL_RAD_S))
        if model == "symmetric":
            value = p["fv_nm_per_rad_s"] * v + p["fc_nm"] * direction + p["offset_nm"]
        elif model == "asymmetric":
            viscous = p["fv_pos"] * v if direction >= 0 else p["fv_neg"] * v
            coulomb = (p["fc_pos"] if direction >= 0 else p["fc_neg"]) * direction
            value = viscous + coulomb + p["offset_nm"]
        else:
            magnitude = p["fc_nm"] + (p["fs_nm"] - p["fc_nm"]) * np.exp(-(abs(v) / p["vs_rad_s"]) ** 2)
            value = p["fv_nm_per_rad_s"] * v + direction * magnitude + p["offset_nm"]
        result[joint - 1] = value
    return result


def mit_command(q, dq, tau, kp, kd):
    command = pyflorid.JointMIT()
    command.q = np.asarray(q, dtype=np.float32)
    command.dq = np.asarray(dq, dtype=np.float32)
    command.kp = np.asarray(kp, dtype=np.float32)
    command.kd = np.asarray(kd, dtype=np.float32)
    command.tau = np.asarray(tau, dtype=np.float32)
    command.firmware_gravity = False
    return command


def move_to_start(control, model, data, target, collision_monitor=None):
    state = read_valid(control)
    start, _, _ = checked_state(state)
    duration = max(1.0, float(np.max(np.abs(target - start))) / np.deg2rad(20.0))
    kp = np.array([30., 60., 60., 50., 40., 18.])
    kd = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2])
    begin = time.monotonic()
    while True:
        alpha = min(1.0, (time.monotonic() - begin) / duration)
        desired = start + alpha * (target - start)
        state = read_valid(control)
        q, _, _ = checked_state(state, desired)
        if collision_monitor is not None: collision_monitor.submit_and_check(q)
        gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q)); gravity[0] = 0.0
        gravity = checked_command_torque(gravity, "move-to-start gravity")
        checked_command_torque(gravity + kp * (desired - q) - kd * np.asarray(state.dq, dtype=float),
            "estimated move-to-start total MIT")
        control.write_once(mit_command(desired, np.zeros(6), gravity, kp, kd))
        if alpha >= 1.0:
            break
    until = time.monotonic() + 1.0
    while time.monotonic() < until:
        state = read_valid(control)
        q, _, _ = checked_state(state, target)
        if collision_monitor is not None: collision_monitor.submit_and_check(q)
        gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q)); gravity[0] = 0.0
        gravity = checked_command_torque(gravity, "start-hold gravity")
        checked_command_torque(gravity + kp * (target - q) - kd * np.asarray(state.dq, dtype=float),
            "estimated start-hold total MIT")
        control.write_once(mit_command(target, np.zeros(6), gravity, kp, kd))


def atomic_save_npz(path, **arrays):
    path = Path(path); path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(path.suffix + ".tmp")
    with temp.open("wb") as stream:
        np.savez_compressed(stream, **arrays); stream.flush(); os.fsync(stream.fileno())
    temp.replace(path)


def limit_slew(requested, previous, dt, rate_limit=TAU_SLEW_NM_S):
    maximum_delta = np.asarray(rate_limit, dtype=float) * max(float(dt), 0.0)
    return np.asarray(previous, dtype=float) + np.clip(np.asarray(requested) - previous, -maximum_delta, maximum_delta)


def interpolate_reference(source_time, source_t, q_ref, dq_ref, ddq_ref):
    """Quintic Hermite interpolation with mutually consistent q/dq/ddq."""
    t = np.asarray(source_t, dtype=float)
    if source_time <= t[0]:
        return q_ref[0].copy(), dq_ref[0].copy(), ddq_ref[0].copy()
    if source_time >= t[-1]:
        return q_ref[-1].copy(), dq_ref[-1].copy(), ddq_ref[-1].copy()
    index = int(np.searchsorted(t, source_time, side="right") - 1)
    h = float(t[index + 1] - t[index]); s = float((source_time - t[index]) / h)
    a0 = q_ref[index]
    a1 = dq_ref[index] * h
    a2 = 0.5 * ddq_ref[index] * h * h
    delta_q = q_ref[index + 1] - (a0 + a1 + a2)
    delta_v = dq_ref[index + 1] * h - (a1 + 2.0 * a2)
    delta_a = ddq_ref[index + 1] * h * h - 2.0 * a2
    a3 = 10.0 * delta_q - 4.0 * delta_v + 0.5 * delta_a
    a4 = -15.0 * delta_q + 7.0 * delta_v - delta_a
    a5 = 6.0 * delta_q - 3.0 * delta_v + 0.5 * delta_a
    q = a0 + a1*s + a2*s**2 + a3*s**3 + a4*s**4 + a5*s**5
    dq = (a1 + 2*a2*s + 3*a3*s**2 + 4*a4*s**3 + 5*a5*s**4) / h
    ddq = (2*a2 + 6*a3*s + 12*a4*s**2 + 20*a5*s**3) / (h*h)
    return q, dq, ddq


def quintic_boundary_segment(q0, dq0, ddq0, q1, dq1, ddq1, duration, rate_hz=500.0):
    """Return a C2 quintic segment including both boundary states."""
    duration = float(duration)
    if duration <= 0.0:
        raise ValueError("quintic segment duration must be positive")
    samples = max(2, int(np.ceil(duration * rate_hz)) + 1)
    time_axis = np.linspace(0.0, duration, samples)
    q = np.empty((samples, 6)); dq = np.empty_like(q); ddq = np.empty_like(q)
    boundary_t = np.array([0.0, duration])
    boundary_q = np.vstack((q0, q1)); boundary_dq = np.vstack((dq0, dq1)); boundary_ddq = np.vstack((ddq0, ddq1))
    for index, sample_time in enumerate(time_axis):
        q[index], dq[index], ddq[index] = interpolate_reference(
            sample_time, boundary_t, boundary_q, boundary_dq, boundary_ddq)
    return time_axis, q, dq, ddq


def add_stationary_endpoint_blends(time_axis, q, dq, ddq, duration=ENDPOINT_BLEND_S):
    """Make runtime entry/exit C2-continuous with a stationary hold state."""
    zeros = np.zeros(6)
    pre_t, pre_q, pre_dq, pre_ddq = quintic_boundary_segment(
        q[0], zeros, zeros, q[0], dq[0], ddq[0], duration)
    post_t, post_q, post_dq, post_ddq = quintic_boundary_segment(
        q[-1], dq[-1], ddq[-1], q[-1], zeros, zeros, duration)
    main_t = np.asarray(time_axis) - float(time_axis[0]) + duration
    post_t = post_t + main_t[-1]
    return (
        np.concatenate((pre_t, main_t[1:], post_t[1:])),
        np.vstack((pre_q, q[1:], post_q[1:])),
        np.vstack((pre_dq, dq[1:], post_dq[1:])),
        np.vstack((pre_ddq, ddq[1:], post_ddq[1:])),
    )


def stop_and_confirm_rest(control, model, data, collision_monitor=None):
    """On normal completion only, hold gravity+damping until motion is stably low."""
    started = time.monotonic(); stable_since = None; last_seq = None
    while time.monotonic() - started < STOP_TIMEOUT_S:
        state = read_valid(control, last_seq=last_seq); last_seq = int(state.seq)
        q, dq, _ = checked_state(state)
        if collision_monitor is not None: collision_monitor.submit_and_check(q)
        gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q)); gravity[0] = 0.0
        tau = checked_command_torque(gravity - STOP_KD_NM_PER_RAD_S * dq, "stop gravity+damping")
        control.write_once(mit_command(np.zeros(6), np.zeros(6), tau, np.zeros(6), np.zeros(6)))
        if np.all(np.abs(np.rad2deg(dq)) <= STOP_MAX_DQ_DEG_S):
            stable_since = time.monotonic() if stable_since is None else stable_since
            if time.monotonic() - stable_since >= STOP_STABLE_S:
                return
        else:
            stable_since = None
    raise RuntimeError(f"robot did not remain below {STOP_MAX_DQ_DEG_S:g} deg/s for {STOP_STABLE_S:g}s")


def super_twisting_correction(surface, integral, dt):
    """Bounded Coucouarm-inspired robustness term in joint acceleration units."""
    smooth_direction = np.tanh(np.asarray(surface) / ST_BOUNDARY)
    updated_integral = np.clip(
        np.asarray(integral) + ST_K2 * smooth_direction * max(float(dt), 0.0),
        -ST_INTEGRAL_LIMIT, ST_INTEGRAL_LIMIT,
    )
    correction = ST_K1 * np.sqrt(np.abs(surface)) * smooth_direction + updated_integral
    return np.clip(ST_WEIGHT * correction, -ST_ACCEL_LIMIT, ST_ACCEL_LIMIT), updated_integral


def require_passing_plain_report(bundle_hash, urdf_hash, design_hash, minimum_speed=None,
                                 required_stage_id=None, required_tracking_joints=None):
    required_speed = SPEED_SCALE if minimum_speed is None else float(minimum_speed)
    reports = sorted(LOG_DIR.glob("computed_torque_*.report.json"), key=lambda p: p.stat().st_mtime, reverse=True)
    for path in reports:
        report = json.loads(path.read_text(encoding="utf-8"))
        if report.get("robust_layer_enabled"):
            continue
        report_tracking = tuple(int(joint) for joint in report.get("tracking_joints", ()))
        if (report.get("passed_logged_safety_checks") and report.get("run_completed") and
                report.get("safe_shutdown_completed") and
                report.get("bundle_sha256") == bundle_hash and report.get("urdf_sha256") == urdf_hash and
                report.get("design_sha256") == design_hash and float(report.get("speed_scale", -1)) >= required_speed and
                (required_stage_id is None or report.get("stage_id") == required_stage_id) and
                (required_tracking_joints is None or report_tracking == tuple(required_tracking_joints))):
            return path
    raise RuntimeError(f"stage requires a complete passing plain-CTC report for identical assets at speed >= {required_speed:g}")


def main():
    if not 0.0 < SPEED_SCALE <= 1.0:
        raise ValueError("SPEED_SCALE must be in (0, 1]")
    design_hash = sha256(DESIGN)
    design = np.load(DESIGN)
    required = ("t", "q", "dq", "ddq")
    if any(name not in design for name in required):
        raise ValueError(f"design must contain {required}")
    source_t = np.asarray(design["t"], dtype=float)
    reference_t = source_t / SPEED_SCALE
    controller_bundle = validate_controller_artifacts() if BUNDLE.exists() and DYNAMIC_URDF.exists() else None
    model_path = DYNAMIC_URDF if controller_bundle is not None else URDF
    loaded_asset_hashes = {"design": design_hash, "model_urdf": sha256(model_path),
                           "bundle": sha256(BUNDLE) if controller_bundle is not None else None}
    model = pin.buildModelFromUrdf(str(model_path))
    data = model.createData()
    q_ref = np.asarray(design["q"]).copy()
    dq_ref = np.asarray(design["dq"]) * SPEED_SCALE
    ddq_ref = np.asarray(design["ddq"]) * SPEED_SCALE**2
    if (source_t.ndim != 1 or len(source_t) < 2 or np.any(np.diff(source_t) <= 0) or
            any(value.shape != (len(source_t), 6) for value in (q_ref, dq_ref, ddq_ref)) or
            not np.all(np.isfinite(np.concatenate((source_t[:, None], q_ref, dq_ref, ddq_ref), axis=1)))):
        raise ValueError("dynamic design is malformed or non-finite")
    tracking_mask = np.asarray([joint in TRACKING_JOINTS for joint in range(1, 7)])
    if not np.any(tracking_mask) or any(joint not in range(1, 7) for joint in TRACKING_JOINTS):
        raise ValueError("TRACKING_JOINTS must contain one or more joints in 1..6")
    q_ref[:, ~tracking_mask] = q_ref[0, ~tracking_mask]
    dq_ref[:, ~tracking_mask] = 0.0
    ddq_ref[:, ~tracking_mask] = 0.0
    reference_t, q_ref, dq_ref, ddq_ref = add_stationary_endpoint_blends(
        reference_t, q_ref, dq_ref, ddq_ref
    )
    duration = float(reference_t[-1])
    if (not np.allclose(dq_ref[[0, -1]], 0.0, atol=1e-12) or
            not np.allclose(ddq_ref[[0, -1]], 0.0, atol=1e-12)):
        raise RuntimeError("runtime reference endpoint blending did not produce stationary C2 endpoints")
    actuator_inertia = np.zeros(6)
    if controller_bundle is not None:
        actuator_inertia = np.asarray([controller_bundle["actuator_inertia"][f"J{joint}"] for joint in range(1, 7)])
    models = friction_models(controller_bundle)
    nominal_tau = np.asarray([pin.rnea(model, data, q, dq, ddq) + actuator_inertia * ddq + friction_torque(dq, models, dq)
        for q, dq, ddq in zip(q_ref, dq_ref, ddq_ref)])
    nominal_peak = np.max(np.abs(nominal_tau), axis=0)
    if not np.all(np.isfinite(nominal_tau)) or np.any(nominal_peak > TAU_LIMIT_NM):
        raise RuntimeError(f"runtime model predicts over-range nominal torque: peak={nominal_peak}, limit={TAU_LIMIT_NM}")
    print("PURE TORQUE MODE: MIT kp=kd=0, firmware_gravity=False")
    print("controller model:", model_path)
    print("speed scale:", SPEED_SCALE, "duration_s:", round(duration, 3))
    print("tracking joints:", TRACKING_JOINTS, "others hold the initial design pose")
    print("nominal feedforward peak Nm:", np.round(nominal_peak, 3))
    print("configured firmware torque clamps Nm:", CONFIGURED_TAU_LIMIT_NM)
    print("MIT encoding ranges Nm:", MIT_ENCODING_LIMIT_NM)
    print("effective command limits Nm:", TAU_LIMIT_NM)
    print("correction/friction ramp s:", CORRECTION_RAMP_S, "torque slew Nm/s:", TAU_SLEW_NM_S)
    print("bounded super-twisting robustness layer:", ROBUST_LAYER_ENABLED)
    print("friction fit:", FRICTION_FIT if FRICTION_FIT.exists() else "not present; disabled")
    print("DISABLED preview:", not ENABLE_HARDWARE)
    print("operator contact-free attestation recorded in telemetry:", KNOWN_CONTACT_FREE_RUN)
    if not ENABLE_HARDWARE:
        return
    # A disabled preview may explain the static-model fallback. Real computed
    # torque is never allowed to use it.
    controller_bundle = validate_controller_artifacts()

    bundle_hash = sha256(BUNDLE); urdf_hash = sha256(DYNAMIC_URDF)
    if REQUIRE_PLAIN_REPORT_FOR_ROBUST:
        required_plain_stage = f"six_axis_plain_{SPEED_SCALE:.2f}".replace(".", "p")
        print("plain CTC prerequisite:", require_passing_plain_report(
            bundle_hash, urdf_hash, design_hash, required_stage_id=required_plain_stage,
            required_tracking_joints=(1, 2, 3, 4, 5, 6)))
    if REQUIRE_PRIOR_STAGE_ID is not None:
        print("prior stage prerequisite:", require_passing_plain_report(
            bundle_hash, urdf_hash, design_hash, minimum_speed=0.0,
            required_stage_id=REQUIRE_PRIOR_STAGE_ID,
            required_tracking_joints=REQUIRE_PRIOR_TRACKING_JOINTS))
    if REQUIRE_PRIOR_PLAIN_SPEED_SCALE is not None:
        if float(REQUIRE_PRIOR_PLAIN_SPEED_SCALE) >= SPEED_SCALE:
            raise ValueError("prior speed prerequisite must be lower than the requested speed")
        print("prior speed-stage prerequisite:", require_passing_plain_report(
            bundle_hash, urdf_hash, design_hash, minimum_speed=REQUIRE_PRIOR_PLAIN_SPEED_SCALE))
    if REQUIRE_KNOWN_PAYLOAD_TORQUE_VALIDATION:
        if not KNOWN_PAYLOAD_VALIDATION.exists():
            raise FileNotFoundError("next-speed control requires known_payload_torque_validation.json")
        torque_validation = json.loads(KNOWN_PAYLOAD_VALIDATION.read_text(encoding="utf-8"))
        if not torque_validation.get("passed"):
            raise RuntimeError("known-payload absolute torque validation did not pass")
        print("known-payload torque validation:", KNOWN_PAYLOAD_VALIDATION)

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    initial = read_valid(arm, 2.0)
    j1_offset = float(np.asarray(initial.q)[0])
    q_ref[:, 0] += j1_offset
    q_ref_deg = np.rad2deg(q_ref)
    if np.any(q_ref_deg <= LOWER_DEG) or np.any(q_ref_deg >= UPPER_DEG):
        raise RuntimeError(f"measured J1 offset moves the design outside software limits; q1 range={q_ref_deg[:,0].min():.2f}..{q_ref_deg[:,0].max():.2f}deg")
    initial_q, _, _ = checked_state(initial)
    print("offset runtime-reference collision audit:", audit_reference_trajectory(model, model_path, q_ref))
    print("actual start-transition collision audit:", audit_start_transition(model, model_path, initial_q, q_ref[0]))
    phrase = input(f'Type exactly "{CONFIRMATION_PHRASE}": ')
    if phrase != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")
    validate_controller_artifacts()
    current_asset_hashes = {"design": sha256(DESIGN), "model_urdf": sha256(model_path), "bundle": sha256(BUNDLE)}
    if current_asset_hashes != loaded_asset_hashes:
        raise RuntimeError(f"controller assets changed after loading; loaded={loaded_asset_hashes}, current={current_asset_hashes}")
    collision_monitor = None
    try:
        collision_monitor = RuntimeCollisionMonitor(model_path)
        initial_clearance = collision_monitor.wait_initial(initial_q)
        print(f"initial measured-pose collision clearance={initial_clearance * 1000:.3f} mm")
        arm.enable()
        control = arm.start_joint_mit_control()
        if control is None:
            raise RuntimeError("start_joint_mit_control returned None")
        move_to_start(control, model, data, q_ref[0], collision_monitor)
        begin = time.monotonic(); rate_begin = begin; rate_frames = 0; last_seq = None; last_feedback_time = None
        log = {name: [] for name in ("elapsed_s", "source_time_s", "index", "q", "dq", "q_ref", "dq_ref", "ddq_cmd", "robust_accel", "tau_raw", "tau_cmd", "tau_measured", "correction_scale", "runtime_clearance_m", "estimated_latency_ms", "state_age_ms", "receive_jitter_us", "receive_hz", "seq", "errors")}
        run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        previous_tau = None; previous_write_time = begin; st_integral = np.zeros(6); run_completed = False; termination_reason = "running"
        safe_shutdown_completed = False
        try:
          while True:
            elapsed = time.monotonic() - begin
            source_time = elapsed
            if elapsed >= reference_t[-1]:
                break
            index = min(int(np.searchsorted(reference_t, elapsed, side="right") - 1), len(reference_t) - 1)
            q_target, dq_target, ddq_target = interpolate_reference(
                elapsed, reference_t, q_ref, dq_ref, ddq_ref
            )
            state = read_valid(control, last_seq=last_seq)
            feedback_time = time.monotonic()
            if last_feedback_time is not None and feedback_time - last_feedback_time > MAX_INTERFRAME_S:
                raise RuntimeError(f"feedback gap {feedback_time - last_feedback_time:.4f}s exceeds {MAX_INTERFRAME_S:.4f}s")
            last_feedback_time = feedback_time
            last_seq = int(state.seq)
            q, dq, measured_tau = checked_state(state, q_target)
            runtime_clearance = collision_monitor.submit_and_check(q)
            error_deg = np.abs(np.rad2deg(q_target - q))
            if np.any(error_deg > MAX_FOLLOWING_ERROR_DEG):
                raise RuntimeError(f"following error exceeded: {np.round(error_deg, 2)} deg")
            correction_scale = min(1.0, elapsed / CORRECTION_RAMP_S)
            now = time.monotonic()
            robust_accel = np.zeros(6)
            if ROBUST_LAYER_ENABLED:
                surface = (dq_target - dq) + ST_LAMBDA * (q_target - q)
                robust_accel, st_integral = super_twisting_correction(
                    surface, st_integral, now - previous_write_time if previous_tau is not None else 0.0
                )
            acceleration = ddq_target + correction_scale * (
                KP_ACC * (q_target - q) + KD_ACC * (dq_target - dq) + robust_accel
            )
            friction_hint = dq_target + 2.0 * (q_target - q)
            tau_raw = np.asarray(pin.rnea(model, data, q, dq, acceleration)) + actuator_inertia * acceleration + correction_scale * friction_torque(dq, models, friction_hint)
            tau_raw = checked_command_torque(tau_raw, "computed")
            if previous_tau is None:
                previous_tau = checked_command_torque(pin.computeGeneralizedGravity(model, data, q), "initial gravity")
            tau = limit_slew(tau_raw, previous_tau, now - previous_write_time)
            tau = checked_command_torque(tau, "final MIT")
            latency_ms = float(control.estimated_latency().to_msec())
            state_age_ms = float(control.state_age().to_msec())
            jitter_us = float(control.receive_jitter_us())
            receive_hz = float(control.receive_hz())
            if not np.all(np.isfinite([latency_ms, state_age_ms, jitter_us, receive_hz])):
                raise RuntimeError("non-finite communication metrics")
            if latency_ms > MAX_ESTIMATED_LATENCY_MS or state_age_ms > MAX_STATE_AGE_MS or jitter_us > MAX_RECEIVE_JITTER_US:
                raise RuntimeError(f"communication quality exceeded limits: latency={latency_ms:.2f}ms age={state_age_ms:.2f}ms jitter={jitter_us:.1f}us")
            previous_tau = tau.copy(); previous_write_time = now
            control.write_once(mit_command(np.zeros(6), np.zeros(6), tau, np.zeros(6), np.zeros(6)))
            for name, value in (("elapsed_s", elapsed), ("source_time_s", source_time), ("index", index),
                ("q", q.copy()), ("dq", dq.copy()), ("q_ref", q_target.copy()), ("dq_ref", dq_target.copy()),
                ("ddq_cmd", acceleration.copy()), ("robust_accel", robust_accel.copy()),
                ("tau_raw", tau_raw.copy()), ("tau_cmd", tau.copy()),
                ("tau_measured", measured_tau.copy()),
                ("runtime_clearance_m", np.nan if runtime_clearance is None else runtime_clearance),
                ("estimated_latency_ms", latency_ms),
                ("state_age_ms", state_age_ms),
                ("receive_jitter_us", jitter_us),
                ("receive_hz", receive_hz),
                ("seq", int(state.seq)), ("errors", int(state.errors))):
                log[name].append(value)
            log["correction_scale"].append(correction_scale)
            rate_frames += 1
            now = time.monotonic()
            if now - rate_begin >= RATE_WINDOW_S:
                rate_hz = rate_frames / (now - rate_begin)
                print(f"control rate={rate_hz:.1f} Hz, t={elapsed:.2f}s, max error={np.max(error_deg):.2f}deg")
                if rate_hz < MIN_CONTROL_RATE_HZ:
                    raise RuntimeError(f"control rate {rate_hz:.1f} Hz below {MIN_CONTROL_RATE_HZ:g} Hz")
                rate_begin = now; rate_frames = 0
          stop_and_confirm_rest(control, model, data, collision_monitor)
          run_completed = True
          termination_reason = "reference_completed"
        except BaseException as exc:
          termination_reason = f"{type(exc).__name__}: {exc}"
          raise
    finally:
        try:
            arm.disable()
            safe_shutdown_completed = True
            print("All axes disabled.")
        finally:
            if collision_monitor is not None: collision_monitor.close()
            if "log" in locals() and log["elapsed_s"]:
                output = LOG_DIR / f"computed_torque_{run_id}.npz"
                atomic_save_npz(output, **{key: np.asarray(value) for key, value in log.items()},
                    urdf=np.asarray(str(model_path)), speed_scale=np.asarray(SPEED_SCALE),
                    robust_layer_enabled=np.asarray(ROBUST_LAYER_ENABLED),
                    tracking_joints=np.asarray(TRACKING_JOINTS, dtype=int), stage_id=np.asarray(stage_id()),
                    run_completed=np.asarray(locals().get("run_completed", False)),
                    safe_shutdown_completed=np.asarray(locals().get("safe_shutdown_completed", False)),
                    termination_reason=np.asarray(locals().get("termination_reason", "setup_failed")),
                    known_contact_free=np.asarray(KNOWN_CONTACT_FREE_RUN),
                    expected_duration_s=np.asarray(duration), design_sha256=np.asarray(design_hash),
                    urdf_sha256=np.asarray(sha256(model_path)),
                    bundle_sha256=np.asarray(sha256(BUNDLE) if BUNDLE.exists() else ""))
                print("telemetry saved:", output)


if __name__ == "__main__":
    main()
