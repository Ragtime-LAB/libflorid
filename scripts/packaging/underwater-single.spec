"""One-file, windowed application; build inputs are prepared by build-single.py."""
import json
import os
from pathlib import Path
from PyInstaller.utils.win32.versioninfo import (
    VSVersionInfo, FixedFileInfo, StringFileInfo, StringTable, StringStruct, VarFileInfo, VarStruct,
)

s_repo = Path(SPECPATH).resolve().parents[1]
s_metadata = Path(os.environ["SINGLE_BUILD_METADATA"])
s_info = json.loads((s_metadata / "build-info.json").read_text(encoding="utf-8"))
s_version = tuple(int(s_part) for s_part in s_info["version"].split("-")[0].split(".")) + (0,)
s_name = f"UnderwaterSingle-{s_info['version']}-windows-x64"
s_resource = VSVersionInfo(
    ffi=FixedFileInfo(filevers=s_version, prodvers=s_version, mask=0x3f, flags=0, OS=0x40004, fileType=1, subtype=0, date=(0, 0)),
    kids=[StringFileInfo([StringTable("040904B0", [
        StringStruct("CompanyName", "Ragtime"),
        StringStruct("FileDescription", "Underwater Single desktop console"),
        StringStruct("FileVersion", s_info["version"]),
        StringStruct("ProductVersion", s_info["version"]),
        StringStruct("ProductName", "Underwater Single"),
        StringStruct("OriginalFilename", s_name + ".exe"),
    ])]), VarFileInfo([VarStruct("Translation", [0x409, 1200])])],
)
a = Analysis(
    [str(s_repo / "tools/underwater_single_console/run.py")],
    pathex=[str(s_repo / "tools/underwater_single_console")],
    binaries=[],
    datas=[(str(s_metadata / "build-info.json"), "."),
           (str(s_metadata / "THIRD_PARTY_NOTICES.txt"), ".")],
    hiddenimports=["pyflorid._pyflorid", "serial.tools.list_ports_windows"],
    hookspath=[], runtime_hooks=[], excludes=[], noarchive=False,
)
pyz = PYZ(a.pure)
exe = EXE(
    pyz, a.scripts, a.binaries, a.datas, [],
    name=s_name, debug=False, bootloader_ignore_signals=False,
    strip=False, upx=False, console=False, version=s_resource,
    disable_windowed_traceback=False, uac_admin=False,
)
