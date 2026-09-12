# Underwater protocol development dependency

`underwater-management.patch` contains the complete FCI contract, device
composition, documentation, and endpoint tests required by this SDK branch.
It applies to protocol commit
`4f30689ac5375295c7e03737548f63c63ffdb004`.

The protocol submodule has not been committed or published by this iteration.
A parent SDK commit alone does not preserve its dirty submodule contents. Keep
this patch alongside `generated/wirelink/` until the FCI change is committed
and the SDK submodule and firmware west manifest can pin that published commit.
The firmware carries a byte-identical copy under `firmware/protocol-patches/`.

After initializing the protocol submodule on a clean checkout, run from the
SDK root:

```sh
git -C protocol rev-parse HEAD
git -C protocol apply --check ../protocol-patches/underwater-management.patch
git -C protocol apply ../protocol-patches/underwater-management.patch
```

An already prepared checkout can be checked without mutation:

```sh
git -C protocol apply --reverse --check ../protocol-patches/underwater-management.patch
```

The normal SDK build uses the checked-in generated host snapshot and does not
run WLC. `generated/wirelink/snapshot.txt` records SHA-256 hashes of all schema
and profile inputs, including both new underwater files, plus every generated
artifact. The SDK snapshot test verifies that edits to either underwater input
are detected. Do not bypass this consistency check.

The SDK pins the published **Wirelink v0.7.0-rc.2** runtime at
`e445ee21c9330527af4ab11f8974c02f4d64d3a3` and regenerates with
**WLC v0.7.0-rc.1 / ABI 32**, source commit
`1ec38f103948a0ef7a33d8655a261eed63101350`.

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

Both response DTOs contain required `result` and `info`. Business errors such
as Busy retain a state snapshot and reason; they are not managed RPC transport
rejections. The contract and integration details are in `protocol/README.md`.
