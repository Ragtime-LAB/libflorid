"""Dedicated no-argument launcher for the resume-safe 200-pose pilot."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

import collector

ROOT = Path(__file__).resolve().parent
SOURCE_URDF = ROOT / "model" / "willow-v0.2" / "urdf" / "willow-v0.2.urdf"
SESSION_MANIFEST = ROOT / "runs" / "pilot_200_model_manifest.json"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    if collector.MAX_POSES_THIS_RUN != 200:
        raise RuntimeError("collector.MAX_POSES_THIS_RUN must be exactly 200")
    if not SOURCE_URDF.is_file():
        raise FileNotFoundError(f"latest URDF is missing: {SOURCE_URDF}")
    manifest = collector.load_manifest()
    manifest_ids = {sample_id for sample_id, _ in manifest}
    completed = collector.completed_sample_ids() & manifest_ids
    rejected = collector.rejected_sample_ids() & manifest_ids
    pending = [row for row in manifest if row[0] not in completed and row[0] not in rejected]
    selected = collector.diverse_subset(pending, 200)
    payload = {
        "source_urdf": str(SOURCE_URDF),
        "source_urdf_sha256": sha256(SOURCE_URDF),
        "design_source": "generated exact grid in collector.py",
        "j2_grid_deg": list(collector.J2_GRID_DEG),
        "j3_grid_deg": list(collector.J3_GRID_DEG),
        "j4_grid_deg": list(collector.J4_GRID_DEG),
        "j5_grid_deg": list(collector.J5_GRID_DEG),
        "j6_grid_deg": list(collector.J6_GRID_DEG),
        "max_poses_this_run": 200,
        "completed_before_run": len(completed),
        "rejected_before_run": len(rejected),
        "planned_sample_ids": [sample_id for sample_id, _ in selected],
        "j1_policy": "hold measured startup encoder angle",
    }
    SESSION_MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    SESSION_MANIFEST.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print("200-pose pilot launcher")
    print(json.dumps(payload, indent=2))
    collector.main()


if __name__ == "__main__":
    main()
