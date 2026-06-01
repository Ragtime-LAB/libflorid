"""Computed torque sinusoidal trajectory using pure torque commands."""

from __future__ import annotations

import json
import time
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from _common import apply_defaults, clip_torque, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state


FREQUENCY = 0.3
DURATION = 10.0
CONTROL_RATE = 500.0
SETTLE_TIME = 1.0
TAU_LIMIT = np.array([26.90, 26.90, 26.90, 6.90, 6.90, 6.90], dtype=np.float32)

JOINT_LIMITS = np.array(
    [
        [-np.pi, np.pi],
        [0.0, np.pi],
        [0.0, np.pi],
        [-1.3, 1.3],
        [-np.pi / 2, np.pi / 2],
        [-np.pi, np.pi],
    ],
    dtype=np.float32,
)

# CENTER_POS = np.array([0.0, 0.8, 0.6, -0.5, 0.0, 0.0], dtype=np.float32)
CENTER_POS = np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32)
PRESET_AMPLITUDES = np.array([0.3, 0.4, 0.3, 0.2, 0.4, 0.4], dtype=np.float32)
PHASE_OFFSETS = np.zeros(6, dtype=np.float32)

# CT_KP = np.array([200.0, 230.0, 350.0, 200.0, 200.0, 0.0], dtype=np.float32)
# CT_KD = np.array([20.0, 23.0, 35.0, 20.0, 20.0, 0.0], dtype=np.float32)
CT_KP = np.array([100.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32)
CT_KD = np.array([10.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32)

def build_trajectory() -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    lower = JOINT_LIMITS[:, 0]
    upper = JOINT_LIMITS[:, 1]
    safe_amp = np.minimum(upper - CENTER_POS, CENTER_POS - lower) * 0.8
    amplitudes = np.minimum(safe_amp, PRESET_AMPLITUDES)
    return lower, upper, amplitudes


def hold_position(arm: pyflorid.Arm, q_target: np.ndarray, duration: float, kp: np.ndarray, kd: np.ndarray) -> None:
    control = arm.start_joint_position_control()
    deadline = time.time() + duration
    while time.time() < deadline:
        state = control.read_once()
        ensure_safe_mode(state)
        cmd = pyflorid.JointPositions()
        cmd.q = q_target
        cmd.dq = np.zeros(6, dtype=np.float32)
        cmd.kp = kp
        cmd.kd = kd
        control.write_once(cmd)
        time.sleep(0.01)


def move_to_start(arm: pyflorid.Arm, start_pos: np.ndarray) -> None:
    arm.switch_control_mode(pyflorid.ControlMode.JointPosition)
    wait_for_valid_state(arm)
    print("Moving to zero pose...")
    hold_position(
        arm,
        np.zeros(6, dtype=np.float32),
        duration=3.0,
        kp=np.array([12, 12, 12, 6, 4, 2], dtype=np.float32),
        kd=np.array([1.2, 1.2, 1.2, 0.6, 0.4, 0.2], dtype=np.float32),
    )
    time.sleep(SETTLE_TIME)

    print("Moving to trajectory center...")
    hold_position(
        arm,
        start_pos,
        duration=4.0,
        kp=np.array([14, 16, 16, 8, 5, 2], dtype=np.float32),
        kd=np.array([1.4, 1.6, 1.6, 0.8, 0.5, 0.2], dtype=np.float32),
    )
    time.sleep(SETTLE_TIME)


def settle_in_torque(arm: pyflorid.Arm, model: pyflorid.WillowModel) -> None:
    state = arm.update()
    q_hold = state.q.astype(np.float32)
    tau_hold = model.gravity(q_hold, state.base_gravity.astype(np.float32))

    print(f"Holding final pose | q={q_hold.round(3)} gravity={tau_hold.round(3)}")
    deadline = time.time() + 1.0
    while time.time() < deadline:
        state = arm.update()
        ensure_safe_mode(state)

        torque_cmd = pyflorid.Torques()
        torque_cmd.tau = tau_hold
        torque_cmd.kp = np.zeros(6, dtype=np.float32)
        torque_cmd.kd = np.zeros(6, dtype=np.float32)
        arm.write_once(torque_cmd)
        time.sleep(0.01)


def save_results(output_dir: Path, result: dict[str, list | np.ndarray]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    json_path = output_dir / f"computed_torque_j12345_{timestamp}.json"
    png_path = output_dir / f"computed_torque_j12345_{timestamp}.png"

    payload = {k: v.tolist() if isinstance(v, np.ndarray) else v for k, v in result.items()}
    json_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"\nSaved data to {json_path}")

    joint_count = result["desired_pos"].shape[1]
    fig, axes = plt.subplots(joint_count, 4, figsize=(24, 2.7 * joint_count), sharex="col")
    if joint_count == 1:
        axes = np.array([axes])

    pos_err = result["desired_pos"] - result["actual_pos"]

    for i in range(joint_count):
        axes[i, 0].plot(result["time"], result["desired_pos"][:, i], label="desired", linewidth=1.5)
        axes[i, 0].plot(result["time"], result["actual_pos"][:, i], label="actual", linewidth=1.1)
        axes[i, 0].set_ylabel(f"J{i + 1} (rad)")
        axes[i, 0].grid(True, linestyle="--", alpha=0.35)
        axes[i, 0].legend(loc="upper right")

        axes[i, 1].plot(result["time"], pos_err[:, i], label="pos_error", linewidth=1.0)
        axes[i, 1].grid(True, linestyle="--", alpha=0.35)
        axes[i, 1].legend(loc="upper right")

        axes[i, 2].plot(result["time"], result["gravity_torque"][:, i], label="gravity", linewidth=1.0)
        axes[i, 2].plot(result["time"], result["coriolis_torque"][:, i], label="coriolis", linewidth=1.0)
        axes[i, 2].plot(result["time"], result["inertia_ff_torque"][:, i], label="M*ddq_des", linewidth=1.0)
        axes[i, 2].plot(result["time"], result["feedback_torque"][:, i], label="feedback", linewidth=1.0)
        axes[i, 2].grid(True, linestyle="--", alpha=0.35)
        axes[i, 2].legend(loc="upper right")

        axes[i, 3].plot(result["time"], result["total_torque"][:, i], label="total", linewidth=1.1)
        axes[i, 3].plot(result["time"], result["commanded_torque"][:, i], label="commanded", linewidth=1.1)
        axes[i, 3].grid(True, linestyle="--", alpha=0.35)
        axes[i, 3].legend(loc="upper right")

    axes[-1, 0].set_xlabel("Time (s)")
    axes[-1, 1].set_xlabel("Time (s)")
    axes[-1, 2].set_xlabel("Time (s)")
    axes[-1, 3].set_xlabel("Time (s)")
    axes[0, 0].set_title("Desired vs Actual")
    axes[0, 1].set_title("Tracking Error")
    axes[0, 2].set_title("Torque Components")
    axes[0, 3].set_title("Total vs Commanded")
    fig.suptitle("Computed Torque Sin Trajectory (Pure Torque)")
    fig.tight_layout()
    fig.savefig(png_path, dpi=180, bbox_inches="tight")
    plt.close(fig)

    print(f"Saved plot to {png_path}")


def main() -> None:
    args = make_parser("Computed torque sin trajectory using Willow model").parse_args()

    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(CONTROL_RATE)
    model = pyflorid.WillowModel()

    lower, upper, amplitudes = build_trajectory()
    print(f"Center position: {CENTER_POS}")
    print(f"Amplitudes: {amplitudes}")
    print(f"Frequency: {FREQUENCY} Hz, duration: {DURATION} s")

    move_to_start(arm, CENTER_POS)

    arm.switch_control_mode(pyflorid.ControlMode.Torque)
    wait_for_valid_state(arm)

    dt = 1.0 / CONTROL_RATE
    omega = 2.0 * np.pi * FREQUENCY

    t_log: list[float] = []
    desired_pos_log: list[np.ndarray] = []
    actual_pos_log: list[np.ndarray] = []
    desired_vel_log: list[np.ndarray] = []
    actual_vel_log: list[np.ndarray] = []
    desired_acc_log: list[np.ndarray] = []
    gravity_log: list[np.ndarray] = []
    coriolis_log: list[np.ndarray] = []
    inertia_ff_log: list[np.ndarray] = []
    feedback_log: list[np.ndarray] = []
    total_log: list[np.ndarray] = []
    commanded_log: list[np.ndarray] = []

    print("\nStarting computed torque sin trajectory. Press Ctrl+C to stop.")
    start_time = time.time()
    step = 0
    try:
        while (time.time() - start_time) < DURATION:
            loop_start = time.time()

            state = arm.update()
            ensure_safe_mode(state)
            q = state.q.astype(np.float32)
            dq = state.dq.astype(np.float32)

            t = loop_start - start_time
            q_des = CENTER_POS + amplitudes * np.sin(omega * t + PHASE_OFFSETS)
            dq_des = amplitudes * omega * np.cos(omega * t + PHASE_OFFSETS)
            ddq_des = -amplitudes * (omega ** 2) * np.sin(omega * t + PHASE_OFFSETS)
            q_des = np.clip(q_des, lower, upper)

            pos_err = q_des - q
            vel_err = dq_des - dq

            gravity_tau = model.gravity(q, state.base_gravity.astype(np.float32))
            coriolis_tau = model.coriolis(q, dq)
            mass_matrix = model.mass(q)
            inertia_ff_tau = mass_matrix @ ddq_des
            feedback_tau = mass_matrix @ (CT_KP * pos_err + CT_KD * vel_err)

            total_tau = gravity_tau + coriolis_tau + inertia_ff_tau + feedback_tau
            commanded_tau = clip_torque(total_tau, TAU_LIMIT)

            torque_cmd = pyflorid.Torques()
            torque_cmd.tau = commanded_tau.astype(np.float32)
            torque_cmd.kp = np.zeros(6, dtype=np.float32)
            torque_cmd.kd = np.zeros(6, dtype=np.float32)

            arm.write_once(torque_cmd)

            t_log.append(t)
            desired_pos_log.append(q_des.copy())
            actual_pos_log.append(q.copy())
            desired_vel_log.append(dq_des.copy())
            actual_vel_log.append(dq.copy())
            desired_acc_log.append(ddq_des.copy())
            gravity_log.append(gravity_tau.copy())
            coriolis_log.append(coriolis_tau.copy())
            inertia_ff_log.append(inertia_ff_tau.copy())
            feedback_log.append(feedback_tau.copy())
            total_log.append(total_tau.copy())
            commanded_log.append(commanded_tau.copy())

            if step % 50 == 0:
                print(
                    f"\rt={t:.2f}s | "
                    f"J1:{q_des[0]:.3f}/{q[0]:.3f} "
                    f"J2:{q_des[1]:.3f}/{q[1]:.3f} "
                    f"J3:{q_des[2]:.3f}/{q[2]:.3f} "
                    f"J4:{q_des[3]:.3f}/{q[3]:.3f}",
                    end="",
                    flush=True,
                )

            step += 1
            elapsed = time.time() - loop_start
            if elapsed < dt:
                time.sleep(dt - elapsed)
    except KeyboardInterrupt:
        print("\nStopped by user.")

    print()
    settle_in_torque(arm, model)

    result = {
        "time": np.array(t_log, dtype=np.float32),
        "desired_pos": np.array(desired_pos_log, dtype=np.float32),
        "actual_pos": np.array(actual_pos_log, dtype=np.float32),
        "desired_vel": np.array(desired_vel_log, dtype=np.float32),
        "actual_vel": np.array(actual_vel_log, dtype=np.float32),
        "desired_acc": np.array(desired_acc_log, dtype=np.float32),
        "gravity_torque": np.array(gravity_log, dtype=np.float32),
        "coriolis_torque": np.array(coriolis_log, dtype=np.float32),
        "inertia_ff_torque": np.array(inertia_ff_log, dtype=np.float32),
        "feedback_torque": np.array(feedback_log, dtype=np.float32),
        "total_torque": np.array(total_log, dtype=np.float32),
        "commanded_torque": np.array(commanded_log, dtype=np.float32),
        "ct_kp": CT_KP.copy(),
        "ct_kd": CT_KD.copy(),
    }

    save_results(Path(__file__).resolve().parent / "plots", result)


if __name__ == "__main__":
    main()
