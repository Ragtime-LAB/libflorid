"""Pure gravity compensation using model.gravity()."""

from __future__ import annotations

import time

import numpy as np

from _common import apply_defaults, clip_torque, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state


def main() -> None:
    parser = make_parser(
        "Gravity compensation control",
        (("--model",), {"choices": ("panthera", "willow"), "default": "willow"}),
    )
    args = parser.parse_args()

    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(200.0)
    model = pyflorid.PantheraModel() if args.model == "panthera" else pyflorid.WillowModel()
    tau_limit = np.array([15.0, 30.0, 30.0, 15.0, 5.0, 5.0], dtype=np.float32)

    wait_for_valid_state(arm)
    control = arm.start_torque_control()
    print(f"Gravity compensation using {args.model} model. Press Ctrl+C to stop.")
    count = 0
    try:
        while True:
            state = control.read_once()
            ensure_safe_mode(state)

            tau = model.gravity(state.q.astype(np.float32), state.base_gravity.astype(np.float32))
            tau = clip_torque(tau, tau_limit)

            cmd = pyflorid.Torques()
            cmd.tau = tau
            cmd.kp = np.zeros(6, dtype=np.float32)
            cmd.kd = np.zeros(6, dtype=np.float32)
            control.write_once(cmd)

            count += 1
            if count % 100 == 0:
                print(f"q={state.q.round(4)} gravity_tau={tau.round(4)}")
            time.sleep(0.002)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
