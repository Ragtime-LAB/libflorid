# libflorid — Arm Control SDK

**libflorid** is a C++20 SDK for the Ragtime Usb2Arm / Willow 6-DOF robotic arm. It talks to the arm controller over USB or UDP using generated FCI Wirelink bindings, exposes six real-time control modes plus gripper control, and ships compile-time generated dynamics via `Model<Traits>`. Python bindings live in `pyflorid/`.

## Key Features

- **Native USB Bulk transport**: `discoverDevices()` enumerates only Florid
  products and can read protocol identity without taking the control lease.
  `Arm::connect()` connects the only device, or selects by immutable serial or
  user-editable custom name. Multiple matches fail safely instead of selecting
  an arbitrary arm. `serial://<port>` remains available for legacy CDC/debug
  firmware. UDP via `Arm::create("udp://<ip>:<port>")` binds a fixed local
  endpoint and learns the device's source endpoint from the first datagram.
- **Six control modes**: `JointMIT`, `JointPosVel`, `JointVel`, `JointPVT`, `CartesianPose`, `CartesianVelocities`. Each frame carries its own `kp/kd`, an optional firmware-gravity flag, and a `MotionFinished` marker.
- **Two control styles**: blocking `Arm::control(cb)` runs your callback on an internal thread at the firmware rate, or `Arm::start*Control()` returns a polling `ActiveControl<T>` with `readOnce()`/`writeOnce()` (this is what the Python bindings use).
- **Gripper control**: `arm->gripper()` supports the joint control modes (motor joint_id 7), with state in `GripperState` / `ArmState`.
- **Compile-time dynamics**: `Model<WillowTraits>` / `Model<PantheraTraits>` provides FK, pose, zero/body Jacobian, mass matrix, Coriolis, and gravity — generated from URDF by `scripts/urdf2traits.py`, resolved at compile time with no runtime allocation.
- **Motor registers**: read/write control-loop gains and protection parameters per joint (1–6 arm, 7 gripper), store to flash, and set zero point.
- **Device management**: `DeviceInfo` exposes the immutable full serial and
  user-editable custom name; `Arm::setCustomName()` cannot rewrite product
  identity. `Arm::setDeviceSettings()` caches the settings actually accepted
  by firmware, including any normalization, and leaves its cache unchanged on
  rejection. Diagnostics, error recovery, homing, and motor registers share
  the same typed API.
- **Optional MPC**: `florid::CartesianMPCSolver<WillowMPCTraits>` over the acados solver (build with `-DBUILD_MPC=ON`).
- **Python bindings**: install `pyflorid` via pip (pybind11), expose the same API with snake_case names.

## System Requirements

| Requirement | Minimum |
|---|---|
| Compiler | GCC 12+ or Clang 15+ (C++20) |
| CMake | 3.20+ |
| Wirelink + `wlc` | Codegen ABI 16-compatible pair |
| Build system | Ninja (recommended) or Make |
| OS | Linux, macOS, or Windows (USB Bulk via Astrial/libusb) |

Optional build-time tools:

| Tool | Purpose |
|---|---|
| pybind11 + NumPy (≥ 2.0) headers | Python bindings (`-DBUILD_PYFLORID=ON`) |
| acados | MPC (`-DBUILD_MPC=ON`) |
| Python 3.9+ + CasADi + Pinocchio (with CasADi bindings) | Regenerating traits / MPC sources from URDF |

## Submodules

```bash
git submodule update --init protocol 3rdparty/astrial 3rdparty/wirelink
# Only when configuring with BUILD_MPC=ON:
git submodule update --init --recursive 3rdparty/acados
```

- `protocol/` → FCI `.wl` schemas and host/firmware binding profiles
- `3rdparty/astrial` (cross-platform serial and native USB Bulk backend)
- `3rdparty/wirelink` (link core, desktop adapters, and host runtime)
- `3rdparty/acados` — only needed when `-DBUILD_MPC=ON`

## Build & Test

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Defaults: `BUILD_TESTS=OFF`, `BUILD_EXAMPLES=ON`, `BUILD_PYFLORID=OFF`, `BUILD_MPC=OFF`.

The bundled `3rdparty/wirelink` source is used by default. Pass
`-DWIRELINK_SOURCE_DIR=/path/to/wirelink` only to override it during coordinated
development. Wirelink downloads its pinned WLC host release when no compatible
compiler is on `PATH`; offline builds may set `WLC_EXECUTABLE` explicitly. The
FCI host sources are generated in the build tree and are never committed.

### Windows (MSVC + vcpkg)

The repository manifest declares the product's native USB dependency. From a
Developer PowerShell, set the vcpkg root and use the checked-in multi-config
preset; the configure step installs the pinned libusb version into the build
tree and downloads the pinned WLC host compiler when necessary:

```powershell
git submodule update --init protocol 3rdparty/astrial 3rdparty/wirelink
$env:VCPKG_ROOT = "C:\src\vcpkg"
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-release --parallel
ctest --preset windows-release
```

No separate `vcpkg install` command is required. The default dynamic triplet
also performs app-local deployment of `libusb-1.0.dll` for built tests and
examples. The same configure tree supports `windows-debug`; use the
`windows-msvc-vcpkg-static`, `windows-static-release`, and
`windows-static-debug` presets when static runtime distribution is an explicit
product choice.

## Quick Start

```cpp
#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/DeviceDiscovery.hpp>
#include <florid/traits/WillowTraits.hpp>

#include <iostream>
#include <utility>

int main() {
    // Fast USB discovery, with optional read-only Wirelink identity probing.
    auto discovery = florid::discoverDevices({.m_probe = true});
    if (!discovery || discovery.m_devices.empty()) {
        return 1;
    }
    for (const auto& device : discovery.m_devices) {
        std::cout << device.m_display_name << "  "
                  << device.serialNumber() << "  "
                  << device.uri() << '\n';
    }

    // Or use Arm::connect() directly when exactly one arm is attached.
    auto connected = florid::Arm::connect(discovery.m_devices.front());
    if (!connected) return 1;
    auto arm = std::move(connected.m_arm);

    arm->home();

    // Compile-time generated dynamics for the Willow arm
    florid::Model<florid::WillowTraits> model;

    float q_des[6] = {0, 0, 0, 0, 0, 0};

    // MIT (impedance/torque) mode with host-side gravity compensation
    arm->control([&](const florid::ArmState& s, florid::ArmControl&) -> florid::JointMIT {
        float g[6];
        model.gravity(s.m_q, s.m_base_gravity, g);  // IMU-aware

        florid::JointMIT cmd;
        for (int i = 0; i < 6; ++i) {
            cmd.m_q[i]    = 0.0f;
            cmd.m_dq[i]   = 0.0f;
            cmd.m_tau[i]  = 600.0f * (q_des[i] - s.m_q[i])  // PD
                          + 50.0f  * (0.0f - s.m_dq[i])     // damping
                          + g[i];                            // gravity
            cmd.m_kp[i]   = 600.0f;
            cmd.m_kd[i]   = 50.0f;
        }
        cmd.m_firmware_gravity = false;
        return cmd;
    });
}
```

### Python

```bash
pip install .                 # or: pip install -e .
```

```python
import numpy as np
from pyflorid import Arm, JointMIT

arm = Arm.create("usb://2fe3:574c")
ctrl = arm.start_joint_mit_control()

state = ctrl.read_once()
q_des = np.array(state.q, dtype=np.float32)

cmd = JointMIT()
cmd.q = q_des
cmd.dq = np.zeros(6, dtype=np.float32)
cmd.tau = np.zeros(6, dtype=np.float32)
cmd.kp = np.full(6, 10.0, dtype=np.float32)
cmd.kd = np.full(6, 0.2, dtype=np.float32)
cmd.firmware_gravity = True
ctrl.write_once(cmd)
```

The C++ `s_`-prefixed methods are bound to snake_case names (`firmware_period_us`, `start_joint_mit_control`, `read_once`, `write_once`, ...). See `pyflorid/examples/pd_hold.py` for a complete example.

## Architecture

```
┌──────────────────────────────────────────────────────┐
│                 User code (C++ / Python)              │
│   Arm::create("usb://...")   Model<Traits>   Gripper  │
├──────────────────────────────────────────────────────┤
│  include/florid/     public API (Arm, Model, types)   │
│    core/    ActiveControl                              │
│    detail/  Transport, ArmImpl, FciWirelinkEndpoint,  │
│             WirelinkExecutor, LatencyEstimator        │
│    traits/  WillowTraits, PantheraTraits (generated)  │
│    mpc/     CartesianMPC                               │
├──────────────────────────────────────────────────────┤
│  src/                implementation (Arm/ArmImpl/...)  │
│  protocol/           FCI .wl schemas + binding profiles│
│  3rdparty/           astrial (USB Bulk/serial), acados│
│  generated/          acados solver + WillowMPCTraits  │
│  pyflorid/           Python bindings (pybind11)       │
└──────────────────────────────────────────────────────┘
```

| Layer | Description |
|---|---|
| `Arm` | Public entry point. Move-only PIMPL over `detail::ArmImpl`. `Arm::create(uri)` builds the transport and fetches `DeviceInfo`/`DeviceSettings` on construction; firmware period comes from `firmware_dt_us`. |
| `Arm::control(...)` | Blocking control loop; the callback runs on an internal thread and returns one of the six control types. `ArmControl` exposes latency/jitter diagnostics and `finishMotion()`/`stopControl()`. |
| `ActiveControl<T>` | Manual read/write polling handle returned by `start*Control()`. Reads `ArmState` with `readOnce()`, sends commands with `writeOnce()`. |
| `Gripper` | `arm->gripper()`; same control modes and `ActiveControl` polling for the gripper motor (joint_id 7). |
| `Model<Traits>` | Stateless computation delegating to generated `Traits` (`fk`, `pose`, Jacobians, `mass`, `coriolis`, `gravity`). Switch arm models by changing the template parameter. |
| `detail::Transport` | Transport lifecycle abstraction. Native USB Bulk claims Wirelink RX storage directly and only wakes the endpoint owner from I/O callbacks; serial/UDP retain the push-driven byte path. |
| `FciWirelinkEndpoint` | Single-owner host runtime generated from `protocol/schema/wirelink/arm/*.wl`. Telemetry is copied from borrowed LATEST views, realtime commands use message-ID keyed coalescing lanes, and configuration uses typed reliable RPCs plus a renewable control lease. |

## Build Options

```bash
cmake -S . -B build \
    -DBUILD_TESTS=ON       # Unit tests (mock transport; no hardware needed)
    -DBUILD_EXAMPLES=ON    # Example programs (default ON)
    -DBUILD_PYFLORID=ON    # Python bindings via pybind11 (needs Python + NumPy dev)
    -DBUILD_MPC=ON         # MPC via acados (pulls in generated/ + 3rdparty/acados)
    -DCMAKE_BUILD_TYPE=Release
```

## Examples

Run from the `examples/` source tree; each binary takes a complete transport URI, e.g.:

```bash
./build/examples/florid_example_00_echo_arm_state usb://2fe3:574c
```

| Example | Demonstrates |
|---|---|
| `00_echo_arm_state` | List USB devices + stream `ArmState` via `Arm::read` |
| `00_read_diagnostics` | Read `ArmDiagnostics` telemetry |
| `01_drag_mode` | `Arm::drag()` + state stream |
| `01_gripper_move` | `Gripper::startJointMITControl()` open/close cycle |
| `01_joint_sine_motion` | `Arm::home()` + joint sine motion |
| `02_gravity_compensation` | `Model<Traits>::gravity` + PD in `JointMIT` |
| `03_active_joint_control` | `startJointMITControl()` polling loop |
| `03_mode_switching` | Cycling MIT / PVT / PosVel control modes |
| `04_motor_registers` | Read/write/store motor registers (joint_id 1–7) |
| `05_cartesian_mpc` | Cartesian pose + `CartesianMPCSolver` (requires `-DBUILD_MPC=ON`) |

## Switching Arm Types

Generated traits live in `include/florid/traits/`. To target a different arm, change one include and one template parameter:

```cpp
// Willow
#include <florid/traits/WillowTraits.hpp>
florid::Model<florid::WillowTraits> model;

// Panthera
#include <florid/traits/PantheraTraits.hpp>
florid::Model<florid::PantheraTraits> model;
```

Everything else — `Arm`, callbacks, examples — stays the same.

## Testing

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The tests run without hardware. `test_transport_pipeline` drives `ArmImpl`
against a fragmented Wirelink device peer and covers:

- lease-backed connection and deterministic release
- typed device settings, metadata, and motor-register RPCs
- stable `ArmStatus` snapshots after borrowed payload release
- arm/gripper command encoding over fragmented COBS streams

## License

libflorid is released under the ISC License.

Third-party code:

- `protocol/` → FCI Wirelink schemas and binding profiles
- `3rdparty/astrial` (USB serial; vendors asio, tl-expected, readerwriterqueue)
- `3rdparty/acados` (MPC, only when `-DBUILD_MPC=ON`)
