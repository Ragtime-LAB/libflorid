#!/usr/bin/env python3
"""Second A/B hold: native dq damping plus low-speed wrist stiction take-up."""
from __future__ import annotations

from pathlib import Path

import numpy as np

import run_computed_torque_startup_hold as experiment


experiment.CONFIRMATION_PHRASE = "RUN WILLOW RAW DQ WRIST HOLD"
experiment.HOLD_SECONDS = 120.0

# Preserve the proximal position stiffness that successfully returned J1..J3.
# J4/J5 receive a modest stiffness increase.  J6 is intentionally not assigned
# an enormous acceleration-domain gain: its identified axial inertia is tiny,
# so static-friction take-up is the physically relevant correction.
experiment.OMEGA_N_RAD_S = np.array([3.0, 3.0, 3.0, 3.0, 3.0, 4.0])
experiment.KP_CT = experiment.OMEGA_N_RAD_S**2

# The desired hold velocity is zero.  Use the firmware's native 500 Hz dq and
# map -dq to a modest, explicit torque damping.  This is equivalent to the
# configuration-dependent acceleration gain M(q)^-1 B, but its physical size
# is readable and does not collapse on the very-low-inertia J5/J6 axes.
experiment.KD_CT = np.zeros(6, dtype=float)
experiment.DAMPING_DQ_SOURCE = "raw"
experiment.JOINT_TORQUE_DAMPING_NM_PER_RAD_S = np.array(
    [0.15, 0.25, 0.12, 0.05, 0.04, 0.03], dtype=float
)
experiment.ENABLE_WRIST_ERROR_DIRECTED_FRICTION = True

experiment.LOG_DIR = (
    Path("/home/kelvinlm/projects/libflorid-main/tools/dynamic_identification")
    / "runs_computed_torque_startup_hold_rawdq_wrist"
)


if __name__ == "__main__":
    experiment.main()
