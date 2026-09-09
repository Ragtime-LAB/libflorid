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
- **Optional position MPC**: `florid::CartesianMPCSolver<WillowMPCTraits>` over acados (`-DBUILD_MPC=ON`). Tracks Willow end-joint position; orientation and Cartesian impedance gains are not part of this OCP.

- **Python bindings**: install `pyflorid` via pip (pybind11), expose the same API with snake_case names.

See [Willow position MPC](docs/mpc.md) for matrix layout, internal 50 Hz planning /
500 Hz output, limits, deadlines and offline verification.

## System Requirements

| Requirement | Minimum |
|---|---|
| Compiler | GCC 12+ or Clang 15+ (C++20) |
| CMake | 3.21+ |
| Wirelink | Bundled source and checked-in ABI 26 bindings |
| Build system | Ninja (recommended) or Make |
| OS | Linux, macOS, or Windows (USB Bulk via Astrial/libusb) |

Optional build-time tools:

| Tool | Purpose |
|---|---|
| pybind11 + NumPy (≥ 2.0) headers | Python bindings (`-DBUILD_PYFLORID=ON`) |
| WLC (ABI 26) | Regenerating FCI bindings (`-DLF_ENABLE_WLC=ON`) |
| Python 3.9+ + CasADi + Pinocchio (with CasADi bindings) | Regenerating traits / MPC sources from URDF |

## Submodules

```bash
git submodule update --init protocol 3rdparty/astrial 3rdparty/wirelink
```

- `protocol/` → FCI `.wl` schemas and host/firmware binding profiles
- `3rdparty/astrial` (cross-platform serial and native USB Bulk backend)
- `3rdparty/wirelink` (link core, desktop adapters, and host runtime)

MPC's acados/HPIPM/BLASFEO runtime is checked in under
[`3rdparty/acados_runtime`](3rdparty/acados_runtime/README.md), approximately
13.64 MB with licenses and metadata. `-DBUILD_MPC=ON` uses this source directly;
no additional submodule, installed acados or Python generator is required.

## Build & Test

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Defaults: `BUILD_TESTS=OFF`, `BUILD_EXAMPLES=ON`, `BUILD_PYFLORID=OFF`, `BUILD_MPC=OFF`,
`LF_ENABLE_WLC=OFF`.

The bundled `3rdparty/wirelink` source is used by default. Pass
`-DWIRELINK_SOURCE_DIR=/path/to/wirelink` only to override it during coordinated
development. This dev pins Wirelink `4b650ba03f6d4d60fcbec76520a28757f834a1af`
and ships its generated arm codec and host runtime in `generated/wirelink/`.
Normal C++ and Python source builds compile these files without finding,
downloading, or running WLC/Rust. CMake validates the snapshot's compiler ABI,
schema/profile hashes and output hashes; stale or modified snapshots fail early.

To change the FCI schema/profile or develop against another Wirelink version,
enable WLC explicitly. The bundled version pairs with WLC
`c6b6a8fa560a15c45d564aad0afd197b13682de8` (codegen ABI 26):

```bash
cmake -S . -B build/wlc -DLF_ENABLE_WLC=ON \
  -DWLC_EXECUTABLE=/path/to/wlc -DWIRELINK_WLC_AUTO_DOWNLOAD=OFF
cmake --build build/wlc
cmake --build build/wlc --target lf_update_wirelink
cmake --build build/wlc --target lf_check_wirelink
```

The normal WLC build writes to `build/wlc/generated/wirelink/`; override the root
with `-DFCI_PROTOCOL_GENERATED_BASE_DIR=/absolute/output/path`. Only the explicit
`lf_update_wirelink` target updates `generated/wirelink/` in the repository.
Commit that directory together with the input/submodule changes. The
`lf_check_wirelink` target compares fresh output with the snapshot without
changing it, and runs in CI. These two targets require `LF_ENABLE_WLC=ON`.

See the [Wirelink setup guide](3rdparty/wirelink/docs/installation.md) for WLC.
When WLC is enabled, omitting its path permits the existing pinned-source Cargo
bootstrap if host Rust/Cargo is available. The FCI schemas retain their explicit
operation/status field mappings; changing this build option does not change the
wire protocol.

Endpoint initialization now obtains a fresh identity from `Wirelink::platform`.
Applications do not choose session IDs. Advanced integrations/tests may override
`FciWirelinkEndpointConfig::m_session_source` with a `wl_session_source_t` callback;
the callback runs only during initialization and must return a fresh nonzero ID.
Failure is reported, never replaced by clock/address-derived randomness. The
existing executor monotonic clock remains the only source for RPC deadlines.
Mapped FCI still randomizes its initial operation ID; it does **not** gain managed
RPC v2's client-session echo validation merely by upgrading the dependency.

### Windows (MSVC + vcpkg)

The repository manifest declares the product's native USB dependency. From a
Developer PowerShell, set the vcpkg root and use the checked-in multi-config
preset; the configure step installs the pinned libusb version into the build
tree. The default build uses the checked-in bindings:

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
│  3rdparty/           astrial, wirelink, acados_runtime│
│  generated/          FCI bindings + acados/traits    │
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
    -DBUILD_MPC=ON         # MPC via the checked-in acados_runtime
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
| `05_cartesian_mpc` | 50 Hz position MPC / internal 500 Hz output, hold-only and CSV (requires `-DBUILD_MPC=ON`) |

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
- [`3rdparty/acados_runtime`](3rdparty/acados_runtime/README.md) (acados, HPIPM,
  BLASFEO; BSD-2-Clause, compiled only when `-DBUILD_MPC=ON`)
