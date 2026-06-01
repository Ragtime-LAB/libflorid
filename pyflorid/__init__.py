"""libflorid Python bindings — variable-frequency arm control."""

from pathlib import Path

_pkg_dir = Path(__file__).resolve().parent
_build_pkg_dir = _pkg_dir.parent / "build" / "pyflorid"
if _build_pkg_dir.is_dir():
    __path__.append(str(_build_pkg_dir))

from ._pyflorid import (  # noqa: F401
    Arm,
    ArmControl,
    ArmMode,
    ArmState,
    CartesianPose,
    CartesianPoseControl,
    CartesianVelocities,
    CartesianVelocityControl,
    ControlMode,
    Duration,
    Frame,
    JointPositions,
    JointPositionControl,
    JointVelocities,
    JointVelocityControl,
    PantheraModel,
    SessionMode,
    TorqueControl,
    Torques,
    WillowModel,
)

Model = PantheraModel
