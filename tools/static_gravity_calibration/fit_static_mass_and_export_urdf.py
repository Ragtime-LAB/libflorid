"""Fit regularized gravity-observable link mass/COM/offset parameters.

Full rotational inertia tensors cannot be identified from static samples, so
they are deliberately preserved from the latest nominal URDF.
"""

from __future__ import annotations

import csv
import hashlib
import json
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np
import pinocchio as pin
from scipy.optimize import least_squares

ROOT = Path(__file__).resolve().parent
SOURCE_URDF = ROOT / "model" / "willow-v0.2" / "urdf" / "willow-v0.2.urdf"
DATA_CSV = ROOT / "runs" / "static_gravity_j2_30_50_70_j3_45_70_90.csv"
OUTPUT_DIR = ROOT / "model" / "identified"
OUTPUT_URDF = OUTPUT_DIR / "willow-v0.2.static-mass-com-calibrated.urdf"
REPORT_JSON = OUTPUT_DIR / "static_mass_com_fit_report.json"

# These SDK->URDF values must be checked against the physical arm.
JOINT_INDEX = np.arange(6)
Q_SIGN = np.ones(6)
Q_OFFSET_RAD = np.zeros(6)
TAU_SIGN = np.ones(6)
MAPPING_VALIDATED = True  # confirmed by the hardware owner: identity mapping
MASS_LOWER_SCALE = 0.25
MASS_UPPER_SCALE = 4.0
PRIOR_SIGMA_FRACTION = 0.35
COM_BOUND_METERS = 0.10
COM_PRIOR_SIGMA_METERS = 0.03
PRIOR_WEIGHT = 3.0
EXCLUDED_SAMPLE_IDS = {11, 182}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_samples() -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    with DATA_CSV.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    good = [
        r for r in rows
        if int(r["sample_id"]) not in EXCLUDED_SAMPLE_IDS
        and all(r.get(f"q{i}_rad") and r.get(f"tau{i}_nm") for i in range(1, 7))
    ]
    if len(good) < 30:
        raise RuntimeError(f"need at least 30 complete static samples, found {len(good)}")
    q_sdk = np.array([[float(r[f"q{i}_rad"]) for i in range(1, 7)] for r in good])
    tau_sdk = np.array([[float(r[f"tau{i}_nm"]) for i in range(1, 7)] for r in good])
    sample_id = np.array([int(r["sample_id"]) for r in good])
    return q_sdk[:, JOINT_INDEX] * Q_SIGN + Q_OFFSET_RAD, tau_sdk[:, JOINT_INDEX] * TAU_SIGN, sample_id


def build_model():
    model = pin.buildModelFromUrdf(str(SOURCE_URDF))
    names = [model.names[i] for i in range(1, model.njoints) if model.joints[i].nv]
    accepted = ([f"joint_{i}" for i in range(1, 7)], [f"joint{i}" for i in range(1, 7)])
    if model.nq != 6 or names not in accepted:
        raise RuntimeError(f"unexpected model nq={model.nq}, joints={names}")
    return model


def gravity_regressor(model, q: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Exact gravity regressor for each link's mass and first moments.

    Parameter order per link is [m, h_x, h_y, h_z], where h=m*com.
    Rotational inertia never contributes to gravity torque.
    """
    joint_ids = [i for i in range(1, model.njoints) if model.joints[i].nv]
    nominal_mass = np.array([model.inertias[i].mass for i in joint_ids])
    nominal_com = np.vstack([np.asarray(model.inertias[i].lever) for i in joint_ids])
    columns = []
    for active_id in joint_ids:
        basis = []
        for lever in (np.zeros(3), np.eye(3)[0], np.eye(3)[1], np.eye(3)[2]):
            unit = model.copy()
            for joint_id in joint_ids:
                unit.inertias[joint_id] = pin.Inertia.Zero()
            unit.inertias[active_id] = pin.Inertia(1.0, lever, np.zeros((3, 3)))
            data = unit.createData()
            basis.append(np.vstack([pin.computeGeneralizedGravity(unit, data, qi) for qi in q]))
        mass_column = basis[0]
        columns.extend([mass_column, basis[1] - mass_column, basis[2] - mass_column, basis[3] - mass_column])
    return np.stack(columns, axis=2), nominal_mass, nominal_com


def main() -> None:
    q, tau, sample_ids = load_samples()
    model = build_model()
    G, nominal_mass, nominal_com = gravity_regressor(model, q)
    validation = (sample_ids % 5) == 0
    if validation.sum() < 5:
        validation[::5] = True
    train = ~validation
    mass_sigma = np.maximum(nominal_mass * PRIOR_SIGMA_FRACTION, 0.05)

    def unpack(x: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        masses = x[:6]
        com = x[6:24].reshape(6, 3)
        offsets = x[24:30]
        standard = np.column_stack([masses, masses[:, None] * com]).reshape(-1)
        return masses, com, offsets, standard

    def residual(x: np.ndarray) -> np.ndarray:
        masses, com, offsets, standard = unpack(x)
        predicted = np.einsum("njk,k->nj", G[train], standard) + offsets
        mass_prior = PRIOR_WEIGHT * (masses - nominal_mass) / mass_sigma
        com_prior = PRIOR_WEIGHT * (com - nominal_com).reshape(-1) / COM_PRIOR_SIGMA_METERS
        return np.concatenate([(predicted - tau[train]).ravel(), mass_prior, com_prior])

    x0 = np.concatenate([nominal_mass, nominal_com.reshape(-1), np.zeros(6)])
    lower = np.concatenate([
        nominal_mass * MASS_LOWER_SCALE,
        (nominal_com - COM_BOUND_METERS).reshape(-1),
        np.full(6, -20.0),
    ])
    upper = np.concatenate([
        nominal_mass * MASS_UPPER_SCALE,
        (nominal_com + COM_BOUND_METERS).reshape(-1),
        np.full(6, 20.0),
    ])
    result = least_squares(residual, x0, bounds=(lower, upper), loss="soft_l1")
    masses, com, offsets, standard = unpack(result.x)
    predicted = np.einsum("njk,k->nj", G, standard) + offsets

    def rmse(mask: np.ndarray) -> list[float]:
        return np.sqrt(np.mean((predicted[mask] - tau[mask]) ** 2, axis=0)).tolist()

    tree = ET.parse(SOURCE_URDF)
    root = tree.getroot()
    links = []
    for index, (mass, center) in enumerate(zip(masses, com), 1):
        joint = root.find(f"joint[@name='joint_{index}']")
        if joint is None:
            joint = root.find(f"joint[@name='joint{index}']")
        link_name = joint.find("child").get("link")
        root.find(f"link[@name='{link_name}']/inertial/mass").set("value", f"{mass:.12g}")
        origin = root.find(f"link[@name='{link_name}']/inertial/origin")
        origin.set("xyz", " ".join(f"{value:.12g}" for value in center))
        links.append({
            "joint": f"joint_{index}", "link": link_name,
            "nominal_mass_kg": float(nominal_mass[index - 1]),
            "fitted_mass_kg": float(mass),
            "mass_delta_kg": float(mass - nominal_mass[index - 1]),
            "mass_delta_percent": float(100.0 * (mass / nominal_mass[index - 1] - 1.0)),
            "nominal_com_m": nominal_com[index - 1].tolist(),
            "fitted_com_m": center.tolist(),
            "com_delta_mm": (1000.0 * (center - nominal_com[index - 1])).tolist(),
            "com_shift_norm_mm": float(1000.0 * np.linalg.norm(center - nominal_com[index - 1])),
        })
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    tree.write(OUTPUT_URDF, encoding="utf-8", xml_declaration=True)
    check = pin.buildModelFromUrdf(str(OUTPUT_URDF))
    report = {
        "status": "fit_complete_mapping_unvalidated" if not MAPPING_VALIDATED else "fit_complete",
        "warning": "static data identifies gravity combinations; mass/COM use CAD priors, rotational inertia tensors preserved",
        "mapping_validated": MAPPING_VALIDATED,
        "source_urdf_sha256": sha256(SOURCE_URDF),
        "output_urdf": str(OUTPUT_URDF),
        "output_urdf_sha256": sha256(OUTPUT_URDF),
        "sample_count": int(len(q)), "train_count": int(train.sum()),
        "validation_count": int(validation.sum()),
        "links": links,
        "nominal_total_moving_mass_kg": float(nominal_mass.sum()),
        "fitted_total_moving_mass_kg": float(masses.sum()),
        "total_moving_mass_delta_kg": float(masses.sum() - nominal_mass.sum()),
        "torque_offsets_nm": offsets.tolist(),
        "train_rmse_nm_by_joint": rmse(train),
        "validation_rmse_nm_by_joint": rmse(validation),
        "pinocchio_reload_nq": int(check.nq),
        "optimizer_success": bool(result.success), "optimizer_message": result.message,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
