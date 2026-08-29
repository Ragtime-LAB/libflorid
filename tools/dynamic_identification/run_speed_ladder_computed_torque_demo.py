#!/usr/bin/env python3
"""Evidence-gated next-speed plain computed-torque launcher."""
from __future__ import annotations

import run_computed_torque_demo as demo


# Raise only one stage at a time after the previous real report passes.
TARGET_SPEED_SCALE = 0.45
REQUIRED_PRIOR_SPEED_SCALE = 0.30

demo.SPEED_SCALE = TARGET_SPEED_SCALE
demo.REQUIRE_PRIOR_PLAIN_SPEED_SCALE = REQUIRED_PRIOR_SPEED_SCALE
demo.REQUIRE_PRIOR_STAGE_ID = "six_axis_plain_0p30"
demo.REQUIRE_PRIOR_TRACKING_JOINTS = (1, 2, 3, 4, 5, 6)
demo.REQUIRE_KNOWN_PAYLOAD_TORQUE_VALIDATION = True
demo.ROBUST_LAYER_ENABLED = False
demo.CONFIRMATION_PHRASE = "RUN GUARDED WILLOW NEXT SPEED COMPUTED TORQUE"
demo.ENABLE_HARDWARE = False


if __name__ == "__main__":
    demo.main()
