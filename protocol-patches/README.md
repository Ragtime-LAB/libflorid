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
and profile inputs, including both new underwater files, plus every generated
artifact. The SDK snapshot test verifies that edits to either underwater input
are detected. Do not bypass this consistency check.

The SDK pins the **Wirelink v0.8.0 endpoint-assignment fix** at
`d89ce8fe64efb709cb65b95dc23fcf7cd0c18cc2` and regenerates with
**WLC v0.8.0 / ABI 32**, source commit
`758d12a2466e254cb4d913fce578fb7deae766ec`.

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
`wlc 0.8.0` and `codegen-abi` must report `32`.

Both response DTOs contain required `result` and `info`. Business errors such
as Busy retain a state snapshot and reason; they are not managed RPC transport
rejections. The contract and integration details are in `protocol/README.md`.
