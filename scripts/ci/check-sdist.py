#!/usr/bin/env python3
"""Build and import a wheel using only an unpacked source distribution."""
# SPDX-License-Identifier: ISC
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import venv


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--work-dir", type=Path, required=True)
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=False)
    source_root = work / "source"
    source_root.mkdir()
    with tarfile.open(args.archive) as archive:
        archive.extractall(source_root, filter="data")
    sources = list(source_root.iterdir())
    if len(sources) != 1 or not sources[0].is_dir():
        raise RuntimeError("Expected one sdist root")
    source = sources[0]
    for required in (
        "CMakeLists.txt",
        "protocol/cmake/FciProtocolWirelink.cmake",
        "3rdparty/astrial/CMakeLists.txt",
        "3rdparty/wirelink/cmake/WirelinkWlcBootstrap.cmake",
    ):
        if not (source / required).is_file():
            raise RuntimeError(f"sdist is missing {required}")
    wheel_dir = work / "wheels"
    environment = os.environ.copy()
    environment["CMAKE_BUILD_PARALLEL_LEVEL"] = "2"
    environment["CMAKE_ARGS"] = f"-DWIRELINK_WLC_CACHE_DIR={work / 'wlc-cache'}"
    subprocess.run(
        [sys.executable, "-m", "pip", "wheel", "--no-deps", str(source),
         "--wheel-dir", str(wheel_dir)],
        cwd=work, env=environment, check=True,
    )
    wheels = list(wheel_dir.glob("*.whl"))
    if len(wheels) != 1:
        raise RuntimeError("Expected one built wheel")
    consumer = work / "consumer"
    venv.EnvBuilder(with_pip=True).create(consumer)
    python = consumer / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    subprocess.run([str(python), "-m", "pip", "install", str(wheels[0])], check=True)
    subprocess.run(
        [str(python), "-I", "-c",
         "import pyflorid; assert hasattr(pyflorid, 'Arm'); print(pyflorid.__file__)"],
        cwd=work, check=True,
    )
    print("sdist -> wheel -> isolated import: PASS")


if __name__ == "__main__":
    main()
