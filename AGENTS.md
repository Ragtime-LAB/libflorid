# libflorid — Agent Guide

C++20 variable-frequency arm control SDK. USB transport, 6-DOF, real-time control loop.

## Build & test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure   # single test, no hardware
```

| Flag | Default | Notes |
|---|---|---|
| `BUILD_TESTS` | OFF | Single test via CTest, no hardware needed |
| `BUILD_EXAMPLES` | ON | 9 executables (10 with `BUILD_MPC`) |
| `BUILD_PYFLORID` | OFF | Needs pybind11 + NumPy |
| `BUILD_MPC` | OFF | Enables acados solver (submodule with nested submodules) |
| `LF_ENABLE_WLC` | OFF | Compile checked-in FCI bindings; ON generates with matching host WLC |

`pip install .` builds the `pyflorid` wheel (scikit-build-core sets
`-DBUILD_PYFLORID=ON` automatically via `pyproject.toml` at the repo root).
Python CI/CD lives in `.github/workflows/` (`wheels.yml`, `publish-pypi.yml`),
built with cibuildwheel; `acados` is excluded (MPC stays OFF for bindings).

## Submodules

`git clone --recurse-submodules`:
| Path | Upstream |
|---|---|
| `protocol/` | FCI `.wl` schemas, WLC profiles, and generated Wirelink component targets |
| `3rdparty/astrial/` | Cross-platform serial library (ASIO, io_uring on Linux) |
| `3rdparty/acados/` | MPC solver; needs `--recurse-submodules` (blasfeo, hpipm nested) |

## Key targets

| Target | Path | Links |
|---|---|---|
| `florid` (lib) | `src/*.cpp` | `fci_protocol` + `astrial` |
| `fci_protocol::arm` | `generated/wirelink/` by default | ABI-26 host endpoint linked with Wirelink |
| `astrial` | `3rdparty/astrial/` | Static lib, vendored ASIO |
| `florid_example_*` | `examples/` | `florid` |
| `test_transport_pipeline` | `tests/` | `MockTransport`, no hardware |
| `_pyflorid` (.so) | `pyflorid/` | pybind11 module, links `florid` |

## API essentials

**URI** formats passed to `Arm::create()`:
- `usb:///dev/ttyACM0` — `usb://` prefix mandatory
- `udp://<ip>:<port>` — binds a fixed local UDP endpoint (e.g. `udp://192.168.1.200:5080`), learns the device source endpoint from the first received datagram, and carries the same Wirelink COBS stream over UDP (best-effort)

**Control loops** (two APIs):
1. **Callback**: `arm.control([](const ArmState&, ArmControl&) -> ControlCmd { ... })` — blocking, internal thread. Return type is one of `JointMIT`, `JointPosVel`, `JointVel`, `JointPVT`, `CartesianPose`, `CartesianVelocities`.
2. **Active control**: `auto ac = arm.startJointMITControl()` → `ac->readOnce()` / `ac->writeOnce(cmd)` — caller owns the loop. `Gripper` has a similar pattern (only `JointMIT` active control, but all 4 joint-mode callbacks).

**`MotionFinished`**: Static method — `JointMIT::MotionFinished(cmd)` returns a copy with `m_motion_finished=true`.

**`JointMIT`** has `m_firmware_gravity` flag.

**`ArmState`** fields are `float[6]` (`m_q`, `m_dq`, `m_tau`, ...). Also has `m_gripper_q`, `m_gripper_dq`, `m_gripper_tau` (single floats).

**`Errors`**: `uint32_t` bitset with named accessors; bits 0–10 defined (position limits, velocity, collision, reflexes, E-stop, watchdog, etc.).

**Motor register helpers** in `florid::motor` (`MotorRegisters.hpp`). Joint IDs: 1–6 arm, 7 gripper.

**Model/Kinematics**: `Model<Traits>` — `forwardKinematics`, `zeroJacobian`, `bodyJacobian`, `pose`, `mass`, `coriolis`, `gravity`. Two CasADi-generated traits: `WillowTraits` and `PantheraTraits` (both include safety params like `collision_lower`, `cartesian_impedance`, `watchdog_timeout_ms`).

**MPC**: `CartesianMPCSolver<WillowMPCTraits>`, position-only acados OCP. Enable via `-DBUILD_MPC=ON`. C solver sources in `generated/c_generated_code/`. Column-major target translation is `[12..14]`; orientation and Cartesian gains are unused. Five 4 ms intervals; command stage 1 as JointPVT. See `docs/mpc.md` for limits and fallback behavior. `test_cartesian_mpc` exercises the actual solver without hardware when MPC/tests are enabled.

## Conventions

- Template params/return locals: `s_` prefix (`s_arm`, `s_cb`). Class members: `m_` prefix.
- C++20, `float[6]` for joint arrays, `std::uint32_t` errors bitset.
- Public headers: `include/florid/*.hpp`. Private detail: `include/florid/detail/`.

## Codegen

| Script | Output | Prerequisites |
|---|---|---|
| `scripts/urdf2traits.py` | `include/florid/traits/*Traits.hpp` | CasADi + Pinocchio |
| `scripts/urdf2mpc.py` | `generated/` C code + `WillowMPCTraits.hpp` | CasADi + Pinocchio + acados_template |

Both run offline; generated files are committed.

MPC C/C++ adapter templates live in `scripts/templates/`. Run
`python3 scripts/urdf2mpc.py --wrappers-only` after changing them (NumPy only).
Changing model/cost expressions requires full URDF generation, including the
gravity reference function. Host CI compares regenerated adapters and runs MPC
under ASan/UBSan with GENERIC BLASFEO/HPIPM.

FCI bindings are checked in under `generated/wirelink/` and verified by CMake
without WLC. For schema/profile updates, configure with `-DLF_ENABLE_WLC=ON`
and a matching `WLC_EXECUTABLE`, then build `lf_update_wirelink` to refresh the
snapshot. Commit the snapshot with the input/submodule changes. Build
`lf_check_wirelink` to compare fresh WLC output against the checked-in files.
Normal WLC output stays in the build tree; `FCI_PROTOCOL_GENERATED_BASE_DIR`
can override its root. The snapshot is the SDK's standard arm/host recipe.

## Platform notes

- io_uring auto-detected on Linux (kernel >= 5.15 + liburing). Disable with `-DASTRIAL_IO_URING=OFF`.
- USB enumeration reads `/sys/class/tty` on Linux.
- Host CI checks both FCI build modes and snapshot regeneration; wheel CI uses the snapshot.
- License: ISC.
- CMake minimum: 3.21.
- Build and regeneration instructions are in `README.md` and `README_CN.md`.
