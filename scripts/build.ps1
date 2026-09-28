# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
# Configure, build and (optionally) test the stand-alone GTM simulation on
# Windows (MinGW-w64).
#
#   .\scripts\build.ps1 [-Preset windows-release|windows-debug] [-Test] [-Clean]
param(
    [string]$Preset = 'windows-release',
    [switch]$Test,
    [switch]$Clean
)
# No $ErrorActionPreference = 'Stop': Windows PowerShell turns stderr output of
# native tools into errors when redirected; exit codes are checked instead.
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'env.ps1')

Push-Location $root
try {
    if ($Clean -and (Test-Path "build\$Preset")) { Remove-Item -Recurse -Force "build\$Preset" }
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    if ($Test) {
        ctest --preset $Preset
        if ($LASTEXITCODE -ne 0) { throw "tests failed" }
    }
}
finally {
    Pop-Location
}
