#!/usr/bin/env python3
"""250 Hz ten-motor USB Bulk stream, firmware >= 0.11.0.

Default: receive status only. --control mit-gravity streams gravity compensation;
--control mit streams zero-torque MIT. Edit the ten-element command arrays to
supply position/velocity/gains/feedforward in motor coordinates.
"""
import argparse
import time
from pathlib import Path
import sys

# Match tools/underwater_console/run.py when running from this source worktree.
repo = Path(__file__).resolve().parents[1]
local_extension = repo / "build-underwater-sdk" / "pyflorid"
if local_extension.is_dir():
    sys.path.insert(0, str(repo / "pyflorid"))
    sys.path.insert(0, str(local_extension))
import pyflorid as f


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("uri", help="Bulk URI from discover_devices()")
    parser.add_argument("--control", choices=("mit", "mit-gravity"))
    parser.add_argument("--seconds", type=float, default=10.0)
    args = parser.parse_args()
    with f.UnderwaterTeleop.connect(args.uri) as device:
        command = f.UnderwaterArmCommand()
        command.kind = (f.UnderwaterControlKind.MitWithGravity if args.control == "mit-gravity"
                        else f.UnderwaterControlKind.MIT)
        # Order: master j1..j4, gripper, slave j1..j4, gripper.
        # All arrays default to zero. MIT: tau_ff=tau; MitWithGravity: tau_ff=tau+g(q).
        try:
            if args.control:
                reply = device.enter_control()
                if not reply:
                    raise RuntimeError(f"Control: {reply.error}, {reply.info.reason if reply.info else ''}")
            start = deadline = report = time.monotonic()
            last_seq = received = missed = sent = 0
            while time.monotonic() - start < args.seconds:
                if args.control:
                    result = device.write_arm_command(command)
                    if not result:
                        raise RuntimeError(f"write: {result.error}")
                    sent += 1
                status = device.read_arm_status()
                if status is not None and status.sequence != last_seq:
                    if last_seq:
                        missed += max(0, ((status.sequence - last_seq) & 0xFFFFFFFF) - 1)
                    last_seq = status.sequence
                    received += 1
                now = time.monotonic()
                if now - report >= 1.0:
                    if status is not None:
                        print(f"mode={status.mode.name} tx={sent} rx={received} skipped={missed} "
                              f"fresh=0x{status.fresh_mask:03x} applied=0x{status.applied_mask:03x} "
                              f"cmd_seq={status.applied_command_sequence} reject={status.command_reject_reason} "
                              f"stop={status.control_stop_reason} q={status.q}")
                    else:
                        print("waiting ArmStatus")
                    sent = received = missed = 0
                    report = now
                deadline += 0.004
                delay = deadline - time.monotonic()
                if delay > 0:
                    time.sleep(delay)
                else:
                    deadline = time.monotonic()  # report/OS stall: do not burst stale commands
        except KeyboardInterrupt:
            pass
        finally:
            if args.control:
                reply = device.disable()
                print(f"disable: accepted={bool(reply)} error={reply.error}")


if __name__ == "__main__":
    main()
