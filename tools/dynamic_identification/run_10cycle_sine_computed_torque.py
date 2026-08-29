#!/usr/bin/env python3
"""Ten-cycle Willow joint-space sine demo with host computed torque.

The actual tracking segment sends pure torque through MIT (kp=kd=0) and turns
firmware gravity off.  Pinocchio always evaluates measured q/dq; MuJoCo or the
reference state is never used as hidden plant state.
"""
from __future__ import annotations

import hashlib
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
FIT = ROOT / "identified/fit_3cb7a96993a3.json"
FRICTION_ROOT = ROOT.parent / "friction_calibration"
sys.path.insert(0, str(FRICTION_ROOT))
import run_gravity_friction_feedforward as friction_ff

DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW 10 CYCLE COMPUTED TORQUE"

POSE_A_DEG = np.array([85.76, 124.84, 104.42, -37.06, -15.22, 128.46])
POSE_B_REQUESTED_DEG = np.array([17.89, 6.02, 0.36, 19.92, 24.25, 154.58])
POSE_B_EXECUTED_DEG = POSE_B_REQUESTED_DEG.copy()
POSE_B_EXECUTED_DEG[2] = 30.0  # previously agreed physical table-clearance rule
ROUND_TRIPS = 10
MAX_REFERENCE_SPEED_DEG_S = 90.0
RATE_HZ = 500.0
CONTROL_PERIOD_S = 1.0 / RATE_HZ
ENDPOINT_BLEND_S = 1.5

# Acceleration-space tracking gains for tau=M(q)v+C(q,dq)dq+g(q).
KP_ACC = np.array([20., 24., 24., 30., 30., 30.])
KD_ACC = np.array([8., 9., 9., 10., 10., 10.])
TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.])
MAX_FOLLOWING_ERROR_DEG = np.array([20., 20., 20., 25., 25., 25.])
STATE_TIMEOUT_S = 0.08
MAX_INTERFRAME_S = 0.020
MIN_CONTROL_RATE_HZ = 300.0
RATE_WINDOW_S = 1.0
MAX_RUN_ATTEMPTS = 3

# Soft position-mode bridge used only to reach A and return to measured startup.
MOVE_KP = np.array([30., 60., 60., 50., 40., 18.], dtype=np.float32)
MOVE_KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
MOVE_SPEED_DEG_S = 60.0
MOVE_MAX_ACCEL_DEG_S2 = 60.0
TAKEOVER_HOLD_S = 0.5
TARGET_HOLD_S = 1.0
CORRECTION_RAMP_S = 1.0
MAX_ENTRY_ERROR_DEG = 8.0
MAX_ENTRY_SPEED_DEG_S = 5.0
RETURN_TO_MEASURED_START = True
LOG_DIR = ROOT / "runs_computed_torque_10cycle"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_valid(reader, timeout=STATE_TIMEOUT_S, last_seq=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) and (last_seq is None or int(state.seq) != int(last_seq)):
            return state
        time.sleep(0.0002)
    raise TimeoutError("no valid ArmState")


def mit_command(q, dq, tau, kp=None, kd=None):
    command = pyflorid.JointMIT()
    command.q = np.asarray(q, dtype=np.float32)
    command.dq = np.asarray(dq, dtype=np.float32)
    command.tau = np.asarray(tau, dtype=np.float32)
    command.kp = np.zeros(6, dtype=np.float32) if kp is None else np.asarray(kp, dtype=np.float32)
    command.kd = np.zeros(6, dtype=np.float32) if kd is None else np.asarray(kd, dtype=np.float32)
    command.firmware_gravity = False
    return command


def checked_state(state):
    errors = int(state.errors)
    if errors:
        raise RuntimeError(f"firmware errors=0x{errors:08X}")
    q = np.asarray(state.q, dtype=float)
    dq = np.asarray(state.dq, dtype=float)
    tau = np.asarray(state.tau, dtype=float)
    if any(value.shape != (6,) for value in (q, dq, tau)) or not np.all(np.isfinite(np.r_[q, dq, tau])):
        raise RuntimeError("non-finite or malformed ArmState")
    return q, dq, tau


def checked_torque(tau, label):
    tau = np.asarray(tau, dtype=float)
    if tau.shape != (6,) or not np.all(np.isfinite(tau)):
        raise RuntimeError(f"{label} torque is malformed or non-finite")
    if np.any(np.abs(tau) > TAU_LIMIT_NM):
        raise RuntimeError(
            f"{label} exceeds configured 40/40/40/12/12/12 Nm: {np.round(tau, 3)}"
        )
    return tau


def wait_for_tick(next_tick):
    remaining = next_tick - time.monotonic()
    if remaining > 0.0:
        time.sleep(remaining)


def quintic_boundary(q0, dq0, ddq0, q1, dq1, ddq1, duration):
    samples = max(2, int(np.ceil(duration * RATE_HZ)) + 1)
    t = np.linspace(0.0, duration, samples)
    h = float(duration)
    a0 = np.asarray(q0)
    a1 = np.asarray(dq0) * h
    a2 = 0.5 * np.asarray(ddq0) * h * h
    delta_q = np.asarray(q1) - (a0 + a1 + a2)
    delta_v = np.asarray(dq1) * h - (a1 + 2.0 * a2)
    delta_a = np.asarray(ddq1) * h * h - 2.0 * a2
    a3 = 10.0 * delta_q - 4.0 * delta_v + 0.5 * delta_a
    a4 = -15.0 * delta_q + 7.0 * delta_v - delta_a
    a5 = 6.0 * delta_q - 3.0 * delta_v + 0.5 * delta_a
    s = (t / h)[:, None]
    q = a0 + a1*s + a2*s**2 + a3*s**3 + a4*s**4 + a5*s**5
    dq = (a1 + 2*a2*s + 3*a3*s**2 + 4*a4*s**3 + 5*a5*s**4) / h
    ddq = (2*a2 + 6*a3*s + 12*a4*s**2 + 20*a5*s**3) / (h*h)
    return t, q, dq, ddq


def build_reference():
    qa = np.deg2rad(POSE_A_DEG)
    qb = np.deg2rad(POSE_B_EXECUTED_DEG)
    delta = qb - qa
    # q=A+0.5*(1-cos(omega*t))*delta. Its peak speed is pi*|delta|/period.
    period = np.pi * float(np.max(np.abs(np.rad2deg(delta)))) / MAX_REFERENCE_SPEED_DEG_S
    duration = period * ROUND_TRIPS
    dt = 1.0 / RATE_HZ
    t = np.arange(0.0, duration + 0.5 * dt, dt)
    omega = 2.0 * np.pi / period
    blend = 0.5 * (1.0 - np.cos(omega * t))
    blend_d = 0.5 * omega * np.sin(omega * t)
    blend_dd = 0.5 * omega**2 * np.cos(omega * t)
    q = qa + blend[:, None] * delta
    dq = blend_d[:, None] * delta
    ddq = blend_dd[:, None] * delta

    zeros = np.zeros(6)
    pre_t, pre_q, pre_dq, pre_ddq = quintic_boundary(
        qa, zeros, zeros, q[0], dq[0], ddq[0], ENDPOINT_BLEND_S
    )
    post_t, post_q, post_dq, post_ddq = quintic_boundary(
        q[-1], dq[-1], ddq[-1], qa, zeros, zeros, ENDPOINT_BLEND_S
    )
    main_t = t + pre_t[-1]
    post_t = post_t + main_t[-1]
    return (
        np.concatenate((pre_t, main_t[1:], post_t[1:])),
        np.vstack((pre_q, q[1:], post_q[1:])),
        np.vstack((pre_dq, dq[1:], post_dq[1:])),
        np.vstack((pre_ddq, ddq[1:], post_ddq[1:])),
        period,
    )


def load_models_and_actuator_inertia():
    import json

    fit = json.loads(FIT.read_text(encoding="utf-8"))
    expected = fit["physical_reconstruction"]["candidate_urdf_sha256"]
    if expected != sha256(URDF):
        raise RuntimeError("dynamic fit and deployed candidate URDF hashes differ")
    parameters = fit["physical_reconstruction"]["full_parameters"]
    actuator_inertia = np.array([parameters[f"Ia_joint{joint}"] for joint in range(1, 7)])
    models = friction_ff.load_friction_models()
    models[1] = models[2]
    breakaway = friction_ff.load_breakaway_models()
    breakaway[1] = breakaway[2]
    # J1 uses the same actuator module as J2.
    friction_ff.FRICTION_SCALE[0] = friction_ff.FRICTION_SCALE[1]
    friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[0] = friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[1]
    friction_ff.BREAKAWAY_SCALE[0] = friction_ff.BREAKAWAY_SCALE[1]
    return models, breakaway, actuator_inertia


def friction_terms(q, dq, model, data, models, breakaway_models):
    friction = friction_ff.friction_torque(dq, models)
    gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
    breakaway = friction_ff.breakaway_torque(
        dq, gravity, models, breakaway_models
    )
    return friction, breakaway


def soft_move(control, model, data, target, models, breakaway_models):
    state = read_valid(control, 2.0)
    start, _, _ = checked_state(state)
    last_seq = int(state.seq)
    hold_until = time.monotonic() + TAKEOVER_HOLD_S
    next_tick = time.monotonic()
    while time.monotonic() < hold_until:
        wait_for_tick(next_tick)
        next_tick += CONTROL_PERIOD_S
        state = read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = checked_state(state)
        friction, breakaway = friction_terms(q, dq, model, data, models, breakaway_models)
        tau = checked_torque(pin.computeGeneralizedGravity(model, data, q) + friction + breakaway, "soft takeover")
        control.write_once(mit_command(start, np.zeros(6), tau, MOVE_KP * 0.25, MOVE_KD))

    delta = np.asarray(target) - start
    distance = float(np.max(np.abs(delta)))
    velocity_duration = 1.875 * distance / np.deg2rad(MOVE_SPEED_DEG_S)
    acceleration_duration = np.sqrt(5.8 * distance / np.deg2rad(MOVE_MAX_ACCEL_DEG_S2))
    duration = max(1.0, velocity_duration, acceleration_duration)
    begin = time.monotonic()
    next_tick = begin
    frames = 0
    while True:
        wait_for_tick(next_tick)
        next_tick += CONTROL_PERIOD_S
        u = min(1.0, (time.monotonic() - begin) / duration)
        alpha = 10*u**3 - 15*u**4 + 6*u**5
        alpha_d = (30*u**2 - 60*u**3 + 30*u**4) / duration
        desired = start + alpha * delta
        desired_dq = alpha_d * delta
        state = read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = checked_state(state)
        friction, breakaway = friction_terms(q, dq, model, data, models, breakaway_models)
        tau = checked_torque(pin.computeGeneralizedGravity(model, data, q) + friction + breakaway, "soft move")
        control.write_once(mit_command(desired, desired_dq, tau, MOVE_KP, MOVE_KD))
        frames += 1
        if u >= 1.0:
            break
    hold_begin = time.monotonic()
    next_tick = hold_begin
    while time.monotonic() - hold_begin < TARGET_HOLD_S:
        wait_for_tick(next_tick)
        next_tick += CONTROL_PERIOD_S
        state = read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = checked_state(state)
        friction, breakaway = friction_terms(q, dq, model, data, models, breakaway_models)
        tau = checked_torque(pin.computeGeneralizedGravity(model, data, q) + friction + breakaway, "target hold")
        control.write_once(mit_command(target, np.zeros(6), tau, MOVE_KP, MOVE_KD))
        frames += 1
    final_q, final_dq, _ = checked_state(state)
    final_error_deg = float(np.max(np.abs(np.rad2deg(target - final_q))))
    final_speed_deg_s = float(np.max(np.abs(np.rad2deg(final_dq))))
    print(
        f"soft transition: {duration:.2f}s + {TARGET_HOLD_S:.1f}s hold, "
        f"rate={frames/max(time.monotonic()-begin, 1e-9):.1f}Hz, "
        f"final_error={final_error_deg:.2f}deg, "
        f"final_speed={final_speed_deg_s:.2f}deg/s"
    )
    if final_error_deg > MAX_ENTRY_ERROR_DEG or final_speed_deg_s > MAX_ENTRY_SPEED_DEG_S:
        raise RuntimeError(
            f"MIT bridge not ready for torque mode: error={final_error_deg:.2f}deg, "
            f"speed={final_speed_deg_s:.2f}deg/s"
        )


def bounded_computed_torque(model, data, q, dq, ddq_ref, correction,
                            friction, breakaway, actuator_inertia, ramp):
    """Retain full model feedforward and scale only the feedback correction."""
    base_acceleration = np.asarray(ddq_ref, dtype=float)
    base_rigid = np.asarray(pin.rnea(model, data, q, dq, base_acceleration), dtype=float)
    base_actuator = actuator_inertia * base_acceleration
    base_tau = checked_torque(
        base_rigid + base_actuator + friction + breakaway, "model feedforward"
    )

    requested_scale = float(np.clip(ramp, 0.0, 1.0))

    def evaluate(scale):
        acceleration = base_acceleration + scale * correction
        rigid = np.asarray(pin.rnea(model, data, q, dq, acceleration), dtype=float)
        actuator = actuator_inertia * acceleration
        return acceleration, rigid, actuator, rigid + actuator + friction + breakaway

    acceleration, rigid, actuator, tau = evaluate(requested_scale)
    if np.any(np.abs(tau) > TAU_LIMIT_NM):
        lower, upper = 0.0, requested_scale
        for _ in range(32):
            middle = 0.5 * (lower + upper)
            _, _, _, candidate = evaluate(middle)
            if np.all(np.abs(candidate) <= 0.995 * TAU_LIMIT_NM):
                lower = middle
            else:
                upper = middle
        applied_scale = lower
        acceleration, rigid, actuator, tau = evaluate(applied_scale)
    else:
        applied_scale = requested_scale
    return (
        acceleration, rigid, actuator,
        checked_torque(tau, "bounded computed torque"), applied_scale, base_tau,
    )


def run_reference(control, model, data, reference, models, breakaway_models, actuator_inertia):
    t, q_ref, dq_ref, ddq_ref, _ = reference
    sample_period = float(np.median(np.diff(t)))
    started = time.monotonic()
    last_seq = None
    last_frame = None
    index = 0
    rate_started = started
    rate_frames = 0
    log = {name: [] for name in (
        "time_s", "seq", "errors", "q", "dq", "tau_measured", "q_ref", "dq_ref",
        "ddq_ref", "ddq_cmd", "tau_rigid", "tau_actuator", "tau_friction",
        "tau_breakaway", "tau_command", "feedback_correction_scale", "tau_model_feedforward",
    )}
    next_tick = started
    while index < len(t):
        wait_for_tick(next_tick)
        next_tick += CONTROL_PERIOD_S
        elapsed = time.monotonic() - started
        desired_index = min(int(elapsed / sample_period), len(t) - 1)
        state = read_valid(control, last_seq=last_seq)
        now = time.monotonic()
        last_seq = int(state.seq)
        if last_frame is not None and now - last_frame > MAX_INTERFRAME_S:
            raise RuntimeError(f"feedback gap {now-last_frame:.4f}s exceeds {MAX_INTERFRAME_S:.4f}s")
        last_frame = now
        q, dq, measured_tau = checked_state(state)
        qr = q_ref[desired_index]
        dqr = dq_ref[desired_index]
        ddqr = ddq_ref[desired_index]
        error_deg = np.abs(np.rad2deg(qr - q))
        if np.any(error_deg > MAX_FOLLOWING_ERROR_DEG):
            raise RuntimeError(f"following error exceeded: {np.round(error_deg, 2)}deg")
        correction = KP_ACC * (qr - q) + KD_ACC * (dqr - dq)
        friction, breakaway = friction_terms(q, dq, model, data, models, breakaway_models)
        ramp = min(1.0, elapsed / CORRECTION_RAMP_S)
        acceleration, rigid, actuator, tau, applied_scale, base_tau = bounded_computed_torque(
            model, data, q, dq, ddqr, correction, friction, breakaway,
            actuator_inertia, ramp,
        )
        # Pure torque tracking segment: no hidden MIT position/velocity gains.
        control.write_once(mit_command(np.zeros(6), np.zeros(6), tau))
        for name, value in (
            ("time_s", elapsed), ("seq", int(state.seq)), ("errors", int(state.errors)),
            ("q", q.copy()), ("dq", dq.copy()), ("tau_measured", measured_tau.copy()),
            ("q_ref", qr.copy()), ("dq_ref", dqr.copy()), ("ddq_ref", ddqr.copy()),
            ("ddq_cmd", acceleration.copy()), ("tau_rigid", rigid.copy()),
            ("tau_actuator", actuator.copy()), ("tau_friction", friction.copy()),
            ("tau_breakaway", breakaway.copy()), ("tau_command", tau.copy()),
            ("feedback_correction_scale", applied_scale),
            ("tau_model_feedforward", base_tau.copy()),
        ):
            log[name].append(value)
        index = desired_index + 1
        rate_frames += 1
        if now - rate_started >= RATE_WINDOW_S:
            rate = rate_frames / (now - rate_started)
            print(
                f"computed torque rate={rate:.1f}Hz, t={elapsed:.1f}/{t[-1]:.1f}s, "
                f"max_error={np.max(error_deg):.2f}deg, correction_scale={applied_scale:.3f}"
            )
            if rate < MIN_CONTROL_RATE_HZ:
                raise RuntimeError(f"control rate {rate:.1f}Hz below {MIN_CONTROL_RATE_HZ:g}Hz")
            rate_started = now
            rate_frames = 0
    return {name: np.asarray(value) for name, value in log.items()}


def atomic_save(path, arrays, **metadata):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("wb") as stream:
        np.savez_compressed(stream, **arrays, **metadata)
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)


def recoverable(error):
    text = str(error).lower()
    return friction_ff.recoverable(error) or any(token in text for token in (
        "following error", "feedback gap", "control rate",
    ))


def main():
    if not URDF.is_file() or not FIT.is_file():
        raise FileNotFoundError("new dynamic URDF or FIGAROH fit receipt is missing")
    model = pin.buildModelFromUrdf(str(URDF))
    data = model.createData()
    models, breakaway_models, actuator_inertia = load_models_and_actuator_inertia()
    reference = build_reference()
    t, q_ref, dq_ref, ddq_ref, period = reference
    nominal = np.asarray([
        pin.rnea(model, data, q, dq, ddq) + actuator_inertia * ddq
        + sum(friction_terms(q, dq, model, data, models, breakaway_models))
        for q, dq, ddq in zip(q_ref, dq_ref, ddq_ref)
    ])
    peak_speed = np.max(np.abs(np.rad2deg(dq_ref)), axis=0)
    peak_acceleration = np.max(np.abs(np.rad2deg(ddq_ref)), axis=0)
    peak_torque = np.max(np.abs(nominal), axis=0)
    print("NEW DYNAMIC URDF:", URDF)
    print("URDF SHA256:", sha256(URDF))
    print("pose A deg:", POSE_A_DEG)
    print("pose B requested deg:", POSE_B_REQUESTED_DEG)
    print("pose B executed deg:", POSE_B_EXECUTED_DEG)
    print(f"round trips={ROUND_TRIPS}, sine period={period:.3f}s, total reference={t[-1]:.3f}s")
    print("peak reference speed deg/s:", np.round(peak_speed, 3))
    print("peak reference acceleration deg/s^2:", np.round(peak_acceleration, 3))
    print("nominal computed-torque peak Nm:", np.round(peak_torque, 3))
    print("identified actuator inertia:", np.round(actuator_inertia, 6))
    print("tracking MIT kp=kd=0; firmware_gravity=False")
    print("DISABLED PREVIEW:", not ENABLE_HARDWARE)
    if np.max(peak_speed) > MAX_REFERENCE_SPEED_DEG_S + 1e-6:
        raise RuntimeError("generated reference exceeds requested 90 deg/s")
    if np.any(peak_torque > TAU_LIMIT_NM):
        raise RuntimeError("offline nominal torque exceeds configured motor limits")
    if not ENABLE_HARDWARE:
        return

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    arm.automatic_error_recovery()
    time.sleep(1.0)
    initial = read_valid(arm, 2.0)
    initial_q, _, _ = checked_state(initial)
    print("measured startup q_deg:", np.round(np.rad2deg(initial_q), 3))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch; hardware remains disabled")

    run_id = time.strftime("%Y%m%dT%H%M%S")
    output = LOG_DIR / f"computed_torque_10cycle_{run_id}.npz"
    try:
        control = friction_ff.start_session(arm)
        for attempt in range(1, MAX_RUN_ATTEMPTS + 1):
            try:
                print(f"computed-torque attempt {attempt}/{MAX_RUN_ATTEMPTS}")
                soft_move(control, model, data, q_ref[0], models, breakaway_models)
                arrays = run_reference(
                    control, model, data, reference, models, breakaway_models,
                    actuator_inertia,
                )
                if RETURN_TO_MEASURED_START:
                    print("returning smoothly to measured startup pose")
                    soft_move(control, model, data, initial_q, models, breakaway_models)
                    print("measured startup pose restored")
                atomic_save(
                    output, arrays, urdf_sha256=np.asarray(sha256(URDF)),
                    fit_sha256=np.asarray(sha256(FIT)), pose_a_deg=POSE_A_DEG,
                    pose_b_requested_deg=POSE_B_REQUESTED_DEG,
                    pose_b_executed_deg=POSE_B_EXECUTED_DEG,
                    round_trips=np.asarray(ROUND_TRIPS),
                    max_reference_speed_deg_s=np.asarray(MAX_REFERENCE_SPEED_DEG_S),
                    run_completed=np.asarray(True),
                )
                print("telemetry saved+fsynced:", output)
                break
            except Exception as error:
                if attempt >= MAX_RUN_ATTEMPTS or not recoverable(error):
                    raise
                print("recoverable computed-torque failure:", error)
                arm, control = friction_ff.recover_session(arm, error)
                print("fault cleared; returning to measured startup pose before retry")
                soft_move(control, model, data, initial_q, models, breakaway_models)
                print("restarting all 10 cycles from the beginning")
    finally:
        friction_ff.best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
