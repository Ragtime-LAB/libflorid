# WLC 0.5.0 / Wirelink main integration

Validated on 2026-09-09. This updates dependencies and generated bindings, not
the public libflorid API, MPC implementation, FCI schemas, or pyflorid version.

## Reproducible dependency pair

- Wirelink: `009a5e92432f8942edebd0180dca1a4760c5360c`.
- WLC: `v0.5.0`, `120b9af130753d2ba0d137882916bfe207d3d312`, codegen ABI 30.
- FCI: unchanged `47d5323ae178042b0f9d83965230dabc9fc3fb6a`.
- The mapped operation/status payloads and schema/profile identities are unchanged.
  This is not a migration to managed RPC v2.

The default build still consumes `generated/wirelink/` without WLC or Rust.
Only `LF_ENABLE_WLC=ON` invokes the compiler. Refresh and verify the snapshot:

```sh
cmake -S . -B build/wirelink-abi30-wlc -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_MPC=ON \
  -DLF_ENABLE_WLC=ON -DWLC_EXECUTABLE=/path/to/wlc \
  -DWIRELINK_WLC_AUTO_DOWNLOAD=OFF
cmake --build build/wirelink-abi30-wlc --target lf_update_wirelink --parallel 2
cmake --build build/wirelink-abi30-wlc --parallel 2
cmake --build build/wirelink-abi30-wlc --target lf_check_wirelink
ctest --test-dir build/wirelink-abi30-wlc --output-on-failure
```

## Local acceptance

- Generated build, MPC ON: 9/9 tests passed, including actual MPC solver tests.
- Snapshot build, MPC OFF: 6/6 passed; configure deliberately supplied an invalid
  WLC path to verify that the compiler is never invoked.
- Fresh generated bindings match the checked-in snapshot.
- An initial CTest invocation started before the build finished: two timing
  checks failed and the not-yet-linked MPC example could not run. The complete
  build was then tested serially; the original failure log is retained.

## Actual Willow / H7 communication

The full `firmware/apps/willow` product image, not the simulated HIL app, was
built and flashed on DM-MC02 / STM32H723. J-Link verified the flash contents.
USB serial: `323738373233511200260036`, VID:PID `2fe3:574c`.

Both generated and default-snapshot host builds communicated successfully:
discovery/probe, identity/settings queries during connection, control-lease
setup, 200 state frames, diagnostics, and ten separate open/read/close cycles
with 200 frames each. No motion or motor-register commands were issued.

```sh
build/wirelink-abi30-snapshot/examples/florid_example_00_discover_devices --probe
build/wirelink-abi30-snapshot/examples/florid_example_00_echo_arm_state usb://2fe3:574c/323738373233511200260036
build/wirelink-abi30-snapshot/examples/florid_example_00_read_diagnostics usb://2fe3:574c/323738373233511200260036
```

The persisted custom name remains `simulated-willow`; it is not an image-type
indicator. Diagnostics reported unhealthy motors and CAN TX errors. This proves
communication, not motor operation, real-time performance, or long-run stability.

Local logs: companion Wirelink checkout `build/publish-h7.RhJgRR/`. Firmware
hashes and simulator acceptance are recorded in Ragtime_Firmwares
`firmware/doc/wirelink-abi30-validation.md`.
