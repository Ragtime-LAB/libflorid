#!/usr/bin/env python3
"""Hardware-free smoke test for tools/gravity_tuner.

Run from the repo root:
    python3 tools/gravity_tuner/tests/smoke_test.py

Requires: pyflorid (pre-built), pinocchio, numpy.
"""

from __future__ import annotations

import os
import sys
import tempfile
import threading
import time
from types import SimpleNamespace

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from gravity_tuner.controller import GravityCompController, describe_errors
from gravity_tuner.model import GravityModel

_URDF = """<?xml version="1.0"?>
<robot name="smoke_arm">
  <link name="base"/>
  <joint name="base2l1" type="fixed"><parent link="base"/><child link="l0"/></joint>
  <link name="l0"/>
  {links}
</robot>
"""

_LINK = """
  <joint name="j{id}" type="revolute"><parent link="l{prev}"/><child link="l{id}"/>
    <origin xyz="0 0 {z}"/><axis xyz="0 1 0"/>
    <limit lower="-3.14" upper="3.14" effort="80.0" velocity="10.0"/></joint>
  <link name="l{id}"><inertial><origin xyz="{com} 0 0"/><mass value="{mass}"/>
    <inertia ixx="{i}" ixy="0" ixz="0" iyy="{i}" iyz="0" izz="{i}"/></inertial></link>
"""


def build_urdf() -> str:
    s_links = []
    s_prev = "0"
    for s_id in range(1, 7):
        s_links.append(
            _LINK.format(id=s_id, prev=s_prev, z=0.25, com=0.1 * s_id, mass=float(s_id), i=0.05 * s_id)
        )
        s_prev = str(s_id)
    return _URDF.format(links="\n".join(s_links))


def make_state(seq: int, errors: int = 0) -> SimpleNamespace:
    s_q = np.zeros(6, dtype=np.float32)
    return SimpleNamespace(
        seq=seq,
        q=s_q,
        base_gravity=np.array([0.0, 0.0, -9.81], dtype=np.float32),
        tau=np.ones(6, dtype=np.float32),
        errors=errors,
    )


class FakeAC:
    """Mimics pyflorid.ActiveJointMIT: read_once/write_once."""

    def __init__(self, make_state, start_seq: int = 1) -> None:
        self._make = make_state
        self._seq = start_seq - 1
        self.sent: list = []

    def read_once(self):
        self._seq += 1
        return self._make(self._seq)

    def write_once(self, cmd):
        self.sent.append(cmd)


def test_model(model: GravityModel, urdf_path: str) -> None:
    model.load(urdf_path)
    assert model.dof == 6, model.dof

    # invalid paths must raise clean Python errors (no native abort)
    for s_bad in ("", "   ", "/nonexistent/file.urdf"):
        try:
            GravityModel().load(s_bad)
        except Exception as s_e:  # noqa: BLE001
            assert isinstance(s_e, FileNotFoundError)
        else:
            raise AssertionError(f"expected FileNotFoundError for {s_bad!r}")
    s_empty = os.path.join(os.path.dirname(urdf_path), "empty.urdf")
    with open(s_empty, "w", encoding="utf-8") as s_f:
        s_f.write("  \n")
    try:
        GravityModel().load(s_empty)
    except Exception as s_e:  # noqa: BLE001
        assert isinstance(s_e, ValueError), type(s_e)
    else:
        raise AssertionError("expected ValueError for empty URDF")

    s_names = model.joint_names()
    assert len(s_names) == 6 and s_names[0] == "j1" and s_names[-1] == "j6", s_names

    s_q = np.zeros(6, dtype=np.float64)
    s_g = np.array([0.0, 0.0, -9.81])
    s_full = model.gravity_torque(s_q, s_g, 1.0)
    s_half = model.gravity_torque(s_q, s_g, 0.5)
    assert np.allclose(s_half, 0.5 * s_full), (s_half, 0.5 * s_full)
    assert np.max(np.abs(s_full)) > 0.1, s_full  # hanging horizontal links

    # gravity is linear in every link mass -> doubling all doubles torque
    s_orig = [j["mass"] for j in model.joints]
    for s_id in range(1, 7):
        s_target = 2.0 * s_orig[s_id - 1]
        model.set_mass(s_id, s_target)
        assert model.mass(s_id) == s_target
    s_double = model.gravity_torque(s_q, s_g, 1.0)
    assert np.allclose(s_double, 2.0 * s_full), (s_double, 2.0 * s_full)
    for s_id in range(1, 7):
        model.set_mass(s_id, s_orig[s_id - 1])

    # default base gravity == passing [0,0,-9.81] explicitly
    assert np.allclose(model.gravity_torque(s_q, k_gravity=1.0), s_full)

    # single-link mass change affects that joint and all proximal joints
    s_before = model.gravity_torque(s_q, s_g, 1.0)
    model.set_mass(3, 2.0 * model.mass(3))
    s_after = model.gravity_torque(s_q, s_g, 1.0)
    model.set_mass(3, model.mass(3) / 2.0)
    assert np.any(s_after[:3] != s_before[:3]) and np.all(s_after[3:] == s_before[3:])
    print("model  OK: dof=6, linear mass scaling, per-joint mass changes")


def test_controller(model: GravityModel) -> None:
    s_ac = FakeAC(lambda seq: make_state(seq))
    ctrl = GravityCompController(model, k_gravity=0.5)
    ctrl.start_with(s_ac)
    s_deadline = time.monotonic() + 2.0
    while len(s_ac.sent) < 5 and time.monotonic() < s_deadline:
        time.sleep(0.02)
    ctrl.stop()

    s_ff = [c for c in s_ac.sent if np.all(c.kp == 0) and np.all(c.kd == 0) and not c.motion_finished]
    assert len(s_ff) >= 3, len(s_ff)

    # feedforward matches model gravity * kGravity
    s_q = np.zeros(6, dtype=np.float64)
    s_g = np.array([0.0, 0.0, -9.81])
    s_exp = model.gravity_torque(s_q, s_g, 0.5)
    for s_cmd in s_ff[-3:]:
        assert np.all(s_cmd.q == 0) and np.all(s_cmd.dq == 0)
        assert np.allclose(s_cmd.tau, s_exp, atol=1e-4), (s_cmd.tau, s_exp)
        assert s_cmd.firmware_gravity is False

    # stop() applies a brief hold -> later commands carry PD gains
    s_holds = [c for c in s_ac.sent if np.any(c.kp != 0)]
    assert s_holds, "expected a graceful hold after stop"
    s_frame = ctrl.last_frame
    assert s_frame is not None and s_frame[2].shape == (6,)
    print("controller OK: MIT frame contents, kGravity scaling, graceful hold")


def test_empty_skip(model: GravityModel) -> None:
    s_ac = FakeAC(lambda seq: make_state(0))  # always seq==0 (empty dequeue)
    ctrl = GravityCompController(model)
    ctrl.start_with(s_ac)
    time.sleep(0.15)
    ctrl.stop()
    assert not s_ac.sent, "no frames must be sent for empty (seq==0) states"
    print("empty  OK: seq==0 states skipped (no zero-frame spam)")


def test_reverse_and_scale(model: GravityModel) -> None:
    ctrl = GravityCompController(model, k_gravity=0.5)
    s_q = np.zeros(6, dtype=np.float32)
    s_g = np.array([0.0, 0.0, -9.81], dtype=np.float32)
    s_base = model.gravity_torque(s_q, s_g, 0.5)

    s_fwd = ctrl._feedforward(s_q, s_g)
    assert np.allclose(s_fwd, s_base)

    ctrl.set_reverse(True)
    assert ctrl.reverse() is True
    assert np.allclose(ctrl._feedforward(s_q, s_g), -s_base)

    ctrl.set_joint_scale(2, 3.0)
    s_scaled = ctrl._feedforward(s_q, s_g)
    assert np.allclose(s_scaled[1], -3.0 * s_base[1]), (s_scaled[1], -3.0 * s_base[1])
    assert np.allclose(s_scaled[0], -s_base[0])
    assert np.allclose(s_scaled[2:], -s_base[2:])
    assert ctrl.joint_scale(2) == 3.0

    ctrl.set_reverse(False)
    s_expected = s_base.copy()
    s_expected[1] *= 3.0
    assert np.allclose(ctrl._feedforward(s_q, s_g), s_expected)
    print("scale  OK: reverse toggle + per-joint scale applied")


def test_save(model: GravityModel, urdf_path: str) -> None:
    import xml.etree.ElementTree as ET

    model.set_mass(2, 7.5)
    model.set_scale(3, -2.0)  # reversed direction for joint 3
    with tempfile.TemporaryDirectory() as s_dir:
        s_out = os.path.join(s_dir, "saved.urdf")
        model.save(s_out)

        s_text = open(s_out, encoding="utf-8").read()
        assert "gravity_scale" not in s_text, "must not add extra URDF attributes"
        s_root = ET.fromstring(s_text)
        s_links = {l.get("name"): l for l in s_root.findall("link")}
        s_joints = {j.get("name"): j for j in s_root.findall("joint")}
        assert float(s_links["l2"].find("inertial/mass").get("value")) == 7.5
        assert s_joints["j3"].find("axis").get("xyz").split()[0].startswith("-")

        # reload: mass persisted, axis flipped -> gravity sign reversed for j3
        s_model2 = GravityModel().load(s_out)
        assert s_model2.mass(2) == 7.5
        s_q = np.zeros(6, dtype=np.float64)
        s_g = np.array([0.0, 0.0, -9.81])
        s_orig = model.gravity_torque(s_q, s_g, 1.0)
        s_new = s_model2.gravity_torque(s_q, s_g, 1.0)
        assert np.allclose(s_new[2], -s_orig[2]), (s_new[2], s_orig[2])
        assert s_model2.scale(3) == 1.0  # scale not persisted
    print("save   OK: mass written, axis flipped for negative scale, no extra attrs")


def test_fault_handling(model: GravityModel) -> None:
    s_make = lambda seq: make_state(seq, errors=0)  # noqa: E731
    s_ac = FakeAC(s_make)
    ctrl = GravityCompController(model)
    ctrl.start_with(s_ac)

    # flip the fake to inject an E-stop on the next read
    s_ac._make = lambda seq: make_state(seq, errors=1 << 9)
    s_deadline = time.monotonic() + 3.0
    while ctrl.is_running and time.monotonic() < s_deadline:
        time.sleep(0.02)
    assert not ctrl.is_running
    assert ctrl.fault and "E-stop" in (ctrl.fault or ""), ctrl.fault
    assert describe_errors(0) == "unknown error bits 0x0"
    print("fault  OK: E-stop bitset stops the loop and surfaces the fault")


def main() -> None:
    assert __import__("pyflorid").Arm is not None, "pyflorid not importable"
    s_model = GravityModel()
    with tempfile.TemporaryDirectory() as s_dir:
        s_path = os.path.join(s_dir, "smoke_arm.urdf")
        with open(s_path, "w", encoding="utf-8") as s_f:
            s_f.write(build_urdf())
        test_model(s_model, s_path)
    test_controller(s_model)
    test_empty_skip(s_model)
    test_reverse_and_scale(s_model)
    with tempfile.TemporaryDirectory() as s_dir:
        s_path = os.path.join(s_dir, "smoke_arm.urdf")
        with open(s_path, "w", encoding="utf-8") as s_f:
            s_f.write(build_urdf())
        test_save(s_model, s_path)
    test_fault_handling(s_model)
    print("\ngravity_tuner smoke test: ALL PASSED")


if __name__ == "__main__":
    main()