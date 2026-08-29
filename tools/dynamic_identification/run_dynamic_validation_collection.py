#!/usr/bin/env python3
"""Guarded launcher for the independent held-out validation trajectory."""
from pathlib import Path

import run_dynamic_collection as collector

ROOT = Path(__file__).resolve().parent
local_design = ROOT / "dynamic_validation_design.npz"
collector.DESIGN = local_design if local_design.exists() else ROOT.parents[1] / "data/dynamic_validation_design.npz"
collector.RUN_LABEL = "validation"
collector.CONFIRMATION_PHRASE = "RUN WILLOW DYNAMIC VALIDATION"
collector.ENABLE_HARDWARE = False

if __name__ == "__main__":
    collector.main()
