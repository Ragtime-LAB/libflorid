"""Resume-safe static gravity-identification data collector for Willow.

Hardware motion is disabled by default. Edit only the module-level settings
below; this script intentionally has no command-line arguments.
"""

from __future__ import annotations

import csv
import json
import os
import time
from pathlib import Path

import numpy as np
import pyflorid


# --------------------------- Editable settings ----------------------------
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True
LIMITS_VALIDATED = True
CONFIRMATION_PHRASE = "ENABLE STATIC GRAVITY PILOT"

# J1 is deliberately never commanded to encoder zero. Its measured startup
# angle is copied into every target and held for the complete acquisition.
HOLD_J1_AT_STARTUP = True

KP = np.array([30.0, 60.0, 60.0, 50.0, 40.0, 18.0], dtype=np.float32)
KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
MOVE_DURATION_SECONDS = 0.5
SETTLE_SECONDS = 3.0
SAMPLE_SECONDS = 0.5
SETTLE_MAX_DQ_DEG_S = 5.0
SETTLE_MAX_POSITION_ERROR_DEG = 20.0  # informational only; measured q is fitted

# A diverse 200-pose pilot is large enough for a first gravity-model fit while
# remaining much faster than the full 1170-pose manifest. Set to 0 for all.
MAX_POSES_THIS_RUN = 200

# Physical-arm observations override the old collision model.  Positive J4
# at +45 deg drives the gripper into the arm for some shoulder/elbow poses.
EXCLUDED_SAMPLE_IDS = {11, 182}
EXCLUDED_J4_DEG = {45.0}
J2_GRID_DEG = (30.0, 50.0, 70.0)
J3_GRID_DEG = (45.0, 70.0, 90.0)
J4_GRID_DEG = (-45.0, -15.0, 0.0, 15.0)
J5_GRID_DEG = (-60.0, -20.0, 0.0, 20.0, 60.0)
J6_GRID_DEG = (-60.0, -20.0, 0.0, 20.0, 60.0)
MAX_FIRMWARE_RECOVERY_ATTEMPTS = 3

ROOT = Path(__file__).resolve().parent
MANIFEST_CSV = ROOT / "static_design.csv"
OUTPUT_CSV = ROOT / "runs" / "static_gravity_j2_30_50_70_j3_45_70_90.csv"
REJECTED_CSV = ROOT / "runs" / "rejected_j2_30_50_70_j3_45_70_90.csv"
PROGRESS_JSON = ROOT / "runs" / "progress_j2_30_50_70_j3_45_70_90.json"

# Old Willow limits, intentionally independent of the latest CAD URDF's zero
# placeholder limits. Any measured result within 2 deg is rejected.
OLD_LOWER_DEG = np.array([-180.0, 0.0, 0.0, -74.4845, -89.9544, -89.9544])
OLD_UPPER_DEG = np.array([180.0, 179.9087, 179.9087, 74.4845, 89.9544, 89.9544])
LIMIT_REJECT_MARGIN_DEG = 2.0

COLUMNS = (
    "run_id", "run_index", "sample_id", "host_time_s", "device_time", "seq", "errors",
    *(f"target_q{i}_rad" for i in range(1, 7)),
    *(f"q{i}_rad" for i in range(1, 7)),
    *(f"dq{i}_rad_s" for i in range(1, 7)),
    *(f"tau{i}_nm" for i in range(1, 7)),
    *(f"kp{i}" for i in range(1, 7)),
    *(f"kd{i}" for i in range(1, 7)),
)


def load_manifest() -> list[tuple[int, np.ndarray]]:
    """Build the exact post-J4-zero pilot grid requested by the operator."""
    rows: list[tuple[int, np.ndarray]] = []
    sample_id = 200_000
    for j2 in J2_GRID_DEG:
        for j3 in J3_GRID_DEG:
            for j4 in J4_GRID_DEG:
                for j5 in J5_GRID_DEG:
                    for j6 in J6_GRID_DEG:
                        target_deg = np.array([0.0, j2, j3, j4, j5, j6])
                        rows.append((sample_id, np.deg2rad(target_deg)))
                        sample_id += 1
    return rows


def completed_sample_ids() -> set[int]:
    if not OUTPUT_CSV.exists() or OUTPUT_CSV.stat().st_size == 0:
        return set()
    completed: set[int] = set()
    required = ["sample_id", *(f"q{i}_rad" for i in range(1, 7)), *(f"tau{i}_nm" for i in range(1, 7))]
    with OUTPUT_CSV.open(newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            try:
                if any(row.get(column, "") == "" for column in required):
                    continue
                completed.add(int(row["sample_id"]))
            except (KeyError, TypeError, ValueError):
                continue
    return completed


def rejected_sample_ids() -> set[int]:
    if not REJECTED_CSV.exists() or REJECTED_CSV.stat().st_size == 0:
        return set()
    with REJECTED_CSV.open(newline="", encoding="utf-8") as file:
        return {int(row["sample_id"]) for row in csv.DictReader(file) if row.get("sample_id")}


def append_rejection(run_id: str, sample_id: int, q_deg: np.ndarray, reason: str) -> None:
    REJECTED_CSV.parent.mkdir(parents=True, exist_ok=True)
    columns = ("run_id", "sample_id", "host_time_s", "reason", *(f"q{i}_deg" for i in range(1, 7)))
    needs_header = not REJECTED_CSV.exists() or REJECTED_CSV.stat().st_size == 0
    row: dict[str, float | int | str] = {
        "run_id": run_id, "sample_id": sample_id, "host_time_s": time.time(), "reason": reason,
    }
    row.update({f"q{i}_deg": float(value) for i, value in enumerate(q_deg, 1)})
    with REJECTED_CSV.open("a", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=columns)
        if needs_header:
            writer.writeheader()
        writer.writerow(row)
        file.flush()
        os.fsync(file.fileno())


def near_old_limit(q_rad: np.ndarray) -> tuple[bool, str]:
    q_deg = np.rad2deg(np.asarray(q_rad, dtype=float))
    lower_distance = q_deg - OLD_LOWER_DEG
    upper_distance = OLD_UPPER_DEG - q_deg
    bad = np.flatnonzero((lower_distance <= LIMIT_REJECT_MARGIN_DEG) | (upper_distance <= LIMIT_REJECT_MARGIN_DEG))
    if bad.size == 0:
        return False, ""
    details = [
        f"J{i + 1}={q_deg[i]:.3f}deg range=[{OLD_LOWER_DEG[i]:.3f},{OLD_UPPER_DEG[i]:.3f}]"
        for i in bad
    ]
    return True, "; ".join(details)


def append_row(row: dict[str, float | int | str]) -> None:
    OUTPUT_CSV.parent.mkdir(parents=True, exist_ok=True)
    needs_header = not OUTPUT_CSV.exists() or OUTPUT_CSV.stat().st_size == 0
    with OUTPUT_CSV.open("a", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=COLUMNS)
        if needs_header:
            writer.writeheader()
        writer.writerow(row)
        file.flush()
        os.fsync(file.fileno())


def write_progress(run_id: str, completed: set[int], rejected: set[int], manifest_count: int, last_sample_id: int) -> None:
    PROGRESS_JSON.parent.mkdir(parents=True, exist_ok=True)
    temporary = PROGRESS_JSON.with_suffix(".json.tmp")
    payload = {
        "run_id": run_id,
        "updated_host_time_s": time.time(),
        "manifest_count": manifest_count,
        "completed_count": len(completed),
        "rejected_count": len(rejected),
        "pending_count": manifest_count - len(completed) - len(rejected),
        "last_sample_id": last_sample_id,
        "completed_sample_ids": sorted(completed),
        "rejected_sample_ids": sorted(rejected),
    }
    with temporary.open("w", encoding="utf-8") as file:
        json.dump(payload, file, ensure_ascii=False, indent=2)
        file.flush()
        os.fsync(file.fileno())
    temporary.replace(PROGRESS_JSON)


def make_command(target: np.ndarray) -> object:
    command = pyflorid.JointMIT()
    command.q = np.asarray(target, dtype=np.float32)
    command.dq = np.zeros(6, dtype=np.float32)
    command.tau = np.zeros(6, dtype=np.float32)
    command.kp = KP
    command.kd = KD
    command.firmware_gravity = False
    return command


def read_valid(reader: object, timeout_s: float = 1.0) -> object:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        state = reader.read_once()
        if int(state.seq) != 0:
            return state
        time.sleep(0.0005)
    raise TimeoutError("no valid ArmState within timeout")


def cycle(control: object, target: np.ndarray) -> object:
    state = read_valid(control)
    if int(state.errors) != 0:
        raise RuntimeError(f"firmware errors=0x{int(state.errors):08X}")
    control.write_once(make_command(target))
    return state


def hold_current(control: object) -> tuple[object, np.ndarray]:
    state = read_valid(control)
    target = np.asarray(state.q, dtype=float).copy()
    control.write_once(make_command(target))
    return state, target


def move_and_sample(control: object, target: np.ndarray) -> list[object]:
    _, start = hold_current(control)
    duration = MOVE_DURATION_SECONDS
    begin = time.monotonic()
    while time.monotonic() - begin < duration:
        alpha = min(1.0, (time.monotonic() - begin) / max(duration, 1e-9))
        cycle(control, start + alpha * (target - start))

    settle_deadline = time.monotonic() + SETTLE_SECONDS
    last = None
    while time.monotonic() < settle_deadline:
        last = cycle(control, target)
    if last is None:
        raise RuntimeError("no state received during settling")

    q = np.asarray(last.q, dtype=float)
    dq = np.asarray(last.dq, dtype=float)
    if np.max(np.abs(np.rad2deg(dq))) > SETTLE_MAX_DQ_DEG_S:
        raise RuntimeError("arm did not settle below velocity threshold")
    # A finite MIT steady-state position error is expected.  Static gravity
    # identification uses the measured q below, not the requested target, so
    # target tracking error must not reject an otherwise stationary sample.

    samples: list[object] = []
    sample_deadline = time.monotonic() + SAMPLE_SECONDS
    while time.monotonic() < sample_deadline:
        samples.append(cycle(control, target))
    return samples


def recover_mit_control(arm: object, error: Exception) -> object:
    """Disable first, then rebuild the MIT channel after a firmware fault."""
    print(f"Firmware fault: {error}; disabling all axes before recovery")
    arm.disable()
    time.sleep(1.0)
    arm.enable()
    control = arm.start_joint_mit_control()
    if control is None:
        raise RuntimeError("MIT recovery returned None")
    state, target = hold_current(control)
    print("MIT control restarted at q_deg:", np.round(np.rad2deg(target), 3))
    return control


def median_row(run_id: str, run_index: int, sample_id: int, target: np.ndarray,
               samples: list[object]) -> dict[str, float | int | str]:
    q = np.median(np.stack([np.asarray(s.q, dtype=float) for s in samples]), axis=0)
    dq = np.median(np.stack([np.asarray(s.dq, dtype=float) for s in samples]), axis=0)
    tau = np.median(np.stack([np.asarray(s.tau, dtype=float) for s in samples]), axis=0)
    row: dict[str, float | int | str] = {
        "run_id": run_id,
        "run_index": run_index,
        "sample_id": sample_id,
        "host_time_s": time.time(),
        "device_time": float(np.median([float(s.time) for s in samples])),
        "seq": int(samples[-1].seq),
        "errors": int(samples[-1].errors),
    }
    for prefix, values in (("target_q", target), ("q", q), ("dq", dq), ("tau", tau), ("kp", KP), ("kd", KD)):
        suffix = {"target_q": "_rad", "q": "_rad", "dq": "_rad_s", "tau": "_nm", "kp": "", "kd": ""}[prefix]
        for index, value in enumerate(values, 1):
            row[f"{prefix}{index}{suffix}"] = float(value)
    return row


def diverse_subset(rows: list[tuple[int, np.ndarray]], count: int) -> list[tuple[int, np.ndarray]]:
    """Greedy farthest-point subset over normalized J2-J6 coordinates."""
    if count <= 0 or count >= len(rows):
        return rows.copy()
    points = np.stack([target[1:] for _, target in rows])
    span = np.ptp(points, axis=0)
    span[span == 0.0] = 1.0
    normalized = (points - np.min(points, axis=0)) / span
    selected = [int(np.argmin(np.linalg.norm(normalized - 0.5, axis=1)))]
    minimum_distance = np.linalg.norm(normalized - normalized[selected[0]], axis=1)
    while len(selected) < count:
        index = int(np.argmax(minimum_distance))
        selected.append(index)
        minimum_distance = np.minimum(
            minimum_distance,
            np.linalg.norm(normalized - normalized[index], axis=1),
        )
    return [rows[index] for index in selected]


def nearest_remaining(rows: list[tuple[int, np.ndarray]], start: np.ndarray) -> list[tuple[int, np.ndarray]]:
    remaining = rows.copy()
    ordered: list[tuple[int, np.ndarray]] = []
    current = start.copy()
    while remaining:
        index = min(range(len(remaining)), key=lambda i: float(np.max(np.abs(remaining[i][1][1:] - current[1:]))))
        item = remaining.pop(index)
        ordered.append(item)
        current = item[1]
    return ordered


def main() -> None:
    run_id = time.strftime("%Y%m%dT%H%M%S")
    manifest = load_manifest()
    # Completed rows that were later blacklisted after a physical collision do
    # not count toward the safe manifest or its progress totals.
    manifest_ids = {sample_id for sample_id, _ in manifest}
    completed = completed_sample_ids() & manifest_ids
    rejected = rejected_sample_ids() & manifest_ids
    pending = [
        (sample_id, target) for sample_id, target in manifest
        if sample_id not in completed and sample_id not in rejected
    ]
    print(f"manifest={len(manifest)}, completed={len(completed)}, rejected={len(rejected)}, pending={len(pending)}")
    print("J1 policy: hold measured startup angle; never command encoder 0")
    print(f"output={OUTPUT_CSV}")
    planned_count = min(len(pending), MAX_POSES_THIS_RUN) if MAX_POSES_THIS_RUN > 0 else len(pending)
    print(f"diverse poses planned this run={planned_count}")
    print(f"run_id={run_id}")

    if not ENABLE_HARDWARE:
        print("DISABLED PREVIEW ONLY: set ENABLE_HARDWARE=True after review")
        return
    if not LIMITS_VALIDATED:
        raise RuntimeError("LIMITS_VALIDATED is false; refusing hardware motion")

    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")

    passive = read_valid(arm, timeout_s=2.0)
    initial_q = np.asarray(passive.q, dtype=float).copy()
    print("Measured startup q_deg:", np.round(np.rad2deg(initial_q), 3))
    phrase = input(f'Type exactly "{CONFIRMATION_PHRASE}" to enable all axes: ')
    if phrase != CONFIRMATION_PHRASE:
        raise RuntimeError("confirmation phrase mismatch; hardware remains disabled")

    control = None
    try:
        arm.enable()
        control = arm.start_joint_mit_control()
        if control is None:
            raise RuntimeError("start_joint_mit_control returned None")
        _, held = hold_current(control)
        selected = diverse_subset(pending, MAX_POSES_THIS_RUN)
        ordered = nearest_remaining(selected, held)

        for run_index, (sample_id, target_template) in enumerate(ordered, 1):
            target = target_template.copy()
            if HOLD_J1_AT_STARTUP:
                target[0] = initial_q[0]
            print(f"[{run_index}/{len(ordered)}] sample_id={sample_id}, target_deg={np.round(np.rad2deg(target), 2)}")
            recovery_attempt = 0
            while True:
                try:
                    samples = move_and_sample(control, target)
                    break
                except RuntimeError as error:
                    if "firmware errors=" not in str(error):
                        raise
                    recovery_attempt += 1
                    if recovery_attempt > MAX_FIRMWARE_RECOVERY_ATTEMPTS:
                        raise RuntimeError(
                            f"firmware recovery failed {MAX_FIRMWARE_RECOVERY_ATTEMPTS} times"
                        ) from error
                    control = recover_mit_control(arm, error)
            row = median_row(run_id, run_index, sample_id, target, samples)
            q_measured = np.array([float(row[f"q{i}_rad"]) for i in range(1, 7)])
            tau_measured = np.array([float(row[f"tau{i}_nm"]) for i in range(1, 7)])
            q_deg = np.rad2deg(q_measured)
            print("  measured q_deg =", np.array2string(q_deg, precision=3, separator=", "))
            print("  measured tau_Nm=", np.array2string(tau_measured, precision=4, separator=", "))
            is_near_limit, limit_reason = near_old_limit(q_measured)
            if is_near_limit:
                append_rejection(run_id, sample_id, q_deg, limit_reason)
                rejected.add(sample_id)
                write_progress(run_id, completed, rejected, len(manifest), sample_id)
                print(f"  REJECTED near old limit (<= {LIMIT_REJECT_MARGIN_DEG} deg): {limit_reason}")
                continue
            append_row(row)
            completed.add(sample_id)
            write_progress(run_id, completed, rejected, len(manifest), sample_id)
            print(
                f"  saved+fsynced; completed={len(completed)}, rejected={len(rejected)}, "
                f"pending={len(manifest) - len(completed) - len(rejected)}"
            )
    finally:
        try:
            arm.disable()
            print("All axes disabled.")
        except Exception as error:
            print(f"CRITICAL: arm.disable() failed: {error}")


if __name__ == "__main__":
    main()
