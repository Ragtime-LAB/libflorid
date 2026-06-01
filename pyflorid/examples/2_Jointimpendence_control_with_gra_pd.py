"""Joint impedance control with gravity compensation."""

from __future__ import annotations

import time

import numpy as np

from _common import apply_defaults, clip_torque, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state


def main() -> None:
    parser = make_parser(
        "Joint impedance with gravity compensation",
        (("--model",), {"choices": ("panthera", "willow"), "default": "willow"}),
    )
    args = parser.parse_args()

    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(200.0)
    model = pyflorid.PantheraModel() if args.model == "panthera" else pyflorid.WillowModel()

    K = np.array([4.0, 10.0, 10.0, 2.0, 2.0, 1.0], dtype=np.float32)
    B = np.array([0.5, 0.8, 0.8, 0.2, 0.2, 0.1], dtype=np.float32)
    q_des = np.array([0.0, 0.7, 0.7, -0.1, 0.0, 0.0], dtype=np.float32)
    v_des = np.zeros(6, dtype=np.float32)
    tau_limit = np.array([10.0, 20.0, 20.0, 10.0, 5.0, 5.0], dtype=np.float32)

    wait_for_valid_state(arm)
    control = arm.start_torque_control()
    print(f"Joint impedance + gravity compensation using {args.model} model. Press Ctrl+C to stop.")
    count = 0
    try:
        while True:
            state = control.read_once()
            ensure_safe_mode(state)

            q = state.q.astype(np.float32)
            dq = state.dq.astype(np.float32)
            g = model.gravity(q, state.base_gravity.astype(np.float32))
            tau_imp = K * (q_des - q) + B * (v_des - dq)
            tau = clip_torque(g + tau_imp, tau_limit)

            cmd = pyflorid.Torques()
            cmd.tau = tau
            cmd.kp = np.zeros(6, dtype=np.float32)
            cmd.kd = np.zeros(6, dtype=np.float32)
            control.write_once(cmd)

            count += 1
            if count % 200 == 0:
                print(
                    f"\ncycle={count}"
                    f"\nq={q.round(4)}"
                    f"\nq_des={q_des.round(4)}"
                    f"\nimp_tau={tau_imp.round(4)}"
                    f"\ngravity_tau={g.round(4)}"
                    f"\ntotal_tau={tau.round(4)}"
                )
            time.sleep(0.002)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
