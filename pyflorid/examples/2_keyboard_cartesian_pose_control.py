"""Keyboard control of end-effector Cartesian pose using direct CartesianPose commands."""

from __future__ import annotations

import threading
import time

import numpy as np

from _common import apply_defaults, ensure_safe_mode, make_parser, pyflorid, wait_for_valid_state

try:
    from pynput import keyboard
except ImportError as exc:  # pragma: no cover - runtime dependency
    raise SystemExit("pynput is required: pip install pynput") from exc


def rot_x(angle: float) -> np.ndarray:
    c, s = np.cos(angle), np.sin(angle)
    return np.array(
        [[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]],
        dtype=np.float32,
    )


def rot_y(angle: float) -> np.ndarray:
    c, s = np.cos(angle), np.sin(angle)
    return np.array(
        [[c, 0.0, s], [0.0, 1.0, 0.0], [-s, 0.0, c]],
        dtype=np.float32,
    )


def rot_z(angle: float) -> np.ndarray:
    c, s = np.cos(angle), np.sin(angle)
    return np.array(
        [[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]],
        dtype=np.float32,
    )


def main() -> None:
    parser = make_parser(
        "Keyboard Cartesian pose control",
        (("--step",), {"type": float, "default": 0.005, "help": "Position step in meters"}),
        (("--rot-step",), {"type": float, "default": 0.03, "help": "Rotation step in radians"}),
    )
    args = parser.parse_args()

    arm = pyflorid.Arm(args.ip)
    apply_defaults(arm)
    arm.set_max_frequency_hz(100.0)
    arm.set_cartesian_impedance(np.array([600, 600, 600, 30, 30, 30], dtype=np.float32))
    arm.switch_control_mode(pyflorid.ControlMode.CartesianPose)

    state = wait_for_valid_state(arm)
    ensure_safe_mode(state)

    target_T = np.array(state.O_T_EE, dtype=np.float32)
    target_lock = threading.Lock()
    running = True
    target_dirty = True

    print("Keyboard Cartesian pose control")
    print("  w/s: X +/-")
    print("  a/d: Y +/-")
    print("  q/e: Z +/-")
    print("  1/2: Rx +/-")
    print("  3/4: Ry +/-")
    print("  5/6: Rz +/-")
    print("  esc: quit")
    print(f"Initial pose position = {target_T[:3, 3].round(4)}")

    def on_press(key) -> None:
        nonlocal running, target_T, target_dirty
        with target_lock:
            if hasattr(key, "char") and key.char:
                ch = key.char.lower()
                if ch == "w":
                    target_T[0, 3] += args.step
                elif ch == "s":
                    target_T[0, 3] -= args.step
                elif ch == "a":
                    target_T[1, 3] += args.step
                elif ch == "d":
                    target_T[1, 3] -= args.step
                elif ch == "q":
                    target_T[2, 3] += args.step
                elif ch == "e":
                    target_T[2, 3] -= args.step
                elif ch == "1":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_x(args.rot_step)
                elif ch == "2":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_x(-args.rot_step)
                elif ch == "3":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_y(args.rot_step)
                elif ch == "4":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_y(-args.rot_step)
                elif ch == "5":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_z(args.rot_step)
                elif ch == "6":
                    target_T[:3, :3] = target_T[:3, :3] @ rot_z(-args.rot_step)
                else:
                    ch = None

                if ch is not None:
                    target_dirty = True

        if key == keyboard.Key.esc:
            running = False
            return False
        return None

    listener = keyboard.Listener(on_press=on_press)
    listener.start()

    control = arm.start_cartesian_pose_control()
    last_print = 0.0

    try:
        while running:
            state = control.read_once()
            ensure_safe_mode(state)

            with target_lock:
                current_target = target_T.copy()
                should_send = target_dirty
                if should_send:
                    target_dirty = False

            if should_send:
                cmd = pyflorid.CartesianPose()
                cmd.T = current_target
                cmd.kp = np.array([8.0, 15.0, 15.0, 5.0, 3.0, 1.0], dtype=np.float32)
                cmd.kd = np.array([0.8, 1.5, 1.5, 0.6, 0.3, 0.1], dtype=np.float32)
                control.write_once(cmd)

            if state.time - last_print >= 0.2:
                last_print = state.time
                current_pos = np.array(state.O_T_EE[:3, 3], dtype=np.float32)
                target_pos = np.array(current_target[:3, 3], dtype=np.float32)
                print(
                    f"\rtarget={target_pos.round(4)} current={current_pos.round(4)}",
                    end="",
                    flush=True,
                )

            time.sleep(0.01)
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        listener.stop()
        print("\nStopped.")


if __name__ == "__main__":
    main()
