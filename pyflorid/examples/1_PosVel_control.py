"""Simple point-to-point joint motion sequence using JointPositions commands."""

from __future__ import annotations

import time

import numpy as np

from _common import apply_defaults, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state


def hold_target(control: pyflorid.JointPositionControl, target: np.ndarray, duration: float) -> None:
    start_time = None
    while True:
        state = control.read_once()
        ensure_safe_mode(state)
        if start_time is None:
            start_time = state.time

        cmd = pyflorid.JointPositions()
        cmd.q = target
        cmd.dq = np.zeros(6, dtype=np.float32)
        cmd.kp = np.array([8.0, 15.0, 15.0, 5.0, 3.0, 1.0], dtype=np.float32)
        cmd.kd = np.array([0.8, 1.5, 1.5, 0.6, 0.3, 0.1], dtype=np.float32)
        control.write_once(cmd)

        if state.time - start_time >= duration:
            return


def main() -> None:
    args = make_parser("Joint position sequence").parse_args()
    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(100.0)
    arm.switch_control_mode(pyflorid.ControlMode.JointPosition)
    wait_for_valid_state(arm)

    zero = np.zeros(6, dtype=np.float32)
    pos1 = np.array([0.0, 0.8, 0.8, 0.3, 0.0, 0.0], dtype=np.float32)
    pos2 = np.array([0.0, 1.2, 1.2, 0.4, 0.0, -1.0], dtype=np.float32)

    control = arm.start_joint_position_control()
    print("Executing motion sequence. Press Ctrl+C to stop.")
    try:
        for name, target in (
            ("zero", zero),
            ("pos2", pos2),
            ("pos1", pos1),
            ("pos2", pos2),
            ("zero", zero),
        ):
            print(f"\nMoving to {name}: {target.round(3)}")
            hold_target(control, target, duration=3.0)
            time.sleep(2.0)
        print("\nSequence complete.")
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
