#requires -Version 7.2
[CmdletBinding()]
param(
    [string]$Python = '',
    [string]$Version = '',
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $IsWindows) { throw 'The desktop EXE must be built on Windows.' }
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$buildRoot = Join-Path $repo 'build/windows-single'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
Start-Transcript -Path (Join-Path $buildRoot 'build.log') -Force | Out-Null
try {
    if (-not $Python) {
        # Resolve the launcher to a real interpreter; avoid Windows Store aliases.
        $Python = & py -3 -c 'import sys; print(sys.executable)'
        if ($LASTEXITCODE -ne 0) { throw 'Pass -Python with the full path to a Python interpreter.' }
    }
    $Python = (Resolve-Path $Python).Path
    $abi = & $Python -I -c 'import struct, sys, sysconfig; assert sys.version_info >= (3, 12), "Python 3.12+ required"; assert struct.calcsize("P") == 8, "x64 Python required"; assert not sysconfig.get_config_var("Py_GIL_DISABLED"), "Use regular CPython, not the free-threaded build"; print(f"cp{sys.version_info.major}{sys.version_info.minor}")'
    if ($LASTEXITCODE -ne 0) { throw 'Unsupported Python interpreter.' }

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $vsPath) { throw 'Install Visual Studio C++ x64 build tools.' }
        & (Join-Path $vsPath 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Host
    }
    foreach ($tool in 'cl.exe', 'cmake.exe', 'ninja.exe', 'git.exe') {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing build tool: $tool" }
    }
    if (-not $VcpkgRoot) { $VcpkgRoot = $env:VCPKG_INSTALLATION_ROOT }
    if (-not $VcpkgRoot -or -not (Test-Path (Join-Path $VcpkgRoot 'scripts/buildsystems/vcpkg.cmake'))) {
        throw 'Pass -VcpkgRoot or set VCPKG_ROOT to a bootstrapped vcpkg checkout.'
    }
    $VcpkgRoot = (Resolve-Path $VcpkgRoot).Path
    $baseline = (Get-Content (Join-Path $repo 'vcpkg.json') -Raw | ConvertFrom-Json).'builtin-baseline'
    & git -C $VcpkgRoot cat-file -e $baseline 2>$null
    if ($LASTEXITCODE -ne 0) {
        & git -C $VcpkgRoot fetch --depth=1 origin $baseline
        if ($LASTEXITCODE -ne 0) { throw 'Could not fetch the pinned vcpkg registry baseline.' }
    }
    & git -C $repo submodule update --init 3rdparty/astrial 3rdparty/wirelink
    if ($LASTEXITCODE -ne 0) { throw 'Submodule checkout failed.' }

    $venv = Join-Path $buildRoot "venv-$abi"
    & $Python -I -m venv $venv
    if ($LASTEXITCODE -ne 0) { throw 'Virtual environment creation failed.' }
    $venvPython = Join-Path $venv 'Scripts/python.exe'
    & $venvPython -I -m pip install --disable-pip-version-check -r (Join-Path $PSScriptRoot 'requirements-windows.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Installing pinned packaging dependencies failed.' }
    $buildArgs = @((Join-Path $PSScriptRoot 'build-single.py'), '--vcpkg-root', $VcpkgRoot)
    if ($Version) { $buildArgs += @('--version', $Version) }
    & $venvPython @buildArgs
    if ($LASTEXITCODE -ne 0) { throw 'Underwater Single packaging or verification failed. See build/windows-single/build.log.' }
}
finally {
    Stop-Transcript | Out-Null
}
