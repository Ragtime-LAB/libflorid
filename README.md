# libflorid-underwater

C++20 and Python SDK for the Ragtime Underwater dual-arm controller. It uses
USB Bulk and generated FCI Wirelink bindings to provide device discovery,
mode RPCs, settings, zeroing, firmware update, 250 Hz ten-motor telemetry, and
MIT control commands.

The public entry point is `florid::UnderwaterTeleop`. Supported robot modes are
Damp, Gravity, Teleop, Control, Zeroing, and the Teleop alignment transition.
Control mode accepts only MIT and MIT-with-firmware-gravity commands for master
j1-j4, master gripper, slave j1-j4, and slave gripper.

## Build

```bash
git submodule update --init 3rdparty/astrial 3rdparty/wirelink
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Normal builds use the checked-in Wirelink snapshot and do not need WLC or Rust.
Protocol regeneration is documented in
[`protocol-patches/README.md`](protocol-patches/README.md).

Build the Python module with `-DBUILD_PYFLORID=ON`, or install it directly:

```bash
python3 -m pip install .
```

## C++ usage

```cpp
#include <florid/UnderwaterTeleop.hpp>

auto connection = florid::UnderwaterTeleop::connect("usb://2fe3:574c/SERIAL");
if (!connection) return 1;
auto& robot = *connection.m_device;

robot.enable();       // Gravity
robot.enterTeleop();  // align, then follow
robot.disable();      // Damp
```

`examples/underwater_control.cpp` shows the 4 ms command loop. The SDK does not
silently change robot mode when a process closes; request Damp explicitly.

## Python console

The desktop console supports USB discovery, mode control, zeroing, persistent
device settings, firmware update, and ten-motor MIT control:

```bash
cmake -S . -B build-underwater-sdk -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_PYFLORID=ON -DBUILD_TESTS=ON
cmake --build build-underwater-sdk
python3 tools/underwater_console/run.py
```

See [`tools/underwater_console/README.md`](tools/underwater_console/README.md)
for operating details.

## Repository layout

- `include/florid/UnderwaterTeleop.hpp`: public C++ API
- `include/florid/UnderwaterTypes.hpp`: RPC, settings, telemetry, and command types
- `src/`: USB transport, discovery, Wirelink endpoint, and update implementation
- `pyflorid/`: Python bindings and example
- `tools/underwater_console/`: Tk desktop application
- `generated/wirelink/`: checked-in ABI 32 protocol snapshot
- `tests/`: host-side protocol, update, control, and packaging tests

This branch intentionally excludes the generic six-axis Arm API, UDP transport,
URDF model generation, gravity tuner, and Acados MPC runtime.

## License

ISC. Third-party components retain their own licenses under `3rdparty/`.
