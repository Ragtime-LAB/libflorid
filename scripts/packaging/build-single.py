"""Build and verify the Windows Single release using an isolated, pinned environment."""
import argparse
import hashlib
from importlib import metadata
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import tomllib
import zipfile


REPO = Path(__file__).resolve().parents[2]
BUILD = REPO / "build/windows-single"


def run(s_command, *, s_cwd=REPO, s_env=None, s_timeout=1800):
    print("+", subprocess.list2cmdline([str(s_arg) for s_arg in s_command]), flush=True)
    subprocess.run([str(s_arg) for s_arg in s_command], cwd=s_cwd, env=s_env,
                   check=True, timeout=s_timeout)


def git(s_path, *s_args):
    return subprocess.check_output(["git", "-C", str(s_path), *s_args], text=True).strip()


def notices(s_destination, s_installed):
    s_sections = [
        "Underwater Single - third-party notices\n"
        "libflorid declares the ISC license in pyproject.toml.\n"
        f"Application source and build scripts: https://github.com/Ragtime-LAB/libflorid/tree/{git(REPO, 'rev-parse', 'HEAD')}\n"
        "The application bundles CPython, Tcl/Tk, pyserial, pybind11, Astrial, Wirelink,\n"
        "Asio, tl-expected, readerwriterqueue, libusb and the Microsoft C/C++ runtime.\n"
        "PyInstaller and build-tool notices are also included below.\n",
    ]
    s_files = [
        ("CPython", Path(sys.base_prefix) / "LICENSE.txt"),
        ("Astrial", REPO / "3rdparty/astrial/LICENSE"),
        ("Wirelink", REPO / "3rdparty/wirelink/LICENSE"),
        ("Asio", REPO / "3rdparty/astrial/3rdparty/asio/LICENSE_1_0.txt"),
        ("readerwriterqueue", REPO / "3rdparty/astrial/3rdparty/readerwriterqueue/LICENSE.md"),
        ("libusb (LGPL-2.1-or-later)", s_installed / "x64-windows-static-md/share/libusb/copyright"),
    ]
    for s_path in sorted((Path(sys.base_prefix) / "tcl").rglob("license.terms")):
        if "demos" not in s_path.parts:
            s_files.append((f"Tcl/Tk ({s_path.parent.name})", s_path))
    # Tcl/Tk 9 ships its script libraries and license texts in zip archives.
    for s_path in sorted((Path(sys.base_prefix) / "tcl").glob("*.zip")):
        with zipfile.ZipFile(s_path) as s_archive:
            for s_name in s_archive.namelist():
                if s_name.endswith("/license.terms"):
                    s_sections.append(f"Tcl/Tk ({s_path.name})\n" + s_archive.read(s_name).decode("utf-8"))
    s_sections.append("libusb source and checksums\n" +
                      (s_installed / "x64-windows-static-md/share/libusb/vcpkg-spdx-resources.json").read_text(encoding="utf-8"))
    # Some bundled headers retain their license directly in the source header.
    s_expected = REPO / "3rdparty/astrial/3rdparty/tl-expected/tl/expected.hpp"
    s_text = s_expected.read_text(encoding="utf-8")
    s_sections.append("tl-expected\n" + s_text[:s_text.index("#ifndef")])
    for s_name, s_path in s_files:
        s_sections.append(f"{s_name}\n{s_path.read_text(encoding='utf-8', errors='replace')}")
    for s_name in ("pyserial", "pybind11", "pyinstaller", "pyinstaller-hooks-contrib"):
        s_dist = metadata.distribution(s_name)
        for s_file in s_dist.files or []:
            if any(s_part.lower().startswith(("license", "copying")) for s_part in s_file.parts):
                s_path = Path(s_dist.locate_file(s_file))
                if s_path.is_file():
                    s_sections.append(f"{s_name} {s_dist.version} / {s_file.name}\n"
                                      + s_path.read_text(encoding="utf-8", errors="replace"))
    s_destination.write_text("\n\n".join(s_sections), encoding="utf-8")


def verify_executable(s_executable, s_reports, s_env):
    from PyInstaller.archive.readers import CArchiveReader
    import pefile
    s_archive = CArchiveReader(s_executable)
    s_names = {s_name.replace("\\", "/").rsplit("/", 1)[-1].lower() for s_name in s_archive.toc}
    s_required = set()
    for s_name in s_archive.toc:
        if not s_name.lower().endswith((".dll", ".pyd")):
            continue
        with pefile.PE(data=s_archive.extract(s_name), fast_load=True) as s_pe:
            s_pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
            for s_import in getattr(s_pe, "DIRECTORY_ENTRY_IMPORT", []):
                s_dll = s_import.dll.decode("ascii").lower()
                if s_dll.startswith(("msvcp", "vcruntime", "concrt")):
                    s_required.add(s_dll)
    if s_required - s_names:
        raise RuntimeError(f"VC runtime DLLs missing from bundle: {sorted(s_required - s_names)}")
    (s_reports / "bundle.json").write_text(json.dumps({"files": sorted(s_names), "required_vc_runtime": sorted(s_required)}, indent=2), encoding="utf-8")
    # Only copy the EXE. No source tree, Python installation or adjacent DLLs.
    with tempfile.TemporaryDirectory(prefix="Underwater 单臂 smoke ") as s_temp:
        s_directory = Path(s_temp)
        s_copy = s_directory / s_executable.name
        shutil.copy2(s_executable, s_copy)
        s_clean = s_env.copy()
        for s_key in list(s_clean):
            if s_key.upper().startswith(("PYTHON", "TCL", "TK_", "VIRTUAL_ENV", "_PYI", "SINGLE_")):
                del s_clean[s_key]
        s_windows = Path(os.environ["SystemRoot"])
        s_clean["PATH"] = os.pathsep.join(str(s_path) for s_path in (s_windows / "System32", s_windows))
        s_clean["LOCALAPPDATA"] = str(s_directory / "用户数据")
        try:
            for s_action in ("self-test", "smoke"):
                s_report = s_directory / f"{s_action}.json"
                run([s_copy, f"--{s_action}", "--report", s_report],
                    s_cwd=s_directory, s_env=s_clean, s_timeout=60)
                s_result = json.loads(s_report.read_text(encoding="utf-8"))
                if not s_result.get("ok") or not s_result.get("frozen"):
                    raise RuntimeError(f"Frozen {s_action} failed: {s_result}")
                shutil.copy2(s_report, s_reports / s_report.name)
        finally:
            s_log = s_directory / "用户数据/UnderwaterSingle/logs/desktop.log"
            if s_log.is_file():
                shutil.copy2(s_log, s_reports / "desktop.log")
            for s_path in s_directory.glob("*.json"):
                shutil.copy2(s_path, s_reports / s_path.name)


def main():
    s_parser = argparse.ArgumentParser(description=__doc__)
    s_parser.add_argument("--vcpkg-root", required=True, type=Path)
    s_parser.add_argument("--version")
    s_args = s_parser.parse_args()
    if sys.platform != "win32" or platform.machine().upper() not in ("AMD64", "X86_64"):
        s_parser.error("Build on Windows x64 using build-underwater-single.ps1")
    s_commit = git(REPO, "rev-parse", "HEAD")
    s_sdk_version = tomllib.loads((REPO / "pyproject.toml").read_text(encoding="utf-8"))["project"]["version"]
    s_version = s_args.version or f"{s_sdk_version}-dev.{s_commit[:8]}"
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?", s_version):
        s_parser.error("Version must be MAJOR.MINOR.PATCH with an optional prerelease suffix")
    if any(int(s_part) > 65535 for s_part in s_version.split("-")[0].split(".")):
        s_parser.error("Windows numeric version components must be <= 65535")
    BUILD.mkdir(parents=True, exist_ok=True)
    s_run = Path(tempfile.mkdtemp(prefix="run-", dir=BUILD))
    s_wheels = s_run / "wheels"
    s_metadata = s_run / "metadata"
    s_dist = s_run / "dist"
    s_reports = s_run / "reports"
    for s_directory in (s_wheels, s_metadata, s_dist, s_reports):
        s_directory.mkdir()
    s_sdk = BUILD / f"sdk-cp{sys.version_info.major}{sys.version_info.minor}"
    s_installed = s_sdk / "vcpkg_installed"
    s_env = os.environ.copy()
    for s_key in list(s_env):
        if s_key.upper().startswith(("PYTHON", "CMAKE_ARGS", "CMAKE_GENERATOR", "SKBUILD_")):
            del s_env[s_key]
    s_env.update(PYTHONUTF8="1", CMAKE_BUILD_PARALLEL_LEVEL="2")
    s_defines = {
        "CMAKE_TOOLCHAIN_FILE": (s_args.vcpkg_root / "scripts/buildsystems/vcpkg.cmake").as_posix(),
        "VCPKG_TARGET_TRIPLET": "x64-windows-static-md",
        "VCPKG_INSTALLED_DIR": s_installed.as_posix(),
        "BUILD_PYFLORID": "ON", "BUILD_TESTS": "ON", "BUILD_EXAMPLES": "ON",
        "BUILD_SHARED_LIBS": "OFF", "ASTRIAL_IO_URING": "OFF", "LF_ENABLE_WLC": "OFF",
        "WIRELINK_WLC_AUTO_DOWNLOAD": "OFF", "WLC_EXECUTABLE": "C:/wlc-must-not-run.exe",
        "CMAKE_MSVC_RUNTIME_LIBRARY": "MultiThreadedDLL",
        "Python_EXECUTABLE": Path(sys.executable).as_posix(),
    }
    s_settings = [f"--config-settings=cmake.define.{s_key}={s_value}" for s_key, s_value in s_defines.items()]
    run([sys.executable, "-I", "-m", "pip", "wheel", REPO, "--verbose", "--no-deps", "--no-build-isolation",
         "--wheel-dir", s_wheels, f"--config-settings=build-dir={s_sdk.as_posix()}",
         "--config-settings=cmake.build-type=Release", "--config-settings=cmake.args=-GNinja", *s_settings], s_env=s_env)
    s_wheel, = s_wheels.glob("*.whl")
    run([sys.executable, "-I", "-m", "pip", "install", "--no-deps", "--force-reinstall", s_wheel], s_env=s_env)
    run(["ctest", "--test-dir", s_sdk, "-C", "Release", "--output-on-failure"], s_env=s_env)
    s_test_env = s_env | {"PYTHONPATH": os.pathsep.join((str(s_sdk / "pyflorid"), str(REPO / "pyflorid")))}
    run([sys.executable, "-m", "unittest", "discover", "-s", "tools/underwater_console/tests", "-v"], s_env=s_test_env)
    run([sys.executable, REPO / "tools/underwater_single_console/tests/test_console.py",
         s_sdk / "pyflorid", REPO / "pyflorid", "-v"], s_env=s_test_env)
    run([sys.executable, "-m", "unittest", "discover", "-s", "tools/underwater_single_console/tests",
         "-p", "test_desktop.py", "-v"], s_env=s_env)
    run([sys.executable, REPO / "tools/underwater_single_console/run.py", "--smoke",
         "--report", s_reports / "source-smoke.json"], s_env=s_env, s_timeout=60)
    s_info = {
        "version": s_version, "sdk_version": s_sdk_version, "commit": s_commit,
        "dirty": bool(git(REPO, "status", "--porcelain")), "python": platform.python_version(),
        "submodules": {s_name: git(REPO / f"3rdparty/{s_name}", "rev-parse", "HEAD") for s_name in ("astrial", "wirelink")},
        "vcpkg_baseline": json.loads((REPO / "vcpkg.json").read_text())["builtin-baseline"],
        "packages": {s_dist.metadata["Name"]: s_dist.version for s_dist in metadata.distributions()},
    }
    (s_metadata / "build-info.json").write_text(json.dumps(s_info, indent=2), encoding="utf-8")
    notices(s_metadata / "THIRD_PARTY_NOTICES.txt", s_installed)
    s_env["SINGLE_BUILD_METADATA"] = str(s_metadata)
    run([sys.executable, "-I", "-m", "PyInstaller", "--noconfirm", "--clean",
         "--distpath", s_dist, "--workpath", s_run / "pyinstaller",
         REPO / "scripts/packaging/underwater-single.spec"], s_env=s_env)
    s_executable, = s_dist.glob("*.exe")
    verify_executable(s_executable, s_reports, s_env)
    # Promote only verified files. Failed runs keep their diagnostics under run-*.
    s_output = BUILD / "release" / s_version
    s_output.mkdir(parents=True, exist_ok=True)
    for s_path in (s_executable, s_metadata / "THIRD_PARTY_NOTICES.txt", s_metadata / "build-info.json"):
        shutil.copy2(s_path, s_output / s_path.name)
    s_checksums = [f"{hashlib.sha256(s_path.read_bytes()).hexdigest()}  {s_path.name}" for s_path in sorted(s_output.iterdir())
                   if s_path.name != "SHA256SUMS.txt"]
    (s_output / "SHA256SUMS.txt").write_text("\n".join(s_checksums) + "\n", encoding="utf-8")
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as s_file:
            s_file.write(f"release_dir={s_output.as_posix()}\nversion={s_version}\n")
    print(f"Verified Windows release: {s_output}", flush=True)


if __name__ == "__main__":
    main()
