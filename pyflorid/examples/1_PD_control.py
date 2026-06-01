"""Joint PD hold using JointPositions commands."""

from __future__ import annotations

import numpy as np

from _common import apply_defaults, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state


def main() -> None:
    parser = make_parser("Joint PD control")
    args = parser.parse_args()

    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(100.0)
    arm.switch_control_mode(pyflorid.ControlMode.JointPosition)

    target_q = np.array([0.0, 0.7, 0.7, -0.1, 0.0, 0.0], dtype=np.float32)
    target_dq = np.zeros(6, dtype=np.float32)
    kp = np.array([8.0, 15.0, 15.0, 5.0, 3.0, 1.0], dtype=np.float32)
    kd = np.array([0.8, 1.5, 1.5, 0.6, 0.3, 0.1], dtype=np.float32)

    wait_for_valid_state(arm)
    control = arm.start_joint_position_control()

    print("Starting joint PD hold. Press Ctrl+C to stop.")
    try:
        while True:
            state = control.read_once()
            ensure_safe_mode(state)

            cmd = pyflorid.JointPositions()
            cmd.q = target_q
            cmd.dq = target_dq
            cmd.kp = kp
            cmd.kd = kd
            control.write_once(cmd)

            err = np.abs(state.q - target_q)
            print(
                f"q={state.q.round(4)} target={target_q.round(4)} err={err.round(4)}",
                end="\r",
                flush=True,
            )
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
