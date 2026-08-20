"""Runtime URDF dynamics for gravity-compensation tuning.

The pyflorid ``Model`` binding bakes link masses into CasADi-generated C code
at codegen time, so it cannot represent an arbitrary URDF or runtime mass
changes. This module builds the model with Pinocchio (the same engine used by
``scripts/urdf2traits.py``) and exposes link masses as runtime-adjustable
parameters.
"""

from __future__ import annotations

import os
import threading
import xml.etree.ElementTree as ET

import numpy as np

_DEFAULT_GRAVITY = np.array([0.0, 0.0, -9.81])


class GravityModel:
    """URDF-backed rigid-body model with runtime-adjustable link masses.

    Joint numbering follows the arm convention: ``joint k`` (1-based) is the
    k-th movable joint in the URDF. It is associated with the child link of
    that joint; changing its mass changes the gravity torque of that link and
    of every more proximal joint.
    """

    def __init__(self) -> None:
        self._model = None
        self._data = None
        self._joints: list[dict] = []
        self._index: dict[int, int] = {}
        self._scale: list[float] = []
        self._xml_root = None
        self._joint_els: dict[int, object] = {}
        self._link_els: dict[int, object] = {}
        self._path: str | None = None
        self._lock = threading.Lock()

    # ---- loading ---------------------------------------------------------

    def load(self, urdf_path: str) -> "GravityModel":
        import pinocchio as pin

        s_path = str(urdf_path or "").strip()
        if not s_path:
            raise FileNotFoundError("no URDF file path given")
        if not os.path.isfile(s_path):
            raise FileNotFoundError(f"URDF file does not exist: {s_path}")

        # Validate/read the content ourselves so urdfdom never receives an empty
        # or unreadable path (its failure path has been seen to crash the process
        # with a native SIGABRT instead of raising a Python exception).
        try:
            with open(s_path, "r", encoding="utf-8") as s_f:
                s_xml = s_f.read()
        except OSError as s_e:
            raise FileNotFoundError(f"cannot read URDF {s_path}: {s_e}") from s_e
        s_xml = s_xml.lstrip("\ufeff")
        if not s_xml.strip():
            raise ValueError(f"URDF file is empty: {s_path}")

        s_model = pin.buildModelFromXML(s_xml)
        if s_model.nq == 0:
            raise ValueError(f"no movable joints found in {urdf_path}")
        s_model.gravity = pin.Motion(
            _DEFAULT_GRAVITY.astype(float).copy(), np.zeros(3)
        )

        s_joints: list[dict] = []
        s_index: dict[int, int] = {}
        for s_i in range(1, s_model.njoints):
            if s_model.joints[s_i].nv == 0:
                continue  # fused/fixed joints carry no dof
            s_id = len(s_joints) + 1
            s_joints.append(
                {
                    "id": s_id,
                    "name": s_model.names[s_i],
                    "mass": float(s_model.inertias[s_i].mass),
                    "com": np.asarray(s_model.inertias[s_i].lever, dtype=np.float64).copy(),
                }
            )
            s_index[s_id] = s_i

        # Parse the URDF XML ourselves to keep the element tree for save().
        s_root = ET.fromstring(s_xml)
        s_urdf_joints = {s_j.get("name"): s_j for s_j in s_root.findall("joint")}
        s_urdf_links = {s_l.get("name"): s_l for s_l in s_root.findall("link")}

        s_scale: list[float] = []
        s_joint_els: dict[int, object] = {}
        s_link_els: dict[int, object] = {}
        for s_j in s_joints:
            s_id = s_j["id"]
            s_jel = s_urdf_joints.get(s_j["name"])
            if s_jel is not None:
                s_joint_els[s_id] = s_jel
                s_child = s_jel.find("child")
                s_link_name = s_child.get("link") if s_child is not None else None
                if s_link_name and s_link_name in s_urdf_links:
                    s_link_els[s_id] = s_urdf_links[s_link_name]
            s_scale.append(1.0)

        with self._lock:
            self._model = s_model
            self._data = s_model.createData()
            self._joints = s_joints
            self._index = s_index
            self._scale = s_scale
            self._xml_root = s_root
            self._joint_els = s_joint_els
            self._link_els = s_link_els
            self._path = s_path
        return self

    # ---- query -----------------------------------------------------------

    @property
    def dof(self) -> int:
        return len(self._joints)

    @property
    def joints(self) -> list[dict]:
        return list(self._joints)

    def joint_names(self) -> list[str]:
        return [s_j["name"] for s_j in self._joints]

    def mass(self, joint: int) -> float:
        return self._joints[joint - 1]["mass"]

    # ---- parameter updates ------------------------------------------------

    def set_mass(self, joint: int, mass_kg: float) -> None:
        if self._model is None:
            raise RuntimeError("no model loaded")
        if mass_kg < 0.0:
            raise ValueError(f"mass must be >= 0, got {mass_kg}")
        with self._lock:
            self._model.inertias[self._index[joint]].mass = float(mass_kg)
            self._joints[joint - 1]["mass"] = float(mass_kg)

    def scale(self, joint: int) -> float:
        with self._lock:
            return self._scale[joint - 1]

    def scales(self) -> np.ndarray:
        with self._lock:
            return np.asarray(self._scale, dtype=np.float32).copy()

    def set_scale(self, joint: int, scale: float) -> None:
        if self._model is None:
            raise RuntimeError("no model loaded")
        with self._lock:
            self._scale[joint - 1] = float(scale)

    # ---- persistence --------------------------------------------------------

    def save(self, path: str | None = None) -> str:
        """Write the current masses and axis directions back into a URDF.

        Masses overwrite each link's ``<inertial><mass value=...>``. For any
        joint whose per-axis scale is negative (reversed direction), the joint's
        ``<axis>`` element is negated in place. No extra attributes are added to
        the URDF. Defaults to overwriting the loaded file.
        """
        with self._lock:
            if self._xml_root is None:
                raise RuntimeError("no model loaded")
            s_path = str(path or self._path or "").strip()
            if not s_path:
                raise ValueError("no output path given; load a URDF or pass a path")
            for s_id in range(1, self.dof + 1):
                s_link = self._link_els.get(s_id)
                if s_link is not None:
                    self._set_mass_element(s_link, self._joints[s_id - 1]["mass"])
                if self._scale[s_id - 1] < 0:
                    self._flip_axis(self._joint_els.get(s_id))
            s_xml = '<?xml version="1.0"?>\n' + ET.tostring(self._xml_root, encoding="unicode")

        with open(s_path, "w", encoding="utf-8") as s_f:
            s_f.write(s_xml)

        with self._lock:
            self._path = s_path
        return s_path

    @staticmethod
    def _set_mass_element(link_el, mass_kg: float) -> None:
        s_mass = link_el.find("inertial/mass")
        if s_mass is None:
            s_inertial = link_el.find("inertial")
            if s_inertial is None:
                s_inertial = ET.SubElement(link_el, "inertial")
            s_mass = ET.SubElement(s_inertial, "mass")
        s_mass.set("value", f"{float(mass_kg):.6g}")

    @staticmethod
    def _flip_axis(joint_el) -> None:
        if joint_el is None:
            return
        s_axis = joint_el.find("axis")
        if s_axis is None:
            return
        s_xyz = s_axis.get("xyz", "0 0 0")
        try:
            s_flipped = " ".join(f"{-float(p):.6g}" for p in s_xyz.split())
        except ValueError:
            return
        s_axis.set("xyz", s_flipped)

    # ---- dynamics -----------------------------------------------------------

    def gravity_torque(self, q, base_gravity=None, k_gravity: float = 1.0) -> np.ndarray:
        """Generalized gravity torques (float32[len]) scaled by ``k_gravity``.

        ``base_gravity`` defaults to the world ``[0, 0, -9.81]``; pass the
        firmware-reported ``ArmState.m_base_gravity`` for IMU-aware
        compensation (see ``examples/02_gravity_compensation.cpp``).
        """
        import pinocchio as pin

        with self._lock:
            s_model, s_data = self._model, self._data
            if s_model is None:
                raise RuntimeError("no model loaded")
            if base_gravity is not None:
                s_model.gravity = pin.Motion(
                    np.asarray(base_gravity, dtype=float).ravel().copy(), np.zeros(3)
                )
            s_q = np.asarray(q, dtype=np.float64).ravel()
            if s_q.size != s_model.nq:
                raise ValueError(f"expected nq={s_model.nq}, got {s_q.size} joints")
            s_g = pin.computeGeneralizedGravity(s_model, s_data, s_q)
        return float(k_gravity) * np.asarray(s_g, dtype=np.float32)