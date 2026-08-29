#!/usr/bin/env python3
"""Run the requested ten-cycle 90 deg/s trajectory with direct MIT tracking.

MIT q/dq/kp/kd produce the motion.  The MIT tau field contains strictly host
gravity compensation plus the identified friction/breakaway compensation.
No inertial, acceleration, or position-error torque is added to tau.
"""
from __future__ import annotations

import time
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid

import run_10cycle_sine_computed_torque as base


ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW 10 CYCLE MIT SINE"
MIT_KP = np.array([30., 60., 60., 50., 40., 18.], dtype=np.float32)
MIT_KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
TRANSITION_SPEED_DEG_S = 60.0
TRANSITION_ACCEL_DEG_S2 = 60.0
TAKEOVER_HOLD_S = 0.5
TARGET_HOLD_S = 1.0
PRINT_PERIOD_S = 1.0
MAX_ATTEMPTS = 3
LOG_DIR = Path(__file__).resolve().parent / "runs_mit_sine_10cycle"


def tau_gravity_friction(q, dq, model, data, models, breakaway_models):
    friction, breakaway = base.friction_terms(
        q, dq, model, data, models, breakaway_models
    )
    gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
    tau = base.checked_torque(
        gravity + friction + breakaway, "gravity/friction feedforward"
    )
    return tau, gravity, friction, breakaway


def write_mit(control, q_ref, dq_ref, tau):
    control.write_once(base.mit_command(q_ref, dq_ref, tau, MIT_KP, MIT_KD))


def drain_feedback_queue(reader):
    """Discard stale pre-confirmation states and return the newest frame."""
    latest = base.read_valid(reader, 2.0)
    drained = 1
    while True:
        state = reader.read_once()
        if int(state.seq) == 0:
            break
        latest = state
        drained += 1
    base.checked_state(latest)
    return latest, drained


def mit_transition(control, model, data, target, models, breakaway_models, label):
    state, drained = drain_feedback_queue(control)
    start, _, _ = base.checked_state(state)
    last_seq = int(state.seq)
    if drained > 1:
        print(f"{label}: discarded {drained-1} stale feedback frames before control")

    hold_end = time.monotonic() + TAKEOVER_HOLD_S
    while time.monotonic() < hold_end:
        state = base.read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = base.checked_state(state)
        tau, _, _, _ = tau_gravity_friction(
            q, dq, model, data, models, breakaway_models
        )
        write_mit(control, start, np.zeros(6), tau)

    delta = np.asarray(target, dtype=float) - start
    distance = float(np.max(np.abs(delta)))
    velocity_duration = 1.875 * distance / np.deg2rad(TRANSITION_SPEED_DEG_S)
    acceleration_duration = np.sqrt(
        5.8 * distance / np.deg2rad(TRANSITION_ACCEL_DEG_S2)
    )
    duration = max(1.0, velocity_duration, acceleration_duration)
    begin = time.monotonic()
    last_print = begin - PRINT_PERIOD_S
    frames = 0
    while True:
        now = time.monotonic()
        u = min(1.0, (now - begin) / duration)
        alpha = 10*u**3 - 15*u**4 + 6*u**5
        alpha_d = (30*u**2 - 60*u**3 + 30*u**4) / duration
        q_ref = start + alpha * delta
        dq_ref = alpha_d * delta
        state = base.read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = base.checked_state(state)
        tau, _, _, _ = tau_gravity_friction(
            q, dq, model, data, models, breakaway_models
        )
        write_mit(control, q_ref, dq_ref, tau)
        frames += 1
        if now - last_print >= PRINT_PERIOD_S:
            print(
                f"{label} t={now-begin:.2f}/{duration:.2f}s "
                f"max_error={np.max(np.abs(np.rad2deg(q_ref-q))):.2f}deg "
                f"q_deg={np.round(np.rad2deg(q), 2)}"
            )
            last_print = now
        if u >= 1.0:
            break

    hold_begin = time.monotonic()
    while time.monotonic() - hold_begin < TARGET_HOLD_S:
        state = base.read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, _ = base.checked_state(state)
        tau, _, _, _ = tau_gravity_friction(
            q, dq, model, data, models, breakaway_models
        )
        write_mit(control, target, np.zeros(6), tau)

    final_q, final_dq, _ = base.checked_state(state)
    print(
        f"{label} complete: rate={frames/max(time.monotonic()-begin, 1e-9):.1f}Hz "
        f"final_error={np.max(np.abs(np.rad2deg(target-final_q))):.2f}deg "
        f"final_speed={np.max(np.abs(np.rad2deg(final_dq))):.2f}deg/s"
    )


def run_sine(control, model, data, reference, models, breakaway_models):
    t, q_ref, dq_ref, ddq_ref, _ = reference
    sample_period = float(np.median(np.diff(t)))
    state, drained = drain_feedback_queue(control)
    if drained > 1:
        print(f"MIT sine: discarded {drained-1} stale feedback frames")
    started = time.monotonic()
    last_seq = int(state.seq)
    rate_started = started
    rate_frames = 0
    rows = {key: [] for key in (
        "time_s", "host_time_s", "source_timestamp_us", "seq", "errors",
        "q_ref", "dq_ref", "ddq_ref", "q",
        "dq", "tau_feedback", "tau_ff_command", "tau_gravity",
        "tau_friction", "tau_breakaway", "tau_mit_pd_estimated",
        "tau_total_reference_estimated",
    )}
    while True:
        elapsed = time.monotonic() - started
        index = min(int(elapsed / sample_period), len(t) - 1)
        state = base.read_valid(control, last_seq=last_seq)
        last_seq = int(state.seq)
        q, dq, tau_feedback = base.checked_state(state)
        qr = q_ref[index]
        dqr = dq_ref[index]
        ddqr = ddq_ref[index]
        tau, gravity, friction, breakaway = tau_gravity_friction(
            q, dq, model, data, models, breakaway_models
        )
        tau_mit_pd = MIT_KP.astype(float) * (qr - q) + MIT_KD.astype(float) * (dqr - dq)
        tau_total_reference = tau + tau_mit_pd
        write_mit(control, qr, dqr, tau)
        rows["time_s"].append(elapsed)
        rows["host_time_s"].append(time.time())
        rows["source_timestamp_us"].append(int(getattr(state, "source_timestamp_us", 0)))
        rows["seq"].append(last_seq)
        rows["errors"].append(int(state.errors))
        rows["q_ref"].append(qr.copy())
        rows["dq_ref"].append(dqr.copy())
        rows["ddq_ref"].append(ddqr.copy())
        rows["q"].append(q.copy())
        rows["dq"].append(dq.copy())
        rows["tau_feedback"].append(tau_feedback.copy())
        rows["tau_ff_command"].append(tau.copy())
        rows["tau_gravity"].append(gravity.copy())
        rows["tau_friction"].append(friction.copy())
        rows["tau_breakaway"].append(breakaway.copy())
        rows["tau_mit_pd_estimated"].append(tau_mit_pd.copy())
        rows["tau_total_reference_estimated"].append(tau_total_reference.copy())
        rate_frames += 1
        now = time.monotonic()
        if now - rate_started >= PRINT_PERIOD_S:
            rate = rate_frames / (now - rate_started)
            print(
                f"MIT sine rate={rate:.1f}Hz "
                f"t={elapsed:.2f}/{t[-1]:.2f}s "
                f"max_error={np.max(np.abs(np.rad2deg(qr-q))):.2f}deg"
            )
            rate_started = now
            rate_frames = 0
        if index >= len(t) - 1:
            break
    return {key: np.asarray(value) for key, value in rows.items()}


def main():
    model = pin.buildModelFromUrdf(str(base.URDF))
    data = model.createData()
    models, breakaway_models, _ = base.load_models_and_actuator_inertia()
    reference = base.build_reference()
    t, _, dq_ref, _, period = reference
    print("MIT POSITION SINE TEST")
    print("URDF:", base.URDF)
    print("pose A deg:", base.POSE_A_DEG)
    print("pose B executed deg:", base.POSE_B_EXECUTED_DEG)
    print(f"round trips={base.ROUND_TRIPS}, period={period:.3f}s, duration={t[-1]:.3f}s")
    print("peak reference speed deg/s:", np.round(np.max(np.abs(np.rad2deg(dq_ref)), axis=0), 3))
    print("MIT kp:", MIT_KP.tolist(), "kd:", MIT_KD.tolist())
    print("tau_ff=gravity+friction only; firmware_gravity=False")
    print("DISABLED PREVIEW:", not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE:
        return

    arm = pyflorid.Arm.create(base.DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {base.DEVICE_URI}")
    arm.automatic_error_recovery()
    time.sleep(1.0)
    initial = base.read_valid(arm, 2.0)
    initial_q, _, _ = base.checked_state(initial)
    print("measured startup q_deg:", np.round(np.rad2deg(initial_q), 3))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation mismatch")

    try:
        control = base.friction_ff.start_session(arm)
        mit_transition(
            control, model, data, reference[1][0], models, breakaway_models,
            "MIT move to A",
        )
        arrays = run_sine(
            control, model, data, reference, models, breakaway_models
        )
        print("returning to measured startup pose with MIT interpolation")
        mit_transition(
            control, model, data, initial_q, models, breakaway_models,
            "MIT return",
        )
        output = LOG_DIR / f"mit_sine_10cycle_{time.strftime('%Y%m%dT%H%M%S')}.npz"
        base.atomic_save(
            output, arrays, urdf_sha256=np.asarray(base.sha256(base.URDF)),
            pose_a_deg=base.POSE_A_DEG,
            pose_b_executed_deg=base.POSE_B_EXECUTED_DEG,
            round_trips=np.asarray(base.ROUND_TRIPS),
            tau_feedforward=np.asarray("gravity+friction_only"),
            mit_kp=MIT_KP.astype(float), mit_kd=MIT_KD.astype(float),
            torque_reference_semantics=np.asarray(
                "tau_total_reference_estimated=tau_ff_command+kp*(q_ref-q)+kd*(dq_ref-dq)"
            ),
        )
        print("telemetry saved+fsynced:", output)
    finally:
        base.friction_ff.best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
