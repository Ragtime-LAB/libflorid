#!/usr/bin/env python3
"""J1-only breakaway collection around measured startup q1; disabled by default."""
from pathlib import Path
import run_friction_collection as base
import run_breakaway_collection as collector

base.ACTIVE_JOINTS = (0,)
base.RELATIVE_SWEEP_JOINTS = (0,)
collector.OUTPUT_DIR = base.ROOT / "runs_j1_breakaway"
collector.RAMP_RATE_NM_S = collector.RAMP_RATE_NM_S.copy(); collector.RAMP_RATE_NM_S[0] = 0.05
collector.MAX_RAMP_NM = collector.MAX_RAMP_NM.copy(); collector.MAX_RAMP_NM[0] = 1.5
collector.CONFIRMATION_PHRASE = "ENABLE WILLOW J1 RELATIVE BREAKAWAY CALIBRATION"
collector.ENABLE_HARDWARE = False
collector.COLLECTOR_SCRIPT = Path(__file__).resolve()

if __name__ == "__main__":
    collector.main()
