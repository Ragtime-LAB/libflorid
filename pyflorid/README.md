# pyflorid

Python bindings for [libflorid](https://github.com/Ragtime-LAB/libflorid) — a
C++20 variable-frequency arm control SDK (USB transport, 6-DOF, real-time
control loop).

This package wraps the native `florid` library with pybind11 and exposes the
`Arm`, `Gripper`, control-loop command types, and the `Model` kinematics
interface to Python.

## Install

```bash
pip install pyflorid
```

Or build from source (requires a C++20 toolchain, pybind11, and NumPy):

```bash
pip install ./pyflorid
```

See the top-level `README.md` for the full SDK documentation.

USB Bulk discovery and connection are available directly from Python:

```python
from pyflorid import Arm, DeviceSelector, discover_devices, wait_for_device

for device in discover_devices(probe=True):
    print(device.display_name, device.serial_number, device.uri)

arm = Arm.connect()                         # exactly one visible arm
arm = Arm.connect_by_serial("RF-H7-001")   # immutable identity
arm = Arm.connect_by_name("left-arm")      # duplicate names are rejected

device = wait_for_device(DeviceSelector.by_serial("RF-H7-001"), 10_000)
arm = Arm.connect_device(device)
```
