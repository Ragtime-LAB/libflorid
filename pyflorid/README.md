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

Or build from the repository root or an unpacked source distribution (requires
C11/C++20, CMake, and libusb development files; pip installs the Python build
dependencies):

```bash
pip install .
```

Source builds use the checked-in C/H bindings by default (`LF_ENABLE_WLC=OFF`)
and do not find, download, or execute WLC/Rust. The sdist includes these bindings,
their verification manifest, Wirelink, and the FCI/Astrial sources. CMake rejects
stale or modified bindings. Native and Python build dependencies are still needed.
To regenerate bindings while developing the schema, set
`CMAKE_ARGS="-DLF_ENABLE_WLC=ON -DWLC_EXECUTABLE=/absolute/path/to/wlc -DWIRELINK_WLC_AUTO_DOWNLOAD=OFF"`
with the matching ABI 26 compiler. See the root README for the
`lf_update_wirelink` and `lf_check_wirelink` development targets.

Native build files are retained in `build/python/{wheel_tag}` (one directory per
Python ABI/platform); they are not included in the sdist or wheel. This also
avoids removing MSVC's working directory while its compiler server is still
running. Windows wheel CI uses vcpkg `x64-windows-static-md`: libusb is static,
while the C runtime matches CPython's dynamic CRT.

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
