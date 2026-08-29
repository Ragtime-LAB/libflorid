#!/usr/bin/env python3
"""Willow first computed-torque experiment: move to A, then hold measured q_d.

Offline preview is the default.  The verified MIT position bridge moves to the
requested pose.  After arrival, the newest measured encoder state is latched as
q_d.  The subsequent 500 Hz host loop sends only tau through MIT, with MIT
kp=kd=0 and firmware gravity disabled.
"""
from __future__ import annotations

import csv
import os
import sys
import time
from collections import deque
from pathlib import Path

import numpy as np
import pinocchio as pin
import pyflorid


ROOT = Path(__file__).resolve().parent
DYNAMIC_ROOT = Path("/home/kelvinlm/projects/libflorid-main/tools/dynamic_identification")
sys.path.insert(0, str(DYNAMIC_ROOT))
import run_10cycle_sine_computed_torque as base

URDF = Path(
    "/home/kelvinlm/projects/libflorid-main/tools/static_gravity_calibration/model/identified/"
    "Ragtime_Willow_dynamic_batch_3cb7a96993a3_120deg_s.urdf"
)
FIT = Path(
    "/mnt/c/Users/KelvinLM/Documents/Codex/FIGAROH/results/figaroh_dynamic_batch/"
    "3cb7a96993a3_120deg_s/fit.json"
)
DEVICE_URI = "usb:///dev/ttyACM0"

ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW COMPUTED TORQUE STARTUP HOLD"
MIT_MOVE_TARGET_DEG = np.array([85.76, 124.84, 104.42, -37.06, -15.22, 128.46])
HOLD_SECONDS = 30.0
HOST_CORRECTION_RAMP_SECONDS = 2.0

# First uniform physical validation: moderate computed-torque bandwidth on the
# large J1..J3 modules and a softer bandwidth on J4..J6.
OMEGA_N_RAD_S = np.array([2.0, 2.0, 2.0, 2.0, 2.0, 2.0])
KP_CT = OMEGA_N_RAD_S**2
KD_CT = 2.0 * OMEGA_N_RAD_S

# Real encoders/firmware velocities are not smooth enough to feed directly
# into Kd, friction direction, and the velocity-quadratic RNEA terms.
DQ_FILTER_CUTOFF_HZ = 10.0
# The original experiment used the filtered velocity for damping.  Follow-up
# wrappers may select the firmware's native 500 Hz state.dq instead; this avoids
# the measured 56 degree filter lag at the 19.5 Hz oscillation.
DAMPING_DQ_SOURCE = "filtered"  # "filtered" or "raw"
# Optional joint-torque damping B [Nm/(rad/s)].  It is inserted through the
# computed-torque acceleration command as M(q)^-1(-B*dq), so the resulting
# RNEA correction is exactly -B*dq.  This is useful on wrist axes whose
# identified M(q) diagonal is very small: one acceleration-domain Kd value
# would otherwise produce radically different physical damping on each axis.
JOINT_TORQUE_DAMPING_NM_PER_RAD_S = np.zeros(6, dtype=float)
# First filtered validation deliberately omits C(q,dq)dq.  It is still
# computed and logged so later tests can raise this through 0.25/0.5/1.0.
CORIOLIS_SCALE = 0.0

# Optional low-speed wrist stiction take-up.  The deployed friction model uses
# measured velocity to choose Coulomb direction, so at rest it contributes
# almost no directional torque.  When enabled, only J4..J6 smoothly borrow the
# sign of position error at low speed.  Their existing dynamic friction model
# and scale remain unchanged away from zero speed.
ENABLE_WRIST_ERROR_DIRECTED_FRICTION = False
WRIST_ERROR_DIRECTION_SCALE_RAD = np.deg2rad(2.0)
WRIST_PROXY_SPEED_RAD_S = 0.02
WRIST_PROXY_DECAY_SPEED_RAD_S = 0.05
WRIST_STATIC_FRICTION_BOOST = np.array(
    [0.0, 0.0, 0.0, 1.0 / 0.55, 1.0 / 0.60, 1.0 / 0.60], dtype=float
)

TAU_LIMIT_NM = np.array([40.0, 40.0, 40.0, 12.0, 12.0, 12.0])
MAX_RECOVERY_ATTEMPTS = 3
PRINT_PERIOD_S = 0.5
LOG_FSYNC_PERIOD_S = 0.25
J2_PLATEAU_NM = 11.9
J2_PLATEAU_BAND_NM = 0.15
J2_PLATEAU_WARN_SECONDS = 0.10

LOG_DIR = DYNAMIC_ROOT / "runs_computed_torque_startup_hold"

# Point the already-tested transport/model helpers at the accepted new model.
base.URDF = URDF
base.FIT = FIT
base.DEVICE_URI = DEVICE_URI
base.friction_ff.DEVICE_URI = DEVICE_URI


def smoothstep(x: float) -> float:
    x = float(np.clip(x, 0.0, 1.0))
    return x * x * (3.0 - 2.0 * x)


def equivalent_branch(target: np.ndarray, measured: np.ndarray) -> np.ndarray:
    return target + 2.0 * np.pi * np.round((measured - target) / (2.0 * np.pi))


class Telemetry:
    def __init__(self, path: Path):
        self.path = path
        path.parent.mkdir(parents=True, exist_ok=True)
        self.file = path.open("w", encoding="utf-8", newline="", buffering=1024 * 1024)
        self.writer = csv.writer(self.file)
        fields = [
            "host_time_s", "monotonic_s", "phase", "correction_scale",
            "command_written", "seq", "seq_delta", "dropped_frames",
            "source_timestamp_us", "host_period_s", "device_period_s",
            "firmware_errors", "V", "V_slope_2s",
            "j2_plateau_residence_s", "j2_wrap_event",
        ]
        for prefix in (
            "q_d_rad", "q_rad", "e_rad", "dq_d_rad_s", "dq_rad_s",
            "dq_filtered_rad_s", "e_dot_rad_s", "e_dot_filtered_rad_s",
            "ddq_cmd_rad_s2", "tau_gravity_nm", "tau_coriolis_full_nm",
            "tau_bias_nm", "tau_position_restore_nm",
            "tau_velocity_damping_nm", "tau_closed_loop_correction_nm",
            "tau_rnea_nm", "tau_friction_nm", "tau_total_requested_nm",
            "tau_total_applied_nm", "tau_feedback_nm", "saturated",
            "mit_kp", "mit_kd",
        ):
            fields.extend(f"{prefix}_j{i}" for i in range(1, 7))
        self.writer.writerow(fields)
        self.rows = 0
        self.last_fsync = time.monotonic()
        self.flush(True)

    def write(
        self, state, phase, scale, seq_delta, host_dt, device_dt, q_d, q, dq,
        dq_filtered, ddq_cmd, tau_gravity, tau_coriolis_full, tau_bias,
        tau_position_restore,
        tau_velocity_damping, tau_closed_loop_correction, tau_rnea,
        tau_friction, tau_requested, tau_applied,
        tau_feedback, saturated, value, value_slope, plateau_s, wrap_event,
        command_written=True,
    ):
        e = q_d - q
        zeros = np.zeros(6)
        row = [
            time.time(), time.monotonic(), phase, scale, int(command_written),
            int(state.seq), int(seq_delta), max(0, int(seq_delta) - 1),
            int(getattr(state, "source_timestamp_us", 0)), host_dt, device_dt,
            int(state.errors), value, value_slope, plateau_s, int(wrap_event),
        ]
        for vector in (
            q_d, q, e, zeros, dq, dq_filtered, -dq, -dq_filtered, ddq_cmd,
            tau_gravity, tau_coriolis_full, tau_bias,
            tau_position_restore, tau_velocity_damping,
            tau_closed_loop_correction, tau_rnea, tau_friction,
            tau_requested, tau_applied, tau_feedback, saturated.astype(int),
            zeros, zeros,
        ):
            row.extend(np.asarray(vector, dtype=float).tolist())
        self.writer.writerow(row)
        self.rows += 1
        self.flush()

    def write_fault(self, state):
        nan = np.full(6, np.nan)
        self.write(
            state=state, phase="firmware_fault", scale=np.nan, seq_delta=0,
            host_dt=np.nan, device_dt=np.nan, q_d=nan, q=nan, dq=nan,
            dq_filtered=nan, ddq_cmd=nan, tau_gravity=nan,
            tau_coriolis_full=nan, tau_bias=nan,
            tau_position_restore=nan, tau_velocity_damping=nan,
            tau_closed_loop_correction=nan, tau_rnea=nan,
            tau_friction=nan, tau_requested=nan, tau_applied=nan,
            tau_feedback=nan, saturated=np.zeros(6, dtype=bool),
            value=np.nan, value_slope=np.nan, plateau_s=np.nan,
            wrap_event=False, command_written=False,
        )

    def flush(self, force=False):
        now = time.monotonic()
        if force or now - self.last_fsync >= LOG_FSYNC_PERIOD_S:
            self.file.flush()
            os.fsync(self.file.fileno())
            self.last_fsync = now

    def close(self):
        if not self.file.closed:
            self.flush(True)
            self.file.close()


def host_hold(control, model, data, q_d, models, breakaway_models, logger, duration_s):
    """Run one 500 Hz segment; q_d remains unchanged across recoveries."""
    state, drained = base.drain_feedback_queue(control)
    q, dq_filtered, _ = base.checked_state(state)
    dq_filtered = np.asarray(dq_filtered, dtype=float).copy()
    last_seq = int(state.seq)
    last_source_us = int(getattr(state, "source_timestamp_us", 0))
    last_host = time.monotonic()
    previous_j2 = float(q[1])
    print(
        f"host takeover: discarded {drained-1} stale frames; "
        f"reread q_deg={np.round(np.rad2deg(q), 3)}; q_d is preserved"
    )

    begin = time.monotonic()
    last_print = begin
    frames_window = drops_window = total_frames = total_drops = 0
    plateau_s = longest_plateau_s = 0.0
    plateau_warned = False
    wrap_events = saturation_rows = 0
    values = deque()

    while time.monotonic() - begin < duration_s:
        state = base.read_valid(control, last_seq=last_seq)
        now = time.monotonic()
        if int(state.errors):
            logger.write_fault(state)
            raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
        q, dq, tau_feedback = base.checked_state(state)
        seq = int(state.seq)
        seq_delta = (seq - last_seq) & 0xFFFFFFFF
        if seq_delta == 0:
            continue
        source_us = int(getattr(state, "source_timestamp_us", 0))
        host_dt = now - last_host
        device_dt = (
            ((source_us - last_source_us) & 0xFFFFFFFF) * 1e-6
            if source_us and last_source_us else np.nan
        )
        last_seq, last_source_us, last_host = seq, source_us, now

        filter_dt = device_dt if np.isfinite(device_dt) and device_dt > 0.0 else host_dt
        filter_alpha = 1.0 - np.exp(-2.0 * np.pi * DQ_FILTER_CUTOFF_HZ * filter_dt)
        filter_alpha = float(np.clip(filter_alpha, 0.0, 1.0))
        dq_filtered += filter_alpha * (dq - dq_filtered)

        elapsed = now - begin
        scale = smoothstep(elapsed / HOST_CORRECTION_RAMP_SECONDS)
        e = q_d - q
        if DAMPING_DQ_SOURCE == "raw":
            dq_damping = dq
        elif DAMPING_DQ_SOURCE == "filtered":
            dq_damping = dq_filtered
        else:
            raise RuntimeError(f"unsupported DAMPING_DQ_SOURCE={DAMPING_DQ_SOURCE!r}")
        ddq_full = KP_CT * e - KD_CT * dq_damping
        ddq_cmd = scale * ddq_full
        mass_matrix = np.asarray(pin.crba(model, data, q), dtype=float)
        mass_matrix = 0.5 * (mass_matrix + mass_matrix.T)
        tau_gravity = np.asarray(pin.computeGeneralizedGravity(model, data, q), dtype=float)
        filtered_bias_full = np.asarray(
            pin.rnea(model, data, q, dq_filtered, np.zeros(6)), dtype=float
        )
        tau_coriolis_full = filtered_bias_full - tau_gravity
        tau_bias = tau_gravity + scale * CORIOLIS_SCALE * tau_coriolis_full
        tau_position_restore = mass_matrix @ (scale * KP_CT * e)
        tau_direct_damping = -scale * JOINT_TORQUE_DAMPING_NM_PER_RAD_S * dq_damping
        tau_velocity_damping = (
            mass_matrix @ (scale * (-KD_CT * dq_damping)) + tau_direct_damping
        )
        # Keep ddq_cmd consistent with the actual torque correction.  Solving
        # through M(q) is mathematically equivalent to adding -B*dq after RNEA,
        # while retaining one computed-torque acceleration command.
        ddq_cmd = ddq_cmd + np.linalg.solve(mass_matrix, tau_direct_damping)
        tau_closed_loop_correction = tau_position_restore + tau_velocity_damping
        tau_rnea = tau_bias + tau_closed_loop_correction
        friction, breakaway = base.friction_terms(
            q, dq_filtered, model, data, models, breakaway_models
        )
        if ENABLE_WRIST_ERROR_DIRECTED_FRICTION:
            proxy_dq = dq_filtered.copy()
            error_direction = np.tanh(e / WRIST_ERROR_DIRECTION_SCALE_RAD)
            proxy_dq[3:6] = WRIST_PROXY_SPEED_RAD_S * error_direction[3:6]
            proxy_friction, _ = base.friction_terms(
                q, proxy_dq, model, data, models, breakaway_models
            )
            speed_gate = np.exp(
                -(np.abs(dq_filtered) / WRIST_PROXY_DECAY_SPEED_RAD_S) ** 2
            )
            friction[3:6] += (
                speed_gate[3:6]
                * WRIST_STATIC_FRICTION_BOOST[3:6]
                * (proxy_friction[3:6] - friction[3:6])
            )
        tau_friction = friction + breakaway
        tau_requested = tau_rnea + tau_friction
        # Operator requested the unmodified computed torque for this instrumented
        # test.  TAU_LIMIT_NM is telemetry-only; no host-side clipping occurs.
        tau_applied = tau_requested.copy()
        saturated = np.abs(tau_requested) > TAU_LIMIT_NM
        saturation_rows += int(np.any(saturated))

        # MIT q/dq fields are inert because both firmware gains are zero.
        control.write_once(base.mit_command(q_d, np.zeros(6), tau_applied))

        value = 0.5 * float(dq_filtered @ dq_filtered) + 0.5 * float(e @ (KP_CT * e))
        values.append((now, value))
        while values and now - values[0][0] > 2.0:
            values.popleft()
        value_slope = np.nan
        if len(values) > 1:
            value_slope = (values[-1][1] - values[0][1]) / (values[-1][0] - values[0][0])

        in_platform_band = abs(abs(float(tau_feedback[1])) - J2_PLATEAU_NM) <= J2_PLATEAU_BAND_NM
        plateau_s = plateau_s + (device_dt if np.isfinite(device_dt) else host_dt) if in_platform_band else 0.0
        longest_plateau_s = max(longest_plateau_s, plateau_s)
        if plateau_s >= J2_PLATEAU_WARN_SECONDS and not plateau_warned:
            print(f"WARNING: J2 feedback near +/-{J2_PLATEAU_NM} Nm for {plateau_s:.3f}s")
            plateau_warned = True
        if not in_platform_band:
            plateau_warned = False

        wrap_event = abs(float(q[1]) - previous_j2) > np.pi
        previous_j2 = float(q[1])
        if wrap_event:
            wrap_events += 1
            print("WARNING: J2 adjacent feedback crossed an encoder +/-180 deg branch")

        logger.write(
            state, "computed_torque_hold", scale, seq_delta, host_dt,
            device_dt, q_d, q, dq, dq_filtered, ddq_cmd, tau_gravity,
            tau_coriolis_full, tau_bias, tau_position_restore,
            tau_velocity_damping,
            tau_closed_loop_correction, tau_rnea, tau_friction, tau_requested,
            tau_applied, tau_feedback, saturated, value, value_slope,
            plateau_s, wrap_event,
        )
        drops = max(0, seq_delta - 1)
        total_frames += 1
        frames_window += 1
        total_drops += drops
        drops_window += drops

        if now - last_print >= PRINT_PERIOD_S:
            window = now - last_print
            print(
                f"hold t={elapsed:.2f}/{duration_s:.2f}s rate={frames_window/window:.1f}Hz "
                f"drops={drops_window} max|e|={np.max(np.abs(np.rad2deg(e))):.3f}deg "
                f"V={value:.6f} dV/2s={value_slope:.6f} "
                f"J2={np.rad2deg(q[1]):.2f}deg/{tau_feedback[1]:.3f}Nm "
                f"max|dq|={np.max(np.abs(np.rad2deg(dq))):.1f}/"
                f"{np.max(np.abs(np.rad2deg(dq_filtered))):.1f}deg/s(raw/filt) "
                f"tau_g={np.round(tau_gravity, 2).tolist()} "
                f"tau_c_full={np.round(tau_coriolis_full, 2).tolist()} "
                f"tau_pos={np.round(tau_position_restore, 2).tolist()} "
                f"tau_damp={np.round(tau_velocity_damping, 2).tolist()} "
                f"nominal_exceeded={saturated.astype(int).tolist()}"
            )
            last_print = now
            frames_window = drops_window = 0

    return {
        "frames": total_frames,
        "dropped_frames": total_drops,
        "saturation_rows": saturation_rows,
        "longest_j2_platform_s": longest_plateau_s,
        "j2_wrap_events": wrap_events,
    }


def main():
    if not URDF.is_file() or not FIT.is_file():
        raise FileNotFoundError("accepted dynamic URDF or fit receipt is missing")
    model = pin.buildModelFromUrdf(str(URDF))
    data = model.createData()
    models, breakaway_models, actuator_inertia = base.load_models_and_actuator_inertia()
    move_target = np.deg2rad(MIT_MOVE_TARGET_DEG)
    zero = np.zeros(6)
    offline_rnea = np.asarray(pin.rnea(model, data, move_target, zero, zero), dtype=float)
    friction, breakaway = base.friction_terms(
        move_target, zero, model, data, models, breakaway_models
    )
    offline_friction = friction + breakaway
    offline_total = offline_rnea + offline_friction

    print("COMPUTED-TORQUE STARTUP HOLD")
    print("URDF:", URDF)
    print("URDF SHA256:", base.sha256(URDF))
    print("MIT move target deg:", MIT_MOVE_TARGET_DEG)
    print("omega_n rad/s:", OMEGA_N_RAD_S.tolist())
    print("Kp_ct:", KP_CT.tolist())
    print("Kd_ct:", KD_CT.tolist())
    print("dq low-pass cutoff Hz:", DQ_FILTER_CUTOFF_HZ)
    print("damping velocity source:", DAMPING_DQ_SOURCE)
    print("joint torque damping B Nm/(rad/s):", JOINT_TORQUE_DAMPING_NM_PER_RAD_S.tolist())
    print("wrist error-directed low-speed friction:", ENABLE_WRIST_ERROR_DIRECTED_FRICTION)
    print("Coriolis scale:", CORIOLIS_SCALE, "(full filtered estimate is still logged)")
    print("offline tau_rnea Nm:", np.round(offline_rnea, 4))
    print("offline tau_friction Nm:", np.round(offline_friction, 4))
    print("offline tau_total Nm:", np.round(offline_total, 4))
    print("actuator inertia is reported but not added to this requested first-law RNEA:", np.round(actuator_inertia, 6))
    print("after MIT arrival, newest measured q is latched as q_d")
    print("host phase: MIT kp=kd=0, firmware_gravity=False, 500 Hz logging")
    print("controller: g(q) + scaled C(q,dq_f) + M(q)(Kp*e-Kd*dq_f) + friction(q,dq_f)")
    print("host torque clipping: DISABLED; 40/40/40/12/12/12 is telemetry-only")
    print("DISABLED PREVIEW:", not ENABLE_HARDWARE)
    if np.any(np.abs(offline_total) > TAU_LIMIT_NM):
        print("WARNING: offline hold torque exceeds nominal telemetry threshold:", offline_total)
    if not ENABLE_HARDWARE:
        return

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    logger = None
    try:
        arm.automatic_error_recovery()
        time.sleep(1.0)
        preview, drained = base.drain_feedback_queue(arm)
        preview_q, _, _ = base.checked_state(preview)
        target_branch = equivalent_branch(move_target, preview_q)
        print(f"preview: discarded {drained-1} stale frames")
        print("startup encoder q_deg:", np.round(np.rad2deg(preview_q), 3))
        print("MIT target encoder branch deg:", np.round(np.rad2deg(target_branch), 3))
        print("entry delta deg:", np.round(np.rad2deg(target_branch - preview_q), 3))
        if input(f'Type exactly "{CONFIRMATION_PHRASE}": ') != CONFIRMATION_PHRASE:
            raise RuntimeError("confirmation mismatch; hardware remains disabled")

        control = base.friction_ff.start_session(arm)
        fresh, fresh_drained = base.drain_feedback_queue(control)
        fresh_q, _, _ = base.checked_state(fresh)
        target_branch = equivalent_branch(move_target, fresh_q)
        print(f"confirmed: discarded {fresh_drained-1} stale frames")
        print("fresh startup q_deg:", np.round(np.rad2deg(fresh_q), 3))
        base.soft_move(
            control, model, data, target_branch, models, breakaway_models,
            actuator_inertia, telemetry=None, phase="mit_move_to_A",
        )

        # This is the computed-torque controller's actual startup state.
        latched, latched_drained = base.drain_feedback_queue(control)
        q_d, _, _ = base.checked_state(latched)
        print(f"computed-torque latch: discarded {latched_drained-1} stale frames")
        print("LOCKED measured q_d deg:", np.round(np.rad2deg(q_d), 4))
        print("q_d will remain unchanged through any fault recovery")

        run_id = time.strftime("%Y%m%dT%H%M%S")
        logger = Telemetry(LOG_DIR / f"computed_torque_startup_hold_{run_id}_500hz.csv")
        print("500 Hz CSV:", logger.path)
        summaries = []
        recoveries = 0
        remaining = HOLD_SECONDS
        while remaining > 0.0:
            segment_start = time.monotonic()
            try:
                summaries.append(host_hold(
                    control, model, data, q_d, models, breakaway_models,
                    logger, remaining,
                ))
                remaining = 0.0
            except Exception as error:
                remaining = max(0.0, remaining - (time.monotonic() - segment_start))
                if not base.recoverable(error) or recoveries >= MAX_RECOVERY_ATTEMPTS:
                    raise
                recoveries += 1
                arm, control = base.friction_ff.recover_session(arm, error)
                current, redrained = base.drain_feedback_queue(control)
                current_q, _, _ = base.checked_state(current)
                print(
                    f"recovery {recoveries}/{MAX_RECOVERY_ATTEMPTS}: discarded {redrained-1} stale frames; "
                    f"reread q_deg={np.round(np.rad2deg(current_q), 3)}; original q_d preserved; "
                    "host correction will ramp in again"
                )

        print("hold complete; handing current state back to MIT before final return")
        base.soft_move(
            control, model, data, q_d, models, breakaway_models,
            actuator_inertia, telemetry=None, phase="mit_finish_at_q_d",
        )
        print("summaries:", summaries, "recoveries=", recoveries)
    finally:
        if logger is not None:
            logger.close()
            print(f"500 Hz CSV closed+fsynced: {logger.path} rows={logger.rows}")
        base.friction_ff.best_effort_disable(arm)
        print("All axes disabled.")


if __name__ == "__main__":
    main()
