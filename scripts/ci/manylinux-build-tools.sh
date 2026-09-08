#!/usr/bin/env bash
# SPDX-License-Identifier: ISC
set -euo pipefail

# Runs inside cibuildwheel's manylinux container, not on its Ubuntu host.
dnf install -y libusb1-devel
