# libflorid acados runtime

This directory contains the C runtime used by `BUILD_MPC=ON`: acados with HPIPM
and BLASFEO, built as static, position-independent libraries. Normal builds need
no acados checkout, Python, CasADi, code generator or network access. Configuration
headers and object files are written only to the build directory.

## Provenance and licenses

Upstream files are copied byte for byte from these Git commits:

| Component | Commit | License |
|---|---|---|
| [acados](https://github.com/acados/acados) | `4c23274e49e1304cf3c859d59ea6694ce36305a7` | [BSD-2-Clause](LICENSE) |
| [BLASFEO](https://github.com/giaf/blasfeo) | `d6251233923c9b475fe894fb729fb63ab693e301` | [BSD-2-Clause](external/blasfeo/LICENSE.txt) |
| [HPIPM](https://github.com/giaf/hpipm) | `e3a56c1caddd7f12d125d84f337b9a9e5c186271` | [BSD-2-Clause](external/hpipm/LICENSE.txt) |

`manifest.json` records every upstream file's size and SHA-256, the version pins,
and each backend's compilation sources. `sources.cmake` is generated from that
manifest. `CMakeLists.txt` and this README are libflorid's integration files.
Retain all three licenses with source and binary distributions using this runtime.

The snapshot is approximately **13.64 MB**, including this metadata and build
integration. The limit is **15,000,000 bytes**, counted as the sum of regular-file
lengths, without compression; filesystem allocation, build output and Git history
are excluded. No examples, tests, documentation, Python/MATLAB interfaces,
alternative QP solvers, or upstream build/install machinery are included.
Only double-precision implementations are compiled; acados already uses doubles.
Public headers and source templates needed by those implementations are retained
verbatim, including declarations for APIs outside this build's scope.

## Build selection

The default `BLASFEO_TARGET=GENERIC` uses portable C. It avoids requiring the
build machine's CPU features on end-user machines. `HPIPM_TARGET=GENERIC` is fixed.
These explicit BLASFEO backends are also included:

| `BLASFEO_TARGET` | Target requirement |
|---|---|
| `GENERIC` | C99 compiler; no assembly |
| `X64_INTEL_CORE` | x86-64 with SSE3, GCC/Clang and assembler |
| `X64_INTEL_HASWELL` | x86-64 with AVX2/FMA, GCC/Clang and assembler |
| `ARMV8A_ARM_CORTEX_A53` | ARM64 toolchain with GNU-compatible assembly |
| `ARMV8A_ARM_CORTEX_A57` | ARM64 toolchain with GNU-compatible assembly |
| `ARMV8A_APPLE_M1` | ARM64 toolchain with GNU-compatible assembly |

Select the backend for the **deployment CPU**, for example
`-DBLASFEO_TARGET=X64_INTEL_HASWELL`. There is no runtime dispatch or automatic CPU
probe. Use `GENERIC` for MSVC/clang-cl, universal macOS builds and other targets.
ARM64 builds use upstream's `-march=armv8-a+crc+crypto+simd` setting.
Unsupported targets and optional `ACADOS_WITH_*` features are rejected at configure
time. This snapshot serves libflorid's MPC; it is not a full acados installation or
a general-purpose BLAS/HPIPM distribution. CMake links it via `florid::acados` and
does not change the parent's `BUILD_SHARED_LIBS`, compiler flags or install prefix.

## Verify or update

From the libflorid root, verify the snapshot offline (also run by host CI):

```sh
python3 scripts/vendor_acados.py --check
```

Maintainers importing or updating the runtime need Python 3.12+, Git and CMake
3.21+. Prepare a separate acados checkout at the recorded revision and initialize
only its `external/blasfeo` and `external/hpipm` submodules. Then run:

```sh
python3 scripts/vendor_acados.py --source /path/to/acados
python3 scripts/vendor_acados.py --check
```

The importer reads committed Git objects, ignoring working-tree changes. It
evaluates the pinned upstream CMake source lists, selects double precision, and
adds public headers and recursively included source templates. It refuses an
import exceeding 15 MB. Upgrades require explicitly updating pins in the importer,
reviewing upstream source lists/build settings, and rerunning MPC Release and
ASan/UBSan tests plus the MPC-disabled build. Model code generation uses a separate
full acados checkout; see [MPC regeneration](../../docs/mpc.md#regeneration).
