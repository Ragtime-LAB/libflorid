# libflorid — Variable-Frequency Arm Control SDK

**libflorid** is a C++ library for real-time motion control of robotic arms. It communicates with the arm controller over TCP+UDP and supports variable-frequency control frames (up to 1 kHz). The arm-side interpolation algorithm handles frame rate mismatch automatically, so the client can run at 100 Hz, 10 Hz, or any rate below 1 kHz without reconfiguration.

The public API is STL-free (no `std::vector`, `std::function`, or `std::string` in headers), making it suitable for both Linux host development and embedded platforms such as Arduino and STM32.

## Key Features

- **Variable-frequency control**: 1 Hz to 1 kHz, no mode negotiation required. The arm interpolates to its internal 1 kHz servo loop.
- **Compile-time dynamics**: CasADi + Pinocchio generates optimized C code from URDF. Full forward kinematics, Jacobians, mass matrix, Coriolis, and gravity — all resolved at compile time with zero runtime allocation.
- **STL-free public headers**: Compatible with Arduino and bare-metal toolchains (C++17 features used without STL containers).
- **Per-frame impedance gains**: Every control packet carries its own `kp/kd` values, eliminating TCP/UDP race conditions.
- **IMU-based gravity alignment**: `base_gravity` field from the arm's onboard IMU means gravity compensation works regardless of mounting orientation (table, wall, ceiling, angled).
- **Online mode switching**: Switch between joint-position, joint-velocity, torque, Cartesian-pose, and Cartesian-velocity control modes over a reliable TCP command, without stopping the control loop.
- **Platform-independent transport**: Inject custom `Transport` implementations for CAN bus, serial, or UAVCAN.
- **Bundled 3rd-party code only**: No submodules, no FetchContent — just `git clone` and `cmake`.

## System Requirements

### Host (Linux)

| Requirement | Minimum |
|---|---|
| OS | Ubuntu 20.04+ / Arch / any Linux |
| Compiler | GCC 9+ or Clang 14+ |
| CMake | 3.20+ |
| Build system | Ninja (recommended) or Make |

Optional (only for running the URDF-to-Traits generator):

| Tool | Purpose |
|---|---|
| Python 3.9+ | Generator runtime |
| CasADi | Symbolic code generation |
| Pinocchio (with CasADi bindings) | Kinematics/dynamics export |

### Embedded

Any platform with a C++17 compiler and a minimal C standard library (`<stdint.h>`, `<stddef.h>`, `<math.h>`). No heap allocation at runtime in the library core.

## Quick Start

```bash
git clone https://github.com/Ragtime-LAB/libflorid.git
cd libflorid
cmake -S . -B build -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

## Usage Example

```cpp
#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>

int main() {
    // One-line connection (TCP 6041, UDP 6040)
    florid::Arm arm("192.168.1.100");

    // Apply safety defaults from the traits constant
    arm.setCollisionBehavior(
        florid::PantheraTraits::collision_lower,
        florid::PantheraTraits::collision_upper);
    arm.setWatchdogTimeout(florid::Duration(200));
    arm.setMaxFrequencyHz(1000.0);

    // Model uses compile-time generated dynamics
    florid::Model<florid::PantheraTraits> model;

    // Torque-mode control with gravity compensation
    arm.control([&](const florid::ArmState& s, florid::ArmControl&)
        -> florid::Torques
    {
        float g[6];
        model.gravity(s.q, s.base_gravity, g);  // IMU-aware

        florid::Torques cmd;
        for (int i = 0; i < 6; ++i)
            cmd.tau[i] = 600.0f * (q_desired[i] - s.q[i])    // PD
                       + 50.0f  * (0.0f - s.dq[i])           // damping
                       + g[i];                                // gravity
        return cmd;
    });
}
```

## Architecture

```
┌──────────────────────────────────────────────┐
│                   User Code                   │
│    Arm arm("ip"); Model<Traits> model;         │
├──────────────────────────────────────────────┤
│  Arm.hpp     — public API (STL-free)          │
│  Model.hpp   — template <typename Traits>     │
│  ArmConfig.hpp — compile-time config (legacy) │
├──────────────────────────────────────────────┤
│  core/       — ArmCore, types, traits helper  │
│  detail/     — Transport, Seqlock, ASIO impl  │
├──────────────────────────────────────────────┤
│  protocols/  — binary packet definitions      │
│  3rdparty/   — BasicLinearAlgebra (vendored)  │
│  traits/     — generated PantheraTraits.hpp   │
└──────────────────────────────────────────────┘
```

| Layer | Description |
|---|---|
| `Arm` | Public entry point. `Arm(ip)` creates TCP+UDP automatically. `Arm(Transport&, Transport*)` allows custom transport injection for embedded platforms. |
| `Model<Traits>` | Stateless computation class. All members delegate to static `Traits` methods. Template parameter selects the arm type at compile time. |
| `ArmCore` | Internal engine. Serialises/deserialises protocol packets, manages sequence numbers, maintains thread-safe state via `SeqlockBuf`. |
| `Traits` (generated) | Static struct with `kDOF`, `kHasMass`, `kHasCoriolis`, and all FK/Jacobian/mass/coriolis/gravity functions. Generated by `urdf2traits.py` from a URDF file. |
| `Protocol` | Header-only binary protocol library. Packet types are dispatched at compile time via a variadic template `Dispatcher`. |
| `BasicLinearAlgebra` | Vendored single-header matrix library (MIT). Provides `Matrix<R,C>` with compile-time dimension checking. |

## Build Options

```bash
cmake -S . -B build \
    -DBUILD_TESTS=ON       # Build unit tests (including protocol tests)
    -DBUILD_EXAMPLES=ON    # Build example programs (default ON)
    -DCMAKE_BUILD_TYPE=Release
```

## Examples

13 example programs demonstrating common use cases:

| Example | Demonstrates |
|---|---|
| `echo_arm_state` | Continuous state monitoring via `arm.read()` |
| `send_joint` | Joint position commands, per-joint cycling test |
| `send_cartesian` | Cartesian pose commands with linear sweep |
| `send` | Variable-frequency send with jitter |
| `active_joint_control` | Manual control loop with `startJointPositionControl()` |
| `joint_impedance_control` | Torque-mode PD + gravity compensation with IMU |
| `cartesian_impedance_control` | Cartesian spring-damper using Model FK + Jacobian |
| `force_control` | PI force control via Jacobian transpose |
| `generate_consecutive_motions` | Multi-segment motion with `MotionFinished` |
| `motion_with_control` | Hybrid torque + motion callback |
| `joint_velocity_motion` | Velocity control mode |
| `joint_point_to_point_motion` | Cubic polynomial point-to-point |
| `switch_control_mode` | Online mode switching via TCP |

Build examples:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

Run an example:

```bash
./build/examples/echo_arm_state/florid_example_echo_arm_state 192.168.1.100
```

## Switching Arm Types

Generated traits files live in `include/florid/traits/`. To target a different arm, change one include and one template parameter:

```cpp
// Panthera
#include <florid/traits/PantheraTraits.hpp>
florid::Model<florid::PantheraTraits> model;

// Willow
#include <florid/traits/WillowTraits.hpp>
florid::Model<florid::WillowTraits> model;
```

Everything else stays the same — Arm, callbacks, examples, and safety configuration.

## Testing

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

Tests cover:
- Protocol packet sizes, types, serialisation (60 tests)
- Model FK orthogonality, symmetry, and correctness (9 tests, or more with additional traits)
- Config type enum values (automatically extended when new configs are added)

## License

libflorid is released under the ISC License.

Third-party code:
- `3rdparty/BasicLinearAlgebra/` — MIT License (copyright 2019 tomstewart89)
