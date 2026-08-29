#!/usr/bin/env python3
"""First-stage computed-torque trial: excite J2 only and hold the other joints."""
from __future__ import annotations

import run_computed_torque_demo as demo


# Change only after a disabled preview and collision/clearance review.
demo.TRACKING_JOINTS = (2,)
demo.SPEED_SCALE = 0.20
demo.REQUIRE_PRIOR_STAGE_ID = None
demo.REQUIRE_PRIOR_TRACKING_JOINTS = None
demo.CONFIRMATION_PHRASE = "RUN GUARDED WILLOW J2 COMPUTED TORQUE"
# The imported module remains disabled by default. Set this True here only for a reviewed real trial.
demo.ENABLE_HARDWARE = False


if __name__ == "__main__":
    demo.main()
