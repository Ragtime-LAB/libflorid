"""Echo arm state — prints robot telemetry continuously."""
import sys
import pyflorid


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <ip>", file=sys.stderr)
        sys.exit(1)

    ip = sys.argv[1]
    arm = pyflorid.Arm(ip)
    print("Reading arm state...")

    while True:
        state = arm.update()
        print(
            f"[mode={state.mode}] "
            f"q={state.q} "
            f"errs=0x{state.errors:x} "
            f"F_ext={state.F_ext} "
            f"g_base={state.base_gravity}",
            end="\r",
        )
        if state.mode == 3 or state.mode == 4:  # Fault / EStop
            print("\nArm fault.")
            break


if __name__ == "__main__":
    main()
