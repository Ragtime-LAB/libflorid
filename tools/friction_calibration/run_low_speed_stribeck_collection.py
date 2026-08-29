#!/usr/bin/env python3
"""Resume-safe low-speed supplement for Stribeck identification."""
from pathlib import Path

import run_friction_collection as collector

ROOT = Path(__file__).resolve().parent
collector.OUTPUT_DIR = ROOT / "runs_low_speed"
collector.ENTRY_SCRIPT = Path(__file__).resolve()
collector.SPEED_LEVELS_DEG_S = (0.5, 1.0)
collector.REPEATS = 3
collector.CONFIRMATION_PHRASE = "ENABLE WILLOW LOW SPEED FRICTION CALIBRATION"
collector.ENABLE_HARDWARE = False
collector.SWEEP_DEG = {
    1: (60.0, 80.0),
    2: (70.0, 90.0),
    3: (-10.0, 10.0),
    4: (-10.0, 10.0),
    5: (-10.0, 10.0),
}

if __name__ == "__main__":
    collector.main()
