"""Gravity-compensation tuning tool for the libflorid SDK.

Layers:
* ``model``        — URDF loaded with Pinocchio; runtime-adjustable link masses
* ``controller``   — background ``JointMIT`` feedforward loop via pyflorid
* ``app``          — tkinter GUI
"""

from .controller import GravityCompController, describe_errors
from .model import GravityModel

__all__ = ["GravityModel", "GravityCompController", "describe_errors"]