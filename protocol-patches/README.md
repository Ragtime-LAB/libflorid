# Underwater protocol development dependency

`protocol/` is tracked directly by the SDK repository and already contains
its Underwater contract, device composition, documentation, and endpoint tests.
A clean checkout requires no protocol submodule initialization or patch step.
Edit protocol sources directly and regenerate the Wirelink snapshot when needed.

`underwater-management.patch` is retained as a reference for applying the
Underwater changes to upstream FCI protocol commit
`4f30689ac5375295c7e03737548f63c63ffdb004`. Do not apply it to the SDK's
already prepared `protocol/` directory.

The normal SDK build uses the checked-in generated host snapshot and does not
run WLC. `generated/wirelink/snapshot.txt` records SHA-256 hashes of all schema
and profile inputs, including the underwater and four-motor single-arm files, plus every generated
artifact. The SDK snapshot test verifies that edits to each tracked protocol input
are detected. Do not bypass this consistency check.

The single-arm branch pins **Wirelink v0.7.0-rc.2** at
`e445ee21c9330527af4ab11f8974c02f4d64d3a3`, aligned with the firmware manifest,
and regenerates with **WLC v0.7.0-rc.1 / ABI 32**. The previous SDK pin
`d89ce8f` was a Zephyr adapter endpoint-assignment fix; its Astrial USB host
adapter is unchanged relative to this pin. The firmware separately retains
`patches/wirelink-usb-bulk-const.patch` for its pinned Zephyr adapter.

To regenerate, leave the compiler path unset to download the matching published
host package with SHA-256 verification. Normal snapshot builds do not download
or run WLC:

```sh
cmake -S . -B build/underwater-codegen \
  -DLF_ENABLE_WLC=ON \
  -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/underwater-codegen --target lf_update_wirelink -j2
cmake --build build/underwater-codegen --target lf_check_wirelink -j2
```

For an offline installation, pass `-DWLC_EXECUTABLE=/path/to/wlc`
and `-DWIRELINK_WLC_AUTO_DOWNLOAD=OFF`. Its `--version` must report
`wlc 0.7.0-rc.1` and `codegen-abi` must report `32`.

Dual-arm response DTOs contain required `result` and `info`; SingleResponse
contains `result` and `state`. Business errors such
as Busy retain a state snapshot and reason; they are not managed RPC transport
rejections. The contract and integration details are in `protocol/README.md`.

## Single-arm contract

`underwater-single.patch` is an additive patch against upstream FCI
`732fa3f37db0dab2ee1644d6eca55e1f40cefafd`, already containing the dual-arm
management contract. It adds `single/` and composes its RPC/latest routes into
the device endpoint. The SDK tracked `protocol/` and generated snapshot already
include it: do not apply either patch to this checkout. The firmware repository
carries the identical patch for its west-managed FCI dependency; do not stack
the older management patch on the 732fa3f base.

The new IDs 30209..30212 contain four motor values, not padded six-/ten-axis
messages. See [the single-arm contract](../docs/single.md). After editing these
schemas, regenerate and run the snapshot consistency tests before delivery.
