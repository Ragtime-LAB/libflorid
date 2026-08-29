#!/usr/bin/env python3
"""Second-stage bounded super-twisting computed-torque launcher."""
from __future__ import annotations

import run_computed_torque_demo as demo


demo.ROBUST_LAYER_ENABLED = True
demo.REQUIRE_PLAIN_REPORT_FOR_ROBUST = True
demo.REQUIRE_PRIOR_STAGE_ID = None
demo.REQUIRE_PRIOR_TRACKING_JOINTS = None
demo.SPEED_SCALE = 0.30
demo.CONFIRMATION_PHRASE = "RUN GUARDED WILLOW ROBUST COMPUTED TORQUE"
# Keep disabled until the plain CTC stage has a passing real telemetry report.
demo.ENABLE_HARDWARE = False


if __name__ == "__main__":
    demo.main()
