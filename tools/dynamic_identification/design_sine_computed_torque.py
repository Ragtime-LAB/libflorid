"""Offline two-pose sine trajectory and preliminary computed-torque curves."""

from __future__ import annotations

import csv
import json
from pathlib import Path

import numpy as np
import pinocchio as pin


ROOT = Path(__file__).resolve().parent
URDF = (
    ROOT.parent
    / "static_gravity_calibration/model/identified/"
    / "Ragtime_Willow.static-mass-com-calibrated.urdf"
)
FIT = ROOT / "preliminary_dynamic_fit.json"
OUTPUT_NPZ = ROOT / "computed_torque_two_pose_sine.npz"
OUTPUT_CSV = ROOT / "computed_torque_two_pose_sine.csv"
OUTPUT_REPORT = ROOT / "computed_torque_two_pose_sine_report.json"

POSE_A_DEG = np.array([85.76, 124.84, 104.42, -37.06, -15.22, 128.46])
POSE_B_REQUESTED_DEG = np.array([17.89, 6.02, 0.36, 19.92, 24.25, 154.58])
POSE_B_USED_DEG = POSE_B_REQUESTED_DEG.copy()
POSE_B_USED_DEG[2] = 30.0  # installation constraint: keep gripper above table
RATE_HZ = 500.0
PERIOD_S = 8.0  # A -> B in 4 s, B -> A in 4 s
CYCLES = 3


def trajectory():
    t = np.arange(0.0, PERIOD_S * CYCLES, 1.0 / RATE_HZ)
    omega = 2.0 * np.pi / PERIOD_S
    blend = 0.5 * (1.0 - np.cos(omega * t))
    blend_d = 0.5 * omega * np.sin(omega * t)
    blend_dd = 0.5 * omega**2 * np.cos(omega * t)
    qa = np.deg2rad(POSE_A_DEG)
    delta = np.deg2rad(POSE_B_USED_DEG - POSE_A_DEG)
    q = qa + blend[:, None] * delta
    dq = blend_d[:, None] * delta
    ddq = blend_dd[:, None] * delta
    return t, q, dq, ddq


def parameter_names(model):
    fields = ("m", "mx", "my", "mz", "Ixx", "Ixy", "Iyy", "Ixz", "Iyz", "Izz")
    result = [f"{model.names[joint]}::{field}" for joint in range(1, model.njoints) for field in fields]
    joints = list(model.names[1:])
    result += [f"{name}::fv" for name in joints]
    result += [f"{name}::fc" for name in joints]
    result += [f"{name}::Ia" for name in joints]
    result += [f"{name}::offset" for name in joints]
    return result


def fitted_curve(model, q, dq, ddq, fit):
    names = parameter_names(model)
    parameters = fit["parameters"]
    missing = [name for name in names if name not in parameters]
    if missing:
        raise RuntimeError(f"preliminary fit is missing parameters: {missing[:3]}")
    phi = np.asarray([parameters[name] for name in names], dtype=float)
    data = model.createData()
    result = np.empty_like(q)
    for index, (qi, dqi, ddqi) in enumerate(zip(q, dq, ddq)):
        rigid = np.asarray(pin.computeJointTorqueRegressor(model, data, qi, dqi, ddqi))
        regressor = np.column_stack(
            (rigid, np.diag(dqi), np.diag(np.sign(dqi)), np.diag(ddqi), np.eye(6))
        )
        result[index] = regressor @ phi
    return result


def main():
    if not URDF.is_file() or not FIT.is_file():
        raise FileNotFoundError("identified URDF or preliminary fit is missing")
    model = pin.buildModelFromUrdf(str(URDF))
    data = model.createData()
    t, q, dq, ddq = trajectory()
    tau_urdf = np.asarray([pin.rnea(model, data, qi, dqi, ddqi) for qi, dqi, ddqi in zip(q, dq, ddq)])
    fit = json.loads(FIT.read_text(encoding="utf-8"))
    tau_preliminary = fitted_curve(model, q, dq, ddq, fit)
    np.savez_compressed(
        OUTPUT_NPZ,
        t=t,
        q=q,
        dq=dq,
        ddq=ddq,
        tau_urdf=tau_urdf,
        tau_preliminary=tau_preliminary,
    )
    names = (
        ["time_s"]
        + [f"q{i}_rad" for i in range(1, 7)]
        + [f"dq{i}_rad_s" for i in range(1, 7)]
        + [f"ddq{i}_rad_s2" for i in range(1, 7)]
        + [f"tau_urdf{i}_nm" for i in range(1, 7)]
        + [f"tau_preliminary{i}_nm" for i in range(1, 7)]
    )
    values = np.column_stack((t, q, dq, ddq, tau_urdf, tau_preliminary))
    with OUTPUT_CSV.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(names)
        writer.writerows(values)
    report = {
        "offline_only": True,
        "pose_a_deg": POSE_A_DEG.tolist(),
        "pose_b_requested_deg": POSE_B_REQUESTED_DEG.tolist(),
        "pose_b_used_deg": POSE_B_USED_DEG.tolist(),
        "period_s": PERIOD_S,
        "cycles": CYCLES,
        "duration_s": float(t[-1]),
        "samples": len(t),
        "peak_dq_deg_s": np.rad2deg(np.max(np.abs(dq), axis=0)).tolist(),
        "peak_ddq_deg_s2": np.rad2deg(np.max(np.abs(ddq), axis=0)).tolist(),
        "tau_urdf_min_nm": tau_urdf.min(axis=0).tolist(),
        "tau_urdf_max_nm": tau_urdf.max(axis=0).tolist(),
        "tau_preliminary_min_nm": tau_preliminary.min(axis=0).tolist(),
        "tau_preliminary_max_nm": tau_preliminary.max(axis=0).tolist(),
        "fit_rank": fit["rank"],
        "fit_condition_reported": fit["condition"],
        "warning": "preliminary unconstrained fit is not a deployable physical mass matrix",
    }
    OUTPUT_REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    print("curve:", OUTPUT_CSV)


if __name__ == "__main__":
    main()
