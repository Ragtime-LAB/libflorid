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

Source builds also need a matching WLC (ABI 26) on PATH, or host Rust/Cargo
supporting edition 2024. Wirelink's CMake fallback fetches a SHA-256-verified
pinned source commit and builds WLC with locked dependencies; it does not use an
older same-version release binary. Rust and WLC are build tools, not wheel
runtime dependencies. For offline builds, supply
`CMAKE_ARGS="-DWLC_EXECUTABLE=/absolute/path/to/wlc -DWIRELINK_WLC_AUTO_DOWNLOAD=OFF"`.
The sdist includes Wirelink and the FCI/Astrial sources, but not downloaded Cargo
dependencies or a WLC binary. No source build is claimed to be offline by default.

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
