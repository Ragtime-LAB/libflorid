"""Read and print robot telemetry continuously."""

from __future__ import annotations

import time

from _common import ensure_safe_mode, make_parser, pyflorid


def main() -> None:
    args = make_parser("Read robot joint state").parse_args()
    arm = pyflorid.Arm(args.ip)

    print("Reading arm state. Press Ctrl+C to stop.")
    try:
        while True:
            state = arm.update()
            ensure_safe_mode(state)
            print(
                f"time={state.time:8.3f} "
                f"mode={state.mode} "
                f"q={state.q.round(4)} "
                f"dq={state.dq.round(4)} "
                f"tau={state.tau.round(4)} "
                f"F_ext={state.F_ext.round(4)}",
                end="\r",
                flush=True,
            )
            time.sleep(0.05)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
