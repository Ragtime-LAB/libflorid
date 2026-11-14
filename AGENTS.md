# libflorid-underwater agent guide

This repository is the host SDK for Ragtime Underwater firmware: the existing
dual-arm USB product and the four-motor Underwater Single USB/UART product.
Keep changes scoped to these Underwater product APIs. Do not restore the
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
edit the tracked `protocol/` sources, configure with the paired ABI 32 compiler,
and use `lf_update_wirelink` / `lf_check_wirelink` as described in
`protocol-patches/README.md`.

## API and behavior

- Public C++ API: `UnderwaterTeleop`, `UnderwaterTypes`, device discovery, and firmware-update DTOs.
- Python API mirrors the C++ dual-arm API through `pyflorid`; `UnderwaterSingle`
  is a separate Python facade over native USB and Python UART backends.
- Dual-arm control telemetry and commands contain ten motors in master four joints,
  master gripper, slave four joints, slave gripper order.
- Control commands support only MIT and MitWithGravity.
- Closing an SDK object does not request Damp. Call `disable()` explicitly.
- Dual-arm settings are persisted by firmware; its console performs the
  user confirmation and Damp transition before writing them.
- Underwater Single is three joints plus gripper, four-element arrays in
  j1/j2/j3/gripper order, 250 Hz command/state, and three gravity settings. Its
  asynchronous settings writes do not implicitly change mode.
- Single Control is exclusive to USB or UART per session. Other links may query
  or request Damp; switch ownership only after Damp. Closing stops the local
  producer, never sends implicit Damp or resumes motion.
- Single firmware update/reboot is USB only; UART uses independent Protocol 3.1
  at 921600 8N1, not Wirelink. See docs/single.md and the single console README.
- Dual-arm remote Zeroing is admitted only from Damp. Underwater Single local
  PA15 long-release enters Zeroing (mode 4), including from Control; short-release
  commits four-motor zero. Damp cancels, normal Drag/Control requests are Busy.
  Firmware 0.4.0 / Protocol 3.2 reports ZeroingFailed=5 and never resumes motion.

Use C++20. Method parameters and local result values conventionally use the
`s_` prefix; members use `m_`.
