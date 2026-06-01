"""Compute and print forward kinematics from the current joint state."""

from __future__ import annotations

import time

import numpy as np

from _common import make_parser, pyflorid


def rotation_matrix_to_euler_zyx(r: np.ndarray) -> np.ndarray:
    sy = np.sqrt(r[0, 0] * r[0, 0] + r[1, 0] * r[1, 0])
    singular = sy < 1e-6
    if not singular:
        x = np.arctan2(r[2, 1], r[2, 2])
        y = np.arctan2(-r[2, 0], sy)
        z = np.arctan2(r[1, 0], r[0, 0])
    else:
        x = np.arctan2(-r[1, 2], r[1, 1])
        y = np.arctan2(-r[2, 0], sy)
        z = 0.0
    return np.degrees(np.array([x, y, z], dtype=np.float32))


def main() -> None:
    parser = make_parser(
        "Forward kinematics monitor",
        (("--model",), {"choices": ("panthera", "willow"), "default": "willow"}),
    )
    args = parser.parse_args()

    arm = pyflorid.Arm(args.ip)
    model = pyflorid.PantheraModel() if args.model == "panthera" else pyflorid.WillowModel()

    print(f"Forward kinematics monitor using {args.model} model. Press Ctrl+C to stop.")
    try:
        while True:
            state = arm.update()
            q = state.q.astype(np.float32)
            T = model.forward_kinematics(q)
            pos = T[:3, 3]
            euler = rotation_matrix_to_euler_zyx(T[:3, :3])
            print(
                f"\nq={q.round(4)}"
                f"\npos(m)={pos.round(4)}"
                f"\neuler(deg)={euler.round(2)}"
                f"\nT=\n{np.array2string(T, precision=4, suppress_small=True)}"
            )
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
