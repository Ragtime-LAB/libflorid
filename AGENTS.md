# libflorid-underwater agent guide

This repository is the host SDK for the Ragtime Underwater dual-arm firmware.
Keep changes scoped to the Underwater USB Bulk product API. Do not restore the
generic Arm, UDP, model-generation, or MPC layers removed from this branch.

## Build and test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DBUILD_PYFLORID=ON
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tools/underwater_console/tests -v
python3 tools/underwater_console/run.py --smoke
```

`LF_ENABLE_WLC=OFF` is the normal build. It verifies and compiles
`generated/wirelink/` without resolving or executing WLC. For schema work,
apply `protocol-patches/underwater-management.patch`, configure with the paired
ABI 32 compiler, and use `lf_update_wirelink` / `lf_check_wirelink` as described
in `protocol-patches/README.md`.

## API and behavior

- Public C++ API: `UnderwaterTeleop`, `UnderwaterTypes`, device discovery, and firmware-update DTOs.
- Python API mirrors the C++ Underwater API through `pyflorid`.
- Control telemetry and commands contain ten motors in master four joints,
  master gripper, slave four joints, slave gripper order.
- Control commands support only MIT and MitWithGravity.
- Closing an SDK object does not request Damp. Call `disable()` explicitly.
- Device settings are persisted by firmware; the desktop console performs the
  user confirmation and Damp transition before writing them.
- Zeroing is admitted by firmware only from Damp.

Use C++20. Method parameters and local result values conventionally use the
`s_` prefix; members use `m_`.
