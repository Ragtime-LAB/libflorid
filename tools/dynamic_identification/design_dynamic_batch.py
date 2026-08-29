"""Generate a resumable half-hour Willow dynamics-identification batch."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

import numpy as np
import pinocchio as pin

import design_two_pose_multisine as design


ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = ROOT / "batch_designs"
MANIFEST = OUTPUT_DIR / "manifest.json"
BATCH_RUNS = 48
SEED = 20260830
COLLISION_CHECK_STRIDE = 50  # 10 Hz for trajectories whose highest component is < 0.8 Hz
MIN_MODEL_CLEARANCE_M = 0.001


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def collision_context():
    model = pin.buildModelFromUrdf(str(design.COLLISION_URDF))
    geometry = pin.buildGeomFromUrdf(
        model, str(design.COLLISION_URDF), pin.GeometryType.COLLISION,
        [str(design.COLLISION_URDF.parent)])
    geometry.addAllCollisionPairs()
    return model, model.createData(), geometry, pin.GeometryData(geometry)


def collision_audit(context, q):
    model, model_data, geometry, geometry_data = context
    minimum = float("inf"); closest = None; checked = 0
    for sample in range(0, len(q), COLLISION_CHECK_STRIDE):
        pin.computeDistances(model, model_data, geometry, geometry_data, q[sample]);checked += 1
        for index, pair in enumerate(geometry.collisionPairs):
            first = geometry.geometryObjects[pair.first];second = geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                continue
            distance = float(geometry_data.distanceResults[index].min_distance)
            if distance < minimum:
                minimum = distance;closest = (sample, first.name, second.name)
    return minimum, closest, checked


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    model = pin.buildModelFromUrdf(str(design.DYNAMICS_URDF))
    collision = collision_context()
    rng = np.random.default_rng(SEED)
    entries = []
    for run_index in range(BATCH_RUNS):
        accepted = None
        for phase_attempt in range(100):
            phases = rng.uniform(-np.pi, np.pi, size=(6, 3))
            t, q, dq, ddq = design.trajectory(phases)
            if not design.feasible(q, dq, ddq):
                continue
            condition, rank = design.regressor_quality(model, q, dq, ddq)
            if rank < 58 or condition >= 90.0:
                continue
            minimum, closest, checked = collision_audit(collision, q)
            if minimum >= MIN_MODEL_CLEARANCE_M:
                accepted = (phases, t, q, dq, ddq, condition, rank, phase_attempt,
                            minimum, closest, checked)
                break
        if accepted is None:
            raise RuntimeError(f"could not generate acceptable design {run_index}")
        phases, t, q, dq, ddq, condition, rank, phase_attempt, minimum, closest, checked = accepted
        path = OUTPUT_DIR / f"design_{run_index:03d}.npz"
        np.savez_compressed(
            path, t=t, q=q, dq=dq, ddq=ddq, phases=phases,
            frequencies_hz=design.FREQUENCIES_HZ,
            endpoint_a_deg=design.ENDPOINT_A_DEG,
            endpoint_b_deg=design.ENDPOINT_B_DEG,
        )
        entries.append({
            "run_index": run_index,
            "file": path.name,
            "sha256": sha256(path),
            "samples": len(t),
            "duration_s": design.DURATION_S,
            "condition": condition,
            "effective_rank": rank,
            "phase_attempt": phase_attempt,
            "hppfcl_checked_samples": checked,
            "hppfcl_minimum_distance_m": minimum,
            "hppfcl_closest": {"sample": closest[0], "first": closest[1], "second": closest[2]},
            "q_min_deg": np.rad2deg(q.min(axis=0)).tolist(),
            "q_max_deg": np.rad2deg(q.max(axis=0)).tolist(),
            "peak_dq_deg_s": np.rad2deg(np.max(np.abs(dq), axis=0)).tolist(),
            "peak_ddq_deg_s2": np.rad2deg(np.max(np.abs(ddq), axis=0)).tolist(),
        })
        print(f"[{run_index + 1}/{BATCH_RUNS}] rank={rank} condition={condition:.2f} "
              f"clearance={minimum*1000:.2f}mm {path.name}")
    payload = {
        "schema_version": 1,
        "seed": SEED,
        "runs": BATCH_RUNS,
        "active_duration_s": BATCH_RUNS * design.DURATION_S,
        "rate_hz": design.RATE_HZ,
        "operator_workspace_override": True,
        "j3_min_deg": float(design.LOWER_DEG[2]),
        "hppfcl_check_hz": design.RATE_HZ / COLLISION_CHECK_STRIDE,
        "minimum_model_clearance_m": MIN_MODEL_CLEARANCE_M,
        "entries": entries,
    }
    MANIFEST.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print("manifest:", MANIFEST)
    print("active excitation minutes:", payload["active_duration_s"] / 60.0)


if __name__ == "__main__":
    main()
