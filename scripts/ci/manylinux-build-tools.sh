#!/usr/bin/env bash
# SPDX-License-Identifier: ISC
set -euo pipefail

# Runs inside cibuildwheel's manylinux container, not on its Ubuntu host.
dnf install -y libusb1-devel
closeout_installer=$(mktemp /tmp/pyflorid-rustup.XXXXXX.sh)
curl --proto '=https' --tlsv1.2 --fail --silent --show-error \
  https://sh.rustup.rs --output "$closeout_installer"
sh "$closeout_installer" -y --profile minimal --default-toolchain 1.96.1 --no-modify-path
# Retain the installer if a build fails; no compiler is installed on the host.
