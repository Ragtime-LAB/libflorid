#!/usr/bin/env python3
"""A/B diagnostic: computed-torque hold with the host velocity loop disabled.

This intentionally changes one factor relative to the instrumented startup-hold
experiment: ``Kd_ct`` is zero.  Gravity, the identified rigid-body model,
position restoration, friction feedforward, MIT-zero-gain torque transport, and
500 Hz telemetry remain unchanged.  Its purpose is to determine whether the
observed 19.5 Hz oscillation is created by the delayed host velocity feedback.
"""
from __future__ import annotations

from pathlib import Path

import numpy as np

import run_computed_torque_startup_hold as experiment


experiment.CONFIRMATION_PHRASE = "RUN WILLOW KD ZERO ABLATION"
experiment.HOLD_SECONDS = 60.0
# Preserve the exact position gains used by the 22:29:59 comparison run; only
# the velocity-feedback term is removed in this A/B test.
experiment.OMEGA_N_RAD_S = np.array([3.0, 3.0, 3.0, 3.0, 3.0, 3.0])
experiment.KP_CT = experiment.OMEGA_N_RAD_S**2
experiment.KD_CT = np.zeros(6, dtype=float)
experiment.LOG_DIR = (
    Path("/home/kelvinlm/projects/libflorid-main/tools/dynamic_identification")
    / "runs_computed_torque_startup_hold_kd0_ablation"
)


if __name__ == "__main__":
    experiment.main()
