"""Set only Willow joint 4's current physical pose as its encoder zero.

No motion command is sent.  The arm is explicitly disabled before the
calibration request.  Joints 1, 2, 3, 5, 6 and the gripper are never passed to
set_zero_point().
"""

from __future__ import annotations

import time

import numpy as np
import pyflorid


DEVICE_URI = "usb:///dev/ttyACM0"
JOINT_ID = 4
CONFIRMATION_PHRASE = "SET J4 ZERO"
READ_TIMEOUT_SECONDS = 2.0


def read_valid(arm):
    deadline = time.monotonic() + READ_TIMEOUT_SECONDS
    while time.monotonic() < deadline:
        state = arm.read_once()
        if int(state.seq) != 0:
            return state
        time.sleep(0.001)
    raise TimeoutError("no valid ArmState received")


def print_pose(label: str, state) -> None:
    q_deg = np.rad2deg(np.asarray(state.q, dtype=float))
    print(label, np.array2string(q_deg, precision=3, separator=", "), "deg")


def main() -> None:
    print("SINGLE-JOINT ZEROING: J4 only")
    print("No motion command will be sent; all axes are disabled first.")
    print("Place J4 manually at the desired mechanical zero before confirming.")
    print(f"Opening {DEVICE_URI} ...")
    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")

    arm.disable()
    time.sleep(0.5)
    before = read_valid(arm)
    print_pose("BEFORE q_deg =", before)
    phrase = input(f'Type exactly "{CONFIRMATION_PHRASE}" to rewrite J4 zero: ')
    if phrase != CONFIRMATION_PHRASE:
        print("Confirmation mismatch. Nothing changed.")
        return

    print("Calling arm.set_zero_point(4) ...")
    if not arm.set_zero_point(JOINT_ID):
        raise RuntimeError("firmware rejected set_zero_point(4)")
    time.sleep(1.0)
    after = read_valid(arm)
    print_pose("AFTER  q_deg =", after)
    after_j4_deg = float(np.rad2deg(np.asarray(after.q, dtype=float)[3]))
    if abs(after_j4_deg) <= 2.0:
        print(f"J4 ZERO OK: feedback={after_j4_deg:.3f} deg")
    else:
        print(f"WARNING: set request succeeded but J4 feedback={after_j4_deg:.3f} deg; reconnect and verify")
    print("Other joint zero points were not addressed by this script.")


if __name__ == "__main__":
    main()
