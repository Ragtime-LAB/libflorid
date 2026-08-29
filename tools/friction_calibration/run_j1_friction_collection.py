#!/usr/bin/env python3
"""J1-only relative constant-speed friction collection; hardware disabled by default."""
from pathlib import Path
import run_friction_collection as collector

collector.ENTRY_SCRIPT = Path(__file__).resolve()
collector.ACTIVE_JOINTS = (0,)
collector.RELATIVE_SWEEP_JOINTS = (0,)
collector.OUTPUT_DIR = collector.ROOT / "runs_j1"
collector.CONFIRMATION_PHRASE = "ENABLE WILLOW J1 RELATIVE FRICTION CALIBRATION"
collector.ENABLE_HARDWARE = False

if __name__ == "__main__":
    collector.main()
