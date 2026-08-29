"""HPP-FCL/coal collision preview for the requested single-joint sweeps."""

from pathlib import Path
import numpy as np
import pinocchio as pin

ROOT = Path(__file__).resolve().parent
_LOCAL_COLLISION_URDF = ROOT / "model" / "Ragtime_Willow_description.urdf"
COLLISION_URDF = (_LOCAL_COLLISION_URDF if _LOCAL_COLLISION_URDF.exists()
                  else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_description.urdf")
PARK_BY_JOINT_DEG = {joint: np.array([0.0, 70.0, 90.0, 0.0, 0.0, 0.0]) for joint in range(1, 6)}
VALIDATION_PARK_BY_JOINT_DEG = {
    1: np.array([0.0, 70.0, 80.0, 0.0, 0.0, 0.0]),
    2: np.array([0.0, 60.0, 90.0, 0.0, 0.0, 0.0]),
    3: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
    4: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
    5: np.array([0.0, 60.0, 80.0, 0.0, 0.0, 0.0]),
}
SWEEP_DEG = {1: (30.0, 90.0), 2: (45.0, 100.0), 3: (-50.0, 50.0), 4: (-10.0, 10.0), 5: (-50.0, 50.0)}
MIN_CLEARANCE_M = 0.005


def main() -> None:
    model = pin.buildModelFromUrdf(str(COLLISION_URDF))
    geometry = pin.buildGeomFromUrdf(model, str(COLLISION_URDF), pin.GeometryType.COLLISION, [str(COLLISION_URDF.parent)])
    geometry.addAllCollisionPairs()
    model_data = model.createData(); geometry_data = pin.GeometryData(geometry)
    rejected = []
    for posture_name, parks in (("train", PARK_BY_JOINT_DEG), ("heldout", VALIDATION_PARK_BY_JOINT_DEG)):
      for joint, (lower, upper) in SWEEP_DEG.items():
        bad = []; minimum_distance = float("inf"); closest = None
        for value in np.linspace(lower, upper, 401):
            q_deg = parks[joint].copy(); q_deg[joint] = value
            pin.computeDistances(model, model_data, geometry, geometry_data, np.deg2rad(q_deg))
            for index, pair in enumerate(geometry.collisionPairs):
                first = geometry.geometryObjects[pair.first]; second = geometry.geometryObjects[pair.second]
                if abs(int(first.parentJoint) - int(second.parentJoint)) <= 1:
                    continue
                distance = float(geometry_data.distanceResults[index].min_distance)
                if distance < minimum_distance:
                    minimum_distance = distance; closest = (float(value), first.name, second.name)
                if distance < MIN_CLEARANCE_M:
                    bad.append((float(value), first.name, second.name, distance)); break
        print(f"{posture_name} J{joint + 1}: [{lower:g}, {upper:g}] deg, checked=401, "
              f"minimum_distance={minimum_distance * 1000:.3f} mm, below_5mm={len(bad)}, closest={closest}")
        rejected.extend((posture_name, joint + 1, *entry) for entry in bad)
    if rejected:
        raise RuntimeError(f"clearance preview rejected {len(rejected)} samples below {MIN_CLEARANCE_M*1000:g}mm; first={rejected[:3]}")
    print(f"HPP-FCL preview passed all requested sweeps with >= {MIN_CLEARANCE_M*1000:g} mm clearance.")


if __name__ == "__main__":
    main()
