"""Generate a six-axis multisine inside two measured Willow endpoint poses.

This is an offline-only design/preview tool.  It never opens the arm.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

import numpy as np
import pinocchio as pin


ROOT = Path(__file__).resolve().parent
DYNAMICS_URDF = (
    ROOT.parent
    / "static_gravity_calibration/model/identified/"
    / "Ragtime_Willow.static-mass-com-calibrated.urdf"
)
COLLISION_URDF = (
    ROOT.parent / "friction_calibration/model/Ragtime_Willow_description.urdf"
)
OUTPUT = ROOT / "dynamic_two_pose_excitation_design.npz"
REPORT = ROOT / "dynamic_two_pose_excitation_report.json"

RATE_HZ = 500.0
DURATION_S = 30.0
CANDIDATES = 96
SEED = 20260829

# Measured configurations supplied by the operator.  Their elementwise minimum
# and maximum define the allowed design envelope; neither row is assumed to be
# a kinematic lower/upper limit as a whole pose.
ENDPOINT_A_DEG = np.array([85.76, 124.84, 104.42, -37.06, -15.22, 128.46])
ENDPOINT_B_DEG = np.array([17.89, 6.02, 0.36, 19.92, 24.25, 154.58])
LOWER_DEG = np.minimum(ENDPOINT_A_DEG, ENDPOINT_B_DEG)
UPPER_DEG = np.maximum(ENDPOINT_A_DEG, ENDPOINT_B_DEG)
# Installation constraint: the gripper can hit the table when J3 folds below
# 30 degrees.  This overrides the J3 value in endpoint B.
LOWER_DEG[2] = 30.0
CENTER_DEG = 0.5 * (LOWER_DEG + UPPER_DEG)
HALF_RANGE_DEG = 0.5 * (UPPER_DEG - LOWER_DEG)

# Three incommensurate components per axis.  Absolute weights sum to one, so
# every generated position is mathematically contained in the endpoint box.
FREQUENCIES_HZ = np.array(
    [
        [0.11, 0.29, 0.53],
        [0.09, 0.23, 0.41],
        [0.11, 0.27, 0.45],
        [0.19, 0.41, 0.71],
        [0.23, 0.43, 0.73],
        [0.27, 0.47, 0.79],
    ],
    dtype=float,
)
WEIGHTS = np.array([0.52, 0.31, 0.17], dtype=float)
MAX_DQ_DEG_S = np.array([55.0, 75.0, 75.0, 75.0, 80.0, 45.0])
MAX_DDQ_DEG_S2 = np.array([180.0, 260.0, 280.0, 300.0, 340.0, 180.0])
MIN_CLEARANCE_M = 0.005
COLLISION_CHECK_STRIDE = 10  # 50 Hz geometric audit of a 500 Hz design
# The operator verified both endpoint poses and the intended workspace on the
# physical arm.  Keep the old-mesh result in the report for diagnosis, but do
# not use it as a gate for this installation-specific design.
ENFORCE_COLLISION_PREVIEW = False


def trajectory(phases: np.ndarray):
    t = np.arange(0.0, DURATION_S, 1.0 / RATE_HZ)
    q = np.repeat(np.deg2rad(CENTER_DEG)[None, :], len(t), axis=0)
    dq = np.zeros_like(q)
    ddq = np.zeros_like(q)
    for joint in range(6):
        for harmonic, frequency in enumerate(FREQUENCIES_HZ[joint]):
            amplitude = np.deg2rad(HALF_RANGE_DEG[joint] * WEIGHTS[harmonic])
            omega = 2.0 * np.pi * frequency
            phase = phases[joint, harmonic]
            q[:, joint] += amplitude * np.sin(omega * t + phase)
            dq[:, joint] += amplitude * omega * np.cos(omega * t + phase)
            ddq[:, joint] -= amplitude * omega**2 * np.sin(omega * t + phase)
    return t, q, dq, ddq


def feasible(q: np.ndarray, dq: np.ndarray, ddq: np.ndarray) -> bool:
    q_deg = np.rad2deg(q)
    return bool(
        np.all(q_deg >= LOWER_DEG - 1e-9)
        and np.all(q_deg <= UPPER_DEG + 1e-9)
        and np.all(np.abs(np.rad2deg(dq)) <= MAX_DQ_DEG_S)
        and np.all(np.abs(np.rad2deg(ddq)) <= MAX_DDQ_DEG_S2)
    )


def regressor_quality(model, q, dq, ddq):
    data = model.createData()
    blocks = []
    for qi, dqi, ddqi in zip(q[::25], dq[::25], ddq[::25]):
        rigid = np.asarray(pin.computeJointTorqueRegressor(model, data, qi, dqi, ddqi))
        # Include viscous, directional Coulomb, actuator inertia and bias columns.
        blocks.append(
            np.column_stack(
                (rigid, np.diag(dqi), np.diag(np.tanh(dqi / 0.01)), np.diag(ddqi), np.eye(6))
            )
        )
    matrix = np.vstack(blocks)
    norms = np.linalg.norm(matrix, axis=0)
    active = norms > 1e-10
    singular = np.linalg.svd(matrix[:, active] / norms[active], compute_uv=False)
    effective = singular[singular > singular[0] * 1e-8]
    return float(effective[0] / effective[-1]), int(len(effective))


def collision_check(q: np.ndarray):
    model = pin.buildModelFromUrdf(str(COLLISION_URDF))
    geometry = pin.buildGeomFromUrdf(
        model,
        str(COLLISION_URDF),
        pin.GeometryType.COLLISION,
        [str(COLLISION_URDF.parent)],
    )
    geometry.addAllCollisionPairs()
    model_data = model.createData()
    geometry_data = pin.GeometryData(geometry)
    minimum = float("inf")
    closest = None
    checked = 0
    for sample in range(0, len(q), COLLISION_CHECK_STRIDE):
        pin.computeDistances(model, model_data, geometry, geometry_data, q[sample])
        checked += 1
        for index, pair in enumerate(geometry.collisionPairs):
            first = geometry.geometryObjects[pair.first]
            second = geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                continue
            distance = float(geometry_data.distanceResults[index].min_distance)
            if distance < minimum:
                minimum = distance
                closest = (sample, first.name, second.name)
    return minimum, closest, checked


def main():
    for path in (DYNAMICS_URDF, COLLISION_URDF):
        if not path.is_file():
            raise FileNotFoundError(path)
    model = pin.buildModelFromUrdf(str(DYNAMICS_URDF))
    rng = np.random.default_rng(SEED)
    best = None
    feasible_count = 0
    for candidate in range(CANDIDATES):
        phases = rng.uniform(-np.pi, np.pi, size=(6, 3))
        t, q, dq, ddq = trajectory(phases)
        if not feasible(q, dq, ddq):
            continue
        feasible_count += 1
        condition, rank = regressor_quality(model, q, dq, ddq)
        if best is None or condition < best[0]:
            best = (condition, rank, candidate, phases, t, q, dq, ddq)
    if best is None:
        raise RuntimeError("no feasible two-pose multisine candidate")
    condition, rank, candidate, phases, t, q, dq, ddq = best
    minimum, closest, checked = collision_check(q)
    payload = {
        "preview_only": True,
        "endpoint_a_deg": ENDPOINT_A_DEG.tolist(),
        "endpoint_b_deg": ENDPOINT_B_DEG.tolist(),
        "center_deg": CENTER_DEG.tolist(),
        "allowed_min_deg": LOWER_DEG.tolist(),
        "allowed_max_deg": UPPER_DEG.tolist(),
        "actual_min_deg": np.rad2deg(q.min(axis=0)).tolist(),
        "actual_max_deg": np.rad2deg(q.max(axis=0)).tolist(),
        "peak_dq_deg_s": np.rad2deg(np.max(np.abs(dq), axis=0)).tolist(),
        "peak_ddq_deg_s2": np.rad2deg(np.max(np.abs(ddq), axis=0)).tolist(),
        "duration_s": DURATION_S,
        "rate_hz": RATE_HZ,
        "samples": len(t),
        "candidates": CANDIDATES,
        "feasible_candidates": feasible_count,
        "selected_candidate": candidate,
        "normalized_regressor_condition": condition,
        "effective_rank": rank,
        "collision_samples_checked": checked,
        "minimum_nonadjacent_distance_m": minimum,
        "minimum_requested_clearance_m": MIN_CLEARANCE_M,
        "collision_preview_passed": minimum >= MIN_CLEARANCE_M,
        "collision_preview_enforced": ENFORCE_COLLISION_PREVIEW,
        "operator_physical_workspace_override": not ENFORCE_COLLISION_PREVIEW,
        "closest_pair": None
        if closest is None
        else {"sample": closest[0], "first": closest[1], "second": closest[2]},
        "dynamics_urdf_sha256": hashlib.sha256(DYNAMICS_URDF.read_bytes()).hexdigest(),
        "collision_urdf_sha256": hashlib.sha256(COLLISION_URDF.read_bytes()).hexdigest(),
    }
    np.savez_compressed(
        OUTPUT,
        t=t,
        q=q,
        dq=dq,
        ddq=ddq,
        phases=phases,
        frequencies_hz=FREQUENCIES_HZ,
        endpoint_a_deg=ENDPOINT_A_DEG,
        endpoint_b_deg=ENDPOINT_B_DEG,
    )
    REPORT.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))
    print("design:", OUTPUT)
    if minimum < MIN_CLEARANCE_M and ENFORCE_COLLISION_PREVIEW:
        raise RuntimeError(
            f"offline collision preview failed: {minimum * 1000:.3f} mm; closest={closest}"
        )
    if minimum < MIN_CLEARANCE_M:
        print("WARNING: old-mesh collision result retained for reference; "
              "operator physical-workspace override is active")


if __name__ == "__main__":
    main()
