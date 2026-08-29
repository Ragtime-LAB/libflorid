#!/usr/bin/env python3
"""J1-only relative low-speed Stribeck collection; hardware disabled by default."""
from pathlib import Path
import run_friction_collection as collector

collector.ENTRY_SCRIPT = Path(__file__).resolve()
collector.ACTIVE_JOINTS = (0,)
collector.RELATIVE_SWEEP_JOINTS = (0,)
collector.SPEED_LEVELS_DEG_S = (0.5, 1.0)
collector.SWEEP_DEG = dict(collector.SWEEP_DEG); collector.SWEEP_DEG[0] = (-10.0, 10.0)
collector.OUTPUT_DIR = collector.ROOT / "runs_j1_low_speed"
collector.CONFIRMATION_PHRASE = "ENABLE WILLOW J1 RELATIVE LOW SPEED CALIBRATION"
collector.ENABLE_HARDWARE = False

if __name__ == "__main__":
    collector.main()
