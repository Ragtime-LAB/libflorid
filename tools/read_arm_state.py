"""Passive Willow state monitor.

This script opens the libflorid transport and only calls ``Arm.read_once()``.
It never enters a control mode and never sends a motion command.
"""

from __future__ import annotations

import math
import threading
import time
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np
import pyflorid


DEVICE_URI = "usb:///dev/ttyACM0"
RUN_SECONDS = 0.0  # 0 = run until the MuJoCo window is closed or Ctrl+C
PRINT_PERIOD_SECONDS = 0.5
EMPTY_POLL_SLEEP_SECONDS = 0.0005
MUJOCO_RENDER_HZ = 10.0
ENABLE_MUJOCO_TWIN = True
USE_OLD_WILLOW_MESH_MODEL = False  # latest kinematics + old Willow display meshes

ROOT = Path(__file__).resolve().parent
SOURCE_DESCRIPTION = (
    ROOT / "static_gravity_calibration" / "model" / "willow-v0.2"
)
SOURCE_URDF = SOURCE_DESCRIPTION / "urdf" / "willow-v0.2.urdf"
TWIN_DIR = SOURCE_DESCRIPTION / "mujoco_twin"
TWIN_URDF = TWIN_DIR / "willow-v0.2-new-geometry-old-mesh.mujoco.urdf"
TWIN_MESH_DIR = TWIN_DIR / "old_willow_meshes"
MAX_TWIN_MESH_FACES = 80_000
OLD_TWIN_XML = ROOT / "willow_digital_twin" / "willow.xml"
OLD_WILLOW_MESH_DIR = ROOT / "willow_digital_twin" / "meshes" / "willow"

# The latest CAD URDF contains zero placeholder limits.  These are the old,
# already-used Willow ranges and affect only the viewer model.
VIEWER_LIMITS_RAD = (
    (-3.14, 3.14), (0.0, 3.14), (0.0, 3.14),
    (-1.3, 1.3), (-1.57, 1.57), (-1.57, 1.57),
)

# Confirmed SDK feedback order/sign/offset mapping for this arm.
SDK_INDEX = np.arange(6)
Q_SIGN = np.ones(6)
Q_OFFSET_RAD = np.zeros(6)


def _format_vector(values: np.ndarray, precision: int = 3) -> str:
    return np.array2string(
        values,
        precision=precision,
        suppress_small=True,
        separator=", ",
        floatmode="fixed",
    )


def _build_mujoco_urdf() -> Path:
    """Create a cached viewer-only URDF and decimated STL copies."""
    if not SOURCE_URDF.is_file():
        raise FileNotFoundError(f"latest Willow URDF not found: {SOURCE_URDF}")
    source_meshes = sorted(OLD_WILLOW_MESH_DIR.glob("*.STL"))
    if not source_meshes:
        raise FileNotFoundError("latest Willow mesh directory is empty")

    newest_source = max([SOURCE_URDF.stat().st_mtime, *(p.stat().st_mtime for p in source_meshes)])
    if TWIN_URDF.is_file() and TWIN_URDF.stat().st_mtime >= newest_source:
        return TWIN_URDF

    import trimesh

    print("Building MuJoCo viewer meshes (one-time cache) ...")
    TWIN_MESH_DIR.mkdir(parents=True, exist_ok=True)
    for source in source_meshes:
        mesh = trimesh.load_mesh(source, process=False)
        if len(mesh.faces) > MAX_TWIN_MESH_FACES:
            mesh = mesh.simplify_quadric_decimation(face_count=MAX_TWIN_MESH_FACES)
        destination = TWIN_MESH_DIR / source.name.lower()
        mesh.export(destination, file_type="stl")
        print(f"  {source.name}: {len(mesh.faces)} faces")

    tree = ET.parse(SOURCE_URDF)
    root = tree.getroot()
    # Keep one mesh geom per link. Collision geometry is displayed by MuJoCo;
    # duplicate visual meshes would only double rendering cost.
    for link in root.findall("link"):
        visual = link.find("visual")
        if visual is not None:
            link.remove(visual)
    for mesh_element in root.findall(".//mesh"):
        filename = Path(mesh_element.get("filename", "")).name.lower()
        # latest names link_1.STL; the old validated display meshes use link1.STL
        filename = filename.replace("link_", "link")
        mesh_element.set("filename", str(TWIN_MESH_DIR / filename))
    for index, (lower, upper) in enumerate(VIEWER_LIMITS_RAD, 1):
        limit = root.find(f"joint[@name='joint_{index}']/limit")
        limit.set("lower", str(lower))
        limit.set("upper", str(upper))
        limit.set("effort", "100")
        limit.set("velocity", "10")
    TWIN_DIR.mkdir(parents=True, exist_ok=True)
    tree.write(TWIN_URDF, encoding="utf-8", xml_declaration=True)
    return TWIN_URDF


def _open_twin():
    if not ENABLE_MUJOCO_TWIN:
        return None, None, None, None
    import mujoco
    import mujoco.viewer

    model_path = OLD_TWIN_XML if USE_OLD_WILLOW_MESH_MODEL else _build_mujoco_urdf()
    if not model_path.is_file():
        raise FileNotFoundError(f"MuJoCo twin model is missing: {model_path}")
    model = mujoco.MjModel.from_xml_path(str(model_path))
    data = mujoco.MjData(model)
    joint_qpos = []
    for index in range(1, 7):
        joint_name = f"joint{index}" if USE_OLD_WILLOW_MESH_MODEL else f"joint_{index}"
        joint_id = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, joint_name)
        if joint_id < 0:
            raise RuntimeError(f"MuJoCo {joint_name} not found")
        joint_qpos.append(int(model.jnt_qposadr[joint_id]))
    viewer = mujoco.viewer.launch_passive(model, data)
    viewer.cam.lookat[:] = [0.0, 0.0, 0.28]
    viewer.cam.distance = 1.15
    viewer.cam.azimuth = 135.0
    viewer.cam.elevation = -20.0
    print(f"MuJoCo twin opened: {model_path}")
    return model, data, viewer, np.asarray(joint_qpos, dtype=int)


def main() -> None:
    print("PASSIVE MODE: no control mode, no command writes, TX command rate = 0 Hz")
    print(f"Opening {DEVICE_URI} ...")
    model, data, viewer, joint_qpos = _open_twin()
    arm = pyflorid.Arm.create(DEVICE_URI)
    if arm is None:
        raise RuntimeError(f"Arm.create() failed for {DEVICE_URI}")

    firmware_period_us = int(arm.firmware_period_us())
    firmware_hz = 1_000_000.0 / firmware_period_us if firmware_period_us > 0 else math.nan
    print(f"connected={arm.is_connected()}, firmware_period_us={firmware_period_us}, nominal={firmware_hz:.1f} Hz")

    started = time.perf_counter()
    deadline = started + RUN_SECONDS if RUN_SECONDS > 0 else math.inf
    stop = threading.Event()
    latest_lock = threading.Lock()
    latest: dict[str, object] = {"state": None, "error": None}
    stats = {"valid": 0, "empty": 0, "repeated": 0}

    def reader_loop() -> None:
        """Read at transport speed and retain only the newest complete frame."""
        last_seq: int | None = None
        try:
            while not stop.is_set():
                state = arm.read_once()
                if int(state.seq) == 0:
                    with latest_lock:
                        stats["empty"] += 1
                    time.sleep(EMPTY_POLL_SLEEP_SECONDS)
                    continue
                seq = int(state.seq)
                snapshot = {
                    "seq": seq,
                    "errors": int(state.errors),
                    "q": np.asarray(state.q, dtype=np.float64).copy(),
                    "dq": np.asarray(state.dq, dtype=np.float64).copy(),
                    "tau": np.asarray(state.tau, dtype=np.float64).copy(),
                    "received_at": time.perf_counter(),
                }
                with latest_lock:
                    stats["valid"] += 1
                    if last_seq == seq:
                        stats["repeated"] += 1
                    latest["state"] = snapshot  # overwrite; never queue stale frames
                last_seq = seq
        except BaseException as error:
            with latest_lock:
                latest["error"] = error
            stop.set()

    reader = threading.Thread(target=reader_loop, name="willow-state-reader", daemon=True)
    reader.start()
    report_started = started
    report_valid_start = 0
    next_render = started
    last_rendered_seq: int | None = None

    try:
        while time.perf_counter() < deadline and (viewer is None or viewer.is_running()):
            now = time.perf_counter()
            with latest_lock:
                state = latest["state"]
                reader_error = latest["error"]
                valid_now = stats["valid"]
            if reader_error is not None:
                raise RuntimeError("state reader thread failed") from reader_error
            if state is None:
                time.sleep(0.001)
                continue

            # Rendering is deliberately fixed at 10 Hz. The state reader keeps
            # running independently and this always uses its newest frame.
            if viewer is not None and now >= next_render:
                q_rad = state["q"]
                q_model = q_rad[SDK_INDEX] * Q_SIGN + Q_OFFSET_RAD
                import mujoco
                with viewer.lock():
                    data.qpos[joint_qpos] = q_model
                    data.qvel[:] = 0.0
                    mujoco.mj_forward(model, data)
                viewer.sync()
                last_rendered_seq = int(state["seq"])
                next_render = now + 1.0 / MUJOCO_RENDER_HZ

            if now - report_started < PRINT_PERIOD_SECONDS:
                time.sleep(0.001)
                continue

            interval = now - report_started
            q_rad = state["q"]
            q_deg = np.rad2deg(q_rad)
            dq = state["dq"]
            tau = state["tau"]
            measured_hz = (valid_now - report_valid_start) / interval
            age_ms = 1000.0 * (now - float(state["received_at"]))

            print(
                f"seq={int(state['seq']):>10d}  rx={measured_hz:7.1f} Hz  "
                f"render={MUJOCO_RENDER_HZ:.1f} Hz  age={age_ms:5.1f} ms  "
                f"errors=0x{int(state['errors']):08X}"
            )
            print(f"  q_deg = {_format_vector(q_deg, 2)}")
            print(f"  q_rad = {_format_vector(q_rad, 4)}")
            print(f"  dq    = {_format_vector(dq, 4)}")
            print(f"  tau    = {_format_vector(tau, 3)}")

            report_started = now
            report_valid_start = valid_now
    except KeyboardInterrupt:
        print("Stopped by user.")
    finally:
        stop.set()
        reader.join(timeout=1.0)
        if viewer is not None:
            viewer.close()

    elapsed = time.perf_counter() - started
    with latest_lock:
        valid_total = stats["valid"]
        empty_total = stats["empty"]
        repeated_seq = stats["repeated"]
    print("--- summary ---")
    print(f"elapsed={elapsed:.3f} s")
    print(f"valid_frames={valid_total}, measured_rx={valid_total / elapsed:.2f} Hz")
    print(f"empty_polls={empty_total}, repeated_seq={repeated_seq}")
    print(f"last_rendered_seq={last_rendered_seq}, mujoco_render_target={MUJOCO_RENDER_HZ:.1f} Hz")
    print("motion_command_frames_sent=0, command_tx_rate=0 Hz")


if __name__ == "__main__":
    main()
