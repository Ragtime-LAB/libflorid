"""Resume-safe single-joint constant-speed friction collection for Willow."""

from __future__ import annotations

import csv
import functools
import hashlib
import json
import os
import random
import sys
import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
ENTRY_SCRIPT = Path(__file__).resolve()
LOG_SCHEMA_VERSION = 3
DEVICE_TIME_UNIT = "s"
_LOCAL_URDF = ROOT.parent / "static_gravity_calibration" / "model" / "identified" / "Ragtime_Willow.static-mass-com-calibrated.urdf"
URDF = _LOCAL_URDF if _LOCAL_URDF.exists() else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_static_mass_com_calibrated.urdf"
_LOCAL_COLLISION_URDF = ROOT / "model" / "Ragtime_Willow_description.urdf"
COLLISION_URDF = (_LOCAL_COLLISION_URDF if _LOCAL_COLLISION_URDF.exists()
                  else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_description.urdf")
OUTPUT_DIR = ROOT / "runs"
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True

CONFIRMATION_PHRASE = "ENABLE WILLOW FRICTION CALIBRATION"

ACTIVE_JOINTS = (1, 2, 3, 4, 5)  # zero-based J2..J6
RELATIVE_SWEEP_JOINTS = ()  # optional J1 wrapper uses offsets around measured startup q1
SPEED_LEVELS_DEG_S = (2.0, 5.0, 10.0, 20.0)
REPEATS = 3
HELD_OUT_REPEAT = 2
ORDER_SEED = 20260829
# Per-axis parks avoid the real J2 ~11.9 Nm plateau seen in the 200-pose log.
# J1 is always replaced by its measured startup value.
PARK_BY_JOINT_DEG = {
    0: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
    1: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
    2: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
    3: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
    4: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
    5: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]),
}
# Repeat 3 is the held-out set and uses a different support posture.  The
# active joint follows the same path, so the test measures load/posture
# generalization rather than merely replay repeatability.
VALIDATION_PARK_BY_JOINT_DEG = {
    0: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
    1: np.array([0.0, 70.0, 80.0, 0.0, 0.0, 0.0]),
    2: np.array([0.0, 60.0, 90.0, 0.0, 0.0, 0.0]),
    3: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
    4: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
    5: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
}
SWEEP_DEG = {
    0: (-20.0, 20.0),
    1: (30.0, 90.0),
    2: (45.0, 100.0),
    3: (-50.0, 50.0),
    # J5 ±50 deg is collision-free but falls to 0.91 mm link3-link5
    # clearance in this park. ±10 deg keeps the independently audited 5 mm margin.
    4: (-10.0, 10.0),
    5: (-50.0, 50.0),
}
KP = np.array([30.0, 60.0, 60.0, 50.0, 40.0, 18.0], dtype=np.float32)
KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
MOVE_SPEED_DEG_S = 30.0
SETTLE_SECONDS = 1.0
RECORD_MARGIN_FRACTION = 0.15
STATE_TIMEOUT_S = 0.08
J1_GRAVITY_ZERO = True
OBSERVED_J2_PLATEAU_NM = 11.9
MAX_CONSECUTIVE_J2_PLATEAU_FRAMES = 10
LIMIT_MARGIN_DEG = 2.0
LOWER_DEG = np.array([-170., 5., 5., -70., -85., -85.])
UPPER_DEG = np.array([170., 175., 175., 70., 85., 85.])
TAU_LIMIT_NM = np.array([10., 28., 28., 10., 10., 10.])
MIN_TRANSITION_CLEARANCE_M = 0.005
MAX_STARTUP_RECOVERY_FRACTION = 0.10
# The description mesh is reliable on the audited sweep/park postures but the
# hardware's folded encoder startup pose uses installation-specific zero
# offsets and can appear penetrating in that model.  Do not use that one
# unmatched pose as a collision gate.  The first move is still required to
# withdraw monotonically from the limit margin; all later transitions retain
# the 5 mm HPP-FCL gate.
CHECK_STARTUP_RECOVERY_COLLISION = False
PREFLIGHT_ALL_PENDING_TRANSITIONS = False  # each move is still audited just-in-time
ENFORCE_RUNTIME_SOFTWARE_LIMITS = False
COLLISION_CONTEXT = None
MAX_TRIAL_RECOVERY_ATTEMPTS = 3
RECONNECT_ATTEMPTS = 15
RECONNECT_DELAY_S = 2.0
RECOVERY_HOLD_S = 0.5
RECOVERY_LOG = OUTPUT_DIR / "recovery_events.jsonl"

# These 14 files were fsynced by the immediately preceding collector.  The
# excitation, units, URDF and CSV schema are unchanged; only runtime recovery
# was added afterwards.  Keep this tuple exact so unrelated/stale data cannot
# silently enter a resumed experiment.
COMPATIBLE_RESUME_PROVENANCE = {
    (
        "d24fbe3839599a6a097188bae8cd47ce8083f27deef1341ae3086bf0d5a45bd1",
        "a264f9f6df4fae96803cb1057b5fedc2d14ddccb52b4eb699c12be7c2ccc333e",
        "a264f9f6df4fae96803cb1057b5fedc2d14ddccb52b4eb699c12be7c2ccc333e",
    ),
}

COLUMNS = (
    "trial_id", "repeat", "support_posture", "joint", "direction", "target_speed_rad_s", "elapsed_s",
    "schema_version", "device_time_unit", "urdf_sha256", "protocol_sha256",
    "collector_script_sha256", "base_collector_script_sha256",
    "host_time_s", "device_time", "seq", "errors",
    *(f"q{i}_rad" for i in range(1, 7)),
    *(f"dq{i}_rad_s" for i in range(1, 7)),
    *(f"tau{i}_nm" for i in range(1, 7)),
    *(f"gravity{i}_nm" for i in range(1, 7)),
)


@functools.lru_cache(maxsize=4)
def file_sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


@functools.lru_cache(maxsize=1)
def protocol_sha256():
    payload = {
        "active_joints_zero_based": ACTIVE_JOINTS, "relative_sweep_joints": RELATIVE_SWEEP_JOINTS,
        "speed_levels_deg_s": SPEED_LEVELS_DEG_S,
        "repeats": REPEATS, "held_out_repeat": HELD_OUT_REPEAT,
        "park_by_joint_deg": {str(k): v.tolist() for k, v in PARK_BY_JOINT_DEG.items()},
        "validation_park_by_joint_deg": {str(k): v.tolist() for k, v in VALIDATION_PARK_BY_JOINT_DEG.items()},
        "sweep_deg": {str(k): list(v) for k, v in SWEEP_DEG.items()},
        "kp": KP.tolist(), "kd": KD.tolist(), "move_speed_deg_s": MOVE_SPEED_DEG_S,
        "record_margin_fraction": RECORD_MARGIN_FRACTION, "lower_deg": LOWER_DEG.tolist(),
        "upper_deg": UPPER_DEG.tolist(), "tau_limit_nm": TAU_LIMIT_NM.tolist(),
        "collision_urdf_sha256": file_sha256(COLLISION_URDF),
        "check_startup_recovery_collision": CHECK_STARTUP_RECOVERY_COLLISION,
        "preflight_all_pending_transitions": PREFLIGHT_ALL_PENDING_TRANSITIONS,
        "schema_version": LOG_SCHEMA_VERSION, "device_time_unit": DEVICE_TIME_UNIT,
    }
    return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def trials() -> list[tuple[int, int, float, int]]:
    by_joint = {joint: [] for joint in ACTIVE_JOINTS}
    trial_id = 0
    for joint in ACTIVE_JOINTS:
        for speed_deg_s in SPEED_LEVELS_DEG_S:
            for direction in (-1, 1):
                for repeat in range(REPEATS):
                    by_joint[joint].append((trial_id, joint, direction * np.deg2rad(speed_deg_s), repeat))
                    trial_id += 1
    result = []
    for joint in ACTIVE_JOINTS:
        # Keep each active joint as one contiguous experiment block, as the
        # operator requested. Randomize direction/speed/repeat only inside the
        # block to reduce drift bias without repeatedly reconfiguring supports.
        training = [item for item in by_joint[joint] if item[3] != HELD_OUT_REPEAT]
        heldout = [item for item in by_joint[joint] if item[3] == HELD_OUT_REPEAT]
        random.Random(ORDER_SEED + joint).shuffle(training)
        random.Random(ORDER_SEED + 100 + joint).shuffle(heldout)
        result.extend(training + heldout)
    return result


def read_valid(control, timeout_s: float = STATE_TIMEOUT_S, last_seq=None):
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        state = control.read_once()
        if int(state.seq) != 0 and (last_seq is None or int(state.seq) != int(last_seq)):
            return state
        time.sleep(0.0005)
    raise TimeoutError("no valid ArmState")


def append_recovery_event(trial_id, attempt, error, status):
    event = {
        "host_time_s": time.time(), "trial_id": int(trial_id),
        "attempt": int(attempt), "error": repr(error), "status": status,
    }
    with RECOVERY_LOG.open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(event, ensure_ascii=False) + "\n")
        stream.flush(); os.fsync(stream.fileno())


def recoverable_error(error):
    message = str(error).lower()
    return isinstance(error, (TimeoutError, OSError, IOError)) or any(token in message for token in (
        "firmware errors=", "usb", "input/output", "i/o error", "no valid armstate",
        "timeout", "read_once", "write_once", "device", "disconnected",
    ))


def best_effort_disable(arm):
    if arm is None:
        return
    try:
        arm.disable()
    except Exception as error:
        print(f"  disable during recovery failed: {error}")


def clear_firmware_faults(arm):
    """Use the SDK/firmware recovery command, not merely disable/enable."""
    arm.automatic_error_recovery()
    time.sleep(1.0)


def restart_mit_session(arm, model, data):
    clear_firmware_faults(arm)
    arm.enable()
    control = arm.start_joint_mit_control()
    if control is None:
        raise RuntimeError("start_joint_mit_control returned None during recovery")
    state = read_valid(control, 2.0)
    held = np.asarray(state.q, dtype=float)
    last_seq = int(state.seq)
    deadline = time.monotonic() + RECOVERY_HOLD_S
    while time.monotonic() < deadline:
        state, _, _ = cycle(control, model, data, held, np.zeros(6), last_seq)
        last_seq = int(state.seq)
    return control


def acquire_startup_state(arm):
    """Clear latched faults and recover a readable passive startup session."""
    last_error = None
    for attempt in range(1, RECONNECT_ATTEMPTS + 1):
        try:
            clear_firmware_faults(arm)
            state = read_valid(arm, 2.0)
            if int(state.errors) != 0:
                raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
            print(f"startup fault recovery/read succeeded on attempt {attempt}")
            return arm, state
        except Exception as error:
            last_error = error
            print(f"startup recovery {attempt}/{RECONNECT_ATTEMPTS} failed: {error}")
            best_effort_disable(arm)
            try:
                arm = pyflorid.Arm.create(DEVICE_URI)
                if arm is None:
                    raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
            except Exception as create_error:
                last_error = create_error
                print(f"  startup Arm.create failed: {create_error}")
            time.sleep(RECONNECT_DELAY_S)
    raise RuntimeError(
        f"startup fault recovery/reconnect exhausted; verify usbipd attachment: {last_error}") from last_error


def recover_arm_session(arm, model, data, trial_id, attempt, error):
    """Clear firmware faults first, then rebuild USB/Arm if the session is dead."""
    append_recovery_event(trial_id, attempt, error, "started")
    print(f"  recoverable failure; recovery {attempt}/{MAX_TRIAL_RECOVERY_ATTEMPTS}: {error}")
    best_effort_disable(arm)
    time.sleep(1.0)

    # Fast path used by the static-gravity collector: same USB session,
    # disable -> enable -> recreate MIT control -> briefly hold measured q.
    try:
        control = restart_mit_session(arm, model, data)
        append_recovery_event(trial_id, attempt, error, "same-session-recovered")
        print("  MIT session recovered; retrying the same trial")
        return arm, control
    except Exception as same_session_error:
        print(f"  same-session recovery failed: {same_session_error}")
        best_effort_disable(arm)

    # Slow path for USB/I/O loss. Arm.create is retried for a bounded period;
    # if usbipd itself detached, the final message tells the operator what is
    # still required and a later process restart resumes from fsynced trials.
    last_error = None
    for reconnect in range(1, RECONNECT_ATTEMPTS + 1):
        try:
            replacement = pyflorid.Arm.create(DEVICE_URI)
            if replacement is None:
                raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
            control = restart_mit_session(replacement, model, data)
            append_recovery_event(trial_id, attempt, error, f"usb-reconnected-{reconnect}")
            print(f"  USB/Arm session reconnected on attempt {reconnect}; retrying the same trial")
            return replacement, control
        except Exception as reconnect_error:
            last_error = reconnect_error
            best_effort_disable(locals().get("replacement"))
            print(f"  reconnect {reconnect}/{RECONNECT_ATTEMPTS} failed: {reconnect_error}")
            time.sleep(RECONNECT_DELAY_S)
    append_recovery_event(trial_id, attempt, last_error, "reconnect-exhausted")
    raise RuntimeError(
        f"automatic USB reconnect exhausted; verify usbipd attach and rerun to resume: {last_error}") from last_error


def gravity(model, data, q: np.ndarray) -> np.ndarray:
    result = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
    if J1_GRAVITY_ZERO:
        result[0] = 0.0
    if result.shape != (6,) or not np.all(np.isfinite(result)) or np.any(np.abs(result) > TAU_LIMIT_NM):
        raise RuntimeError(f"gravity command invalid or outside effective MIT range: {result}")
    return result


def command(q: np.ndarray, dq: np.ndarray, tau: np.ndarray):
    cmd = pyflorid.JointMIT()
    cmd.q = np.asarray(q, dtype=np.float32)
    cmd.dq = np.asarray(dq, dtype=np.float32)
    cmd.tau = np.asarray(tau, dtype=np.float32)
    cmd.kp = KP
    cmd.kd = KD
    cmd.firmware_gravity = False
    return cmd


def _inside_limit_margin(q_deg):
    return np.all(q_deg > LOWER_DEG + LIMIT_MARGIN_DEG) and np.all(q_deg < UPPER_DEG - LIMIT_MARGIN_DEG)


def _valid_limit_recovery(q_deg, target_deg, recovery_start_deg, require_target_safe=True):
    """Allow only monotonic withdrawal from an encoder position outside the margin."""
    low = LOWER_DEG + LIMIT_MARGIN_DEG; high = UPPER_DEG - LIMIT_MARGIN_DEG
    if require_target_safe and not _inside_limit_margin(target_deg):
        return False
    for index in range(6):
        if recovery_start_deg[index] <= low[index]:
            if q_deg[index] < recovery_start_deg[index] - 0.5 or target_deg[index] <= recovery_start_deg[index]:
                return False
        elif recovery_start_deg[index] >= high[index]:
            if q_deg[index] > recovery_start_deg[index] + 0.5 or target_deg[index] >= recovery_start_deg[index]:
                return False
        elif not (low[index] < q_deg[index] < high[index]) or not (low[index] < target_deg[index] < high[index]):
            return False
    return True


def cycle(control, model, data, target_q: np.ndarray, target_dq: np.ndarray, last_seq=None,
          recovery_start_deg=None):
    state = read_valid(control, last_seq=last_seq)
    if int(state.errors) != 0:
        raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
    q = np.asarray(state.q, dtype=float); dq = np.asarray(state.dq, dtype=float); tau = np.asarray(state.tau, dtype=float)
    target_q = np.asarray(target_q, dtype=float); target_dq = np.asarray(target_dq, dtype=float)
    if any(value.shape != (6,) for value in (q, dq, tau, target_q, target_dq)) or not np.all(np.isfinite(np.r_[q, dq, tau, target_q, target_dq])):
        raise RuntimeError("non-finite or malformed state/reference")
    q_deg = np.rad2deg(q)
    target_deg = np.rad2deg(target_q)
    outside = not _inside_limit_margin(q_deg)
    if ENFORCE_RUNTIME_SOFTWARE_LIMITS and outside and (recovery_start_deg is None or
                    not _valid_limit_recovery(q_deg, target_deg, np.asarray(recovery_start_deg),
                                              require_target_safe=False)):
        raise RuntimeError(f"measured joint entered {LIMIT_MARGIN_DEG:g} deg software-limit margin: {np.round(q_deg, 2)}")
    if outside and not ENFORCE_RUNTIME_SOFTWARE_LIMITS:
        now = time.monotonic()
        last_warning = getattr(cycle, "_last_limit_warning", 0.0)
        if now - last_warning >= 1.0:
            print(f"  WARNING: measured q outside former software margin; continuing: {np.round(q_deg, 2)}")
            cycle._last_limit_warning = now
    g = gravity(model, data, q)
    control.write_once(command(target_q, target_dq, g))
    return state, q, g


def build_collision_context(_dynamics_model=None):
    # The calibrated URDF above is the authoritative dynamics model, but its
    # mesh paths are relative to the original description package.  Collision
    # checking deliberately uses the self-contained description URDF that was
    # already validated by preview_hppfcl.py.  Keep the two Pinocchio models
    # separate: they share the same six joint coordinates but serve different
    # purposes and may have different inertial parameters.
    collision_model = pin.buildModelFromUrdf(str(COLLISION_URDF))
    geometry = pin.buildGeomFromUrdf(
        collision_model, str(COLLISION_URDF), pin.GeometryType.COLLISION,
        [str(COLLISION_URDF.parent)])
    geometry.addAllCollisionPairs()
    if not geometry.geometryObjects or not geometry.collisionPairs:
        raise RuntimeError("URDF lacks collision geometry")
    if collision_model.nq != 6:
        raise RuntimeError(f"collision model expected nq=6, got {collision_model.nq}")
    return collision_model, geometry, collision_model.createData(), pin.GeometryData(geometry)


def audit_transition(model, start, target, allow_start_recovery=False):
    if COLLISION_CONTEXT is None:
        raise RuntimeError("collision context was not initialized")
    collision_model, geometry, model_data, geometry_data = COLLISION_CONTEXT
    steps = max(2, int(np.ceil(np.max(np.abs(np.rad2deg(target - start))))) + 1)
    minimum = float("inf"); closest = None; safe_clearance_reached = False; safe_reached_sample = None
    start_deg = np.rad2deg(start); target_deg = np.rad2deg(target)
    if not _inside_limit_margin(target_deg):
        raise RuntimeError("move target is inside software-limit margin")
    if not allow_start_recovery and not _inside_limit_margin(start_deg):
        raise RuntimeError("move transition starts inside software-limit margin")
    if allow_start_recovery and not _valid_limit_recovery(start_deg, target_deg, start_deg):
        raise RuntimeError("startup transition does not monotonically withdraw from the software-limit margin")
    for sample, q in enumerate(np.linspace(start, target, steps)):
        q_deg = np.rad2deg(q)
        if not allow_start_recovery and not _inside_limit_margin(q_deg):
            raise RuntimeError("move transition crosses software-limit margin")
        pin.computeDistances(collision_model, model_data, geometry, geometry_data, q)
        configuration_minimum = float("inf"); configuration_pair = None
        for index, pair in enumerate(geometry.collisionPairs):
            first = geometry.geometryObjects[pair.first]; second = geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                continue
            distance = float(geometry_data.distanceResults[index].min_distance)
            if distance < configuration_minimum:
                configuration_minimum = distance; configuration_pair = (first.name, second.name)
        if allow_start_recovery:
            if not safe_clearance_reached and configuration_minimum >= MIN_TRANSITION_CLEARANCE_M:
                safe_clearance_reached = True; safe_reached_sample = sample
            if safe_clearance_reached:
                if configuration_minimum < MIN_TRANSITION_CLEARANCE_M:
                    raise RuntimeError("startup recovery re-entered the collision-clearance margin")
                if configuration_minimum < minimum:
                    minimum = configuration_minimum
                    closest = (sample, configuration_pair[0], configuration_pair[1])
        elif configuration_minimum < minimum:
            minimum = configuration_minimum; closest = (sample, configuration_pair[0], configuration_pair[1])
    if allow_start_recovery:
        maximum_recovery_sample = max(1, int(np.ceil((steps - 1) * MAX_STARTUP_RECOVERY_FRACTION)))
        if not safe_clearance_reached or safe_reached_sample > maximum_recovery_sample:
            raise RuntimeError(
                f"startup recovery did not reach {MIN_TRANSITION_CLEARANCE_M*1000:g}mm clearance "
                f"within {MAX_STARTUP_RECOVERY_FRACTION*100:g}% of the move; sample={safe_reached_sample}")
    if not np.isfinite(minimum) or minimum < MIN_TRANSITION_CLEARANCE_M:
        raise RuntimeError(f"move transition clearance {minimum:.6g}m below {MIN_TRANSITION_CLEARANCE_M:g}m; closest={closest}")
    return minimum


def move(control, model, data, target: np.ndarray) -> None:
    state = read_valid(control)
    last_seq = int(state.seq)
    start = np.asarray(state.q, dtype=float)
    recovery_start_deg = np.rad2deg(start) if not _inside_limit_margin(np.rad2deg(start)) else None
    if recovery_start_deg is not None and not CHECK_STARTUP_RECOVERY_COLLISION:
        if not _valid_limit_recovery(np.rad2deg(start), np.rad2deg(target), np.rad2deg(start)):
            raise RuntimeError("startup move does not monotonically withdraw into the safe scan region")
        print("  startup collision gate bypassed for installation-specific folded encoder pose")
    else:
        print(f"  transition minimum clearance={audit_transition(model, start, target, recovery_start_deg is not None) * 1000:.3f} mm")
    if recovery_start_deg is not None:
        print(f"  guarded startup limit recovery from q_deg={np.round(recovery_start_deg, 3)}")
    duration = max(0.5, float(np.max(np.abs(target - start))) / np.deg2rad(MOVE_SPEED_DEG_S))
    begin = time.monotonic()
    while True:
        elapsed = time.monotonic() - begin
        alpha = min(1.0, elapsed / duration)
        desired = start + alpha * (target - start)
        state, _, _ = cycle(control, model, data, desired, np.zeros(6), last_seq,
                            recovery_start_deg=recovery_start_deg)
        last_seq = int(state.seq)
        if alpha >= 1.0:
            break
    recovered_deg = np.rad2deg(np.asarray(state.q, dtype=float))
    if (ENFORCE_RUNTIME_SOFTWARE_LIMITS and recovery_start_deg is not None and
            not _inside_limit_margin(recovered_deg)):
        raise RuntimeError(f"startup recovery failed to enter safe limit region: {np.round(recovered_deg, 2)}")
    until = time.monotonic() + SETTLE_SECONDS
    while time.monotonic() < until:
        state, _, _ = cycle(control, model, data, target, np.zeros(6), last_seq)
        last_seq = int(state.seq)


def write_row(writer, trial_id, repeat, joint, velocity, elapsed, state, q, g):
    source_timestamp_us = int(getattr(state, "source_timestamp_us", 0))
    device_time_s = source_timestamp_us * 1e-6 if source_timestamp_us > 0 else float(state.time) * 1e-3
    row = {
        "trial_id": trial_id, "repeat": repeat, "joint": joint + 1,
        "support_posture": "heldout" if repeat == 2 else "train",
        "schema_version": LOG_SCHEMA_VERSION, "device_time_unit": DEVICE_TIME_UNIT,
        "urdf_sha256": file_sha256(URDF), "protocol_sha256": protocol_sha256(),
        "collector_script_sha256": file_sha256(ENTRY_SCRIPT),
        "base_collector_script_sha256": file_sha256(Path(__file__).resolve()),
        "direction": int(np.sign(velocity)), "target_speed_rad_s": velocity,
        "elapsed_s": elapsed, "host_time_s": time.time(),
        "device_time": device_time_s, "seq": int(state.seq), "errors": int(state.errors),
    }
    for prefix, values in (("q", q), ("dq", state.dq), ("tau", state.tau), ("gravity", g)):
        suffix = {"q": "_rad", "dq": "_rad_s", "tau": "_nm", "gravity": "_nm"}[prefix]
        row.update({f"{prefix}{i}{suffix}": float(value) for i, value in enumerate(values, 1)})
    writer.writerow(row)


def run_trial(control, model, data, trial_id: int, joint: int, velocity: float, repeat: int, j1: float) -> None:
    parks = VALIDATION_PARK_BY_JOINT_DEG if repeat == 2 else PARK_BY_JOINT_DEG
    park = np.deg2rad(parks[joint])
    park[0] = j1
    low, high = np.deg2rad(SWEEP_DEG[joint])
    if joint in RELATIVE_SWEEP_JOINTS:
        low += j1; high += j1
    start = park.copy(); end = park.copy()
    start[joint], end[joint] = (low, high) if velocity > 0 else (high, low)
    duration = abs(high - low) / abs(velocity)
    move(control, model, data, start)

    final_path = OUTPUT_DIR / f"trial_{trial_id:03d}_J{joint + 1}_{np.rad2deg(velocity):+g}deg_s.csv"
    temp_path = final_path.with_suffix(".csv.tmp")
    with temp_path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=COLUMNS)
        writer.writeheader()
        begin = time.monotonic(); last_seq = None; plateau_frames = 0; recorded = 0
        while True:
            elapsed = time.monotonic() - begin
            alpha = min(1.0, elapsed / duration)
            target = start + alpha * (end - start)
            target_dq = np.zeros(6); target_dq[joint] = velocity
            state, q, g = cycle(control, model, data, target, target_dq, last_seq)
            last_seq = int(state.seq)
            plateau_frames = plateau_frames + 1 if abs(float(state.tau[1])) >= OBSERVED_J2_PLATEAU_NM else 0
            if plateau_frames >= MAX_CONSECUTIVE_J2_PLATEAU_FRAMES:
                raise RuntimeError("J2 feedback torque stayed at the observed 11.9 Nm plateau")
            if RECORD_MARGIN_FRACTION <= alpha <= 1.0 - RECORD_MARGIN_FRACTION:
                write_row(writer, trial_id, repeat, joint, velocity, elapsed, state, q, g)
                recorded += 1
            if alpha >= 1.0:
                break
        file.flush(); os.fsync(file.fileno())
    temp_path.replace(final_path)
    if hasattr(os, "O_DIRECTORY"):
        directory_fd = os.open(OUTPUT_DIR, os.O_RDONLY | os.O_DIRECTORY)
        try: os.fsync(directory_fd)
        finally: os.close(directory_fd)
    print(f"  unique feedback frames recorded={recorded}, mean recorded rate={recorded / max(duration * (1 - 2 * RECORD_MARGIN_FRACTION), 1e-9):.1f} Hz")


def trial_endpoints(joint: int, velocity: float, repeat: int, j1: float):
    parks = VALIDATION_PARK_BY_JOINT_DEG if repeat == HELD_OUT_REPEAT else PARK_BY_JOINT_DEG
    park = np.deg2rad(parks[joint]); park[0] = j1
    low, high = np.deg2rad(SWEEP_DEG[joint])
    if joint in RELATIVE_SWEEP_JOINTS:
        low += j1; high += j1
    start = park.copy(); end = park.copy()
    start[joint], end[joint] = (low, high) if velocity > 0 else (high, low)
    return start, end


def preflight_pending_transitions(model, initial_q, pending, j1):
    current = np.asarray(initial_q, dtype=float); minimum = float("inf")
    for index, (_, joint, velocity, repeat) in enumerate(pending):
        start, end = trial_endpoints(joint, velocity, repeat, j1)
        startup_recovery = index == 0 and not _inside_limit_margin(np.rad2deg(current))
        if startup_recovery and not CHECK_STARTUP_RECOVERY_COLLISION:
            if not _valid_limit_recovery(np.rad2deg(current), np.rad2deg(start), np.rad2deg(current)):
                raise RuntimeError("startup preflight does not monotonically withdraw into the safe scan region")
            first_leg_minimum = float("inf")
        else:
            first_leg_minimum = audit_transition(model, current, start,
                                                 allow_start_recovery=startup_recovery)
        minimum = min(minimum, first_leg_minimum, audit_transition(model, start, end))
        current = end
    return minimum


def main() -> None:
    global COLLISION_CONTEXT
    if "--print-protocol-sha256" in sys.argv:
        print(protocol_sha256())
        return
    model = pin.buildModelFromUrdf(str(URDF)); data = model.createData()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    current_protocol = protocol_sha256(); current_urdf = file_sha256(URDF); current_script = file_sha256(ENTRY_SCRIPT)
    current_base_script = file_sha256(Path(__file__).resolve())
    completed = set()
    for path in OUTPUT_DIR.glob("trial_*.csv"):
        with path.open(newline="", encoding="utf-8") as stream:
            first = next(csv.DictReader(stream), None)
        if not first:
            raise RuntimeError(f"empty existing trial blocks resume: {path}")
        provenance = (first.get("protocol_sha256"), first.get("collector_script_sha256"),
                      first.get("base_collector_script_sha256"))
        current_provenance = (current_protocol, current_script, current_base_script)
        provenance_ok = provenance == current_provenance or provenance in COMPATIBLE_RESUME_PROVENANCE
        if (not provenance_ok or first.get("urdf_sha256") != current_urdf or
                first.get("schema_version") != str(LOG_SCHEMA_VERSION) or
                first.get("device_time_unit") != DEVICE_TIME_UNIT):
            raise RuntimeError(f"stale/incompatible existing trial blocks resume; archive and recollect: {path}")
        if provenance != current_provenance:
            print(f"compatible pre-recovery trial accepted: {path.name}")
        completed.add(int(first["trial_id"]))
    pending = [(i, j, v, r) for i, j, v, r in trials() if i not in completed]
    print("URDF:", URDF)
    print("SHA256:", hashlib.sha256(URDF.read_bytes()).hexdigest())
    print("MIT: host Pinocchio gravity, firmware_gravity=False")
    print(f"trials total={len(trials())}, complete={len(trials()) - len(pending)}, pending={len(pending)}")
    pure_sweep_s = sum(abs(SWEEP_DEG[joint][1] - SWEEP_DEG[joint][0]) / abs(np.rad2deg(speed))
                       for _, joint, speed, _ in trials())
    print(f"planned pure constant-speed sweep time={pure_sweep_s/60:.1f} min; transitions/settling are additional")
    for i, j, v, r in pending:
        print(f"  trial {i:03d}: J{j + 1} {np.rad2deg(v):+g} deg/s repeat={r + 1}/{REPEATS}")
    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY: set ENABLE_HARDWARE=True after review")
        return

    COLLISION_CONTEXT = build_collision_context(model)

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    arm, initial = acquire_startup_state(arm)
    j1 = float(np.asarray(initial.q)[0])
    print("startup q_deg:", np.round(np.rad2deg(np.asarray(initial.q)), 3))
    if pending and PREFLIGHT_ALL_PENDING_TRANSITIONS:
        minimum = preflight_pending_transitions(model, np.asarray(initial.q, dtype=float), pending, j1)
        print(f"complete pending transition preflight minimum clearance={minimum * 1000:.3f} mm")
    elif pending:
        first_start, _ = trial_endpoints(pending[0][1], pending[0][2], pending[0][3], j1)
        if not _valid_limit_recovery(np.rad2deg(np.asarray(initial.q)), np.rad2deg(first_start),
                                     np.rad2deg(np.asarray(initial.q))):
            raise RuntimeError("startup pose cannot monotonically withdraw to the first scan posture")
        print("startup recovery checked; later transitions will be collision-audited just-in-time")
    phrase = input(f'Type exactly "{CONFIRMATION_PHRASE}" while ready to stop the arm: ')
    if phrase != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch; hardware remains disabled")
    control = None
    try:
        arm.enable()
        control = arm.start_joint_mit_control()
        if control is None:
            raise RuntimeError("start_joint_mit_control returned None")
        for index, (trial_id, joint, velocity, repeat) in enumerate(pending, 1):
            print(f"[{index}/{len(pending)}] trial={trial_id:03d} J{joint + 1} {np.rad2deg(velocity):+g} deg/s repeat={repeat + 1}/{REPEATS}")
            recovery_attempt = 0
            while True:
                try:
                    run_trial(control, model, data, trial_id, joint, velocity, repeat, j1)
                    break
                except Exception as error:
                    if not recoverable_error(error):
                        raise
                    recovery_attempt += 1
                    if recovery_attempt > MAX_TRIAL_RECOVERY_ATTEMPTS:
                        append_recovery_event(trial_id, recovery_attempt, error, "trial-recovery-exhausted")
                        raise RuntimeError(
                            f"trial {trial_id:03d} recovery exhausted after "
                            f"{MAX_TRIAL_RECOVERY_ATTEMPTS} attempts") from error
                    arm, control = recover_arm_session(
                        arm, model, data, trial_id, recovery_attempt, error)
            print("  saved+fsynced")
    finally:
        best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
