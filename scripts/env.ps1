# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
# Dot-source this file in PowerShell:  . .\scripts\env.ps1
# Checks the environment variables that point the Windows build at the
# external (read-only) inputs:
#   GTM_BUNDLE_DIR   Bosch GTM SystemC model bundle (unpacked folder)
#   SYSTEMC_AMS_DIR  SystemC-AMS proof-of-concept (unpacked source tree)
#   MINGW_DIR        MinGW-w64 GCC toolchain (folder containing bin\g++.exe)
# Set them once, e.g. persistently for the current user:
#   [Environment]::SetEnvironmentVariable('GTM_BUNDLE_DIR', 'C:\tools\coside-<version>-Bosch_GTM-bundle-<date>', 'User')
#   [Environment]::SetEnvironmentVariable('SYSTEMC_AMS_DIR', 'C:\tools\systemc-ams-<version>', 'User')
#   [Environment]::SetEnvironmentVariable('MINGW_DIR', 'C:\tools\winlibs\mingw64', 'User')
# MINGW_DIR\bin is appended to PATH (ninja, gdb, ...). It is appended rather
# than prepended because WinLibs also ships a cmake.exe, whose downloads fail
# (no CA certificates); the system CMake must be found first. The compilers
# themselves are selected by full path in cmake/toolchains/mingw-w64.cmake.
$problems = @()
if (-not $env:GTM_BUNDLE_DIR -or -not (Test-Path (Join-Path $env:GTM_BUNDLE_DIR 'cos_gtm_lib'))) {
    $problems += "GTM_BUNDLE_DIR='$env:GTM_BUNDLE_DIR' is not the GTM bundle folder (cos_gtm_lib\ not found)."
}
if (-not $env:SYSTEMC_AMS_DIR -or -not (Test-Path (Join-Path $env:SYSTEMC_AMS_DIR 'src\systemc-ams.h'))) {
    $problems += "SYSTEMC_AMS_DIR='$env:SYSTEMC_AMS_DIR' is not a SystemC-AMS source tree (src\systemc-ams.h not found)."
}
if (-not $env:MINGW_DIR -or -not (Test-Path (Join-Path $env:MINGW_DIR 'bin\g++.exe'))) {
    $problems += ("MINGW_DIR='$env:MINGW_DIR' does not contain bin\g++.exe. Set it to a MinGW-w64 GCC for x86_64 " +
                  "with SEH exceptions (e.g. WinLibs, https://winlibs.com/).")
}
if ($problems) { throw ($problems -join "`n") }

$mingwBin = Join-Path $env:MINGW_DIR 'bin'
if (($env:PATH -split ';') -notcontains $mingwBin) { $env:PATH = "$env:PATH;$mingwBin" }

Write-Host "GTM_BUNDLE_DIR=$env:GTM_BUNDLE_DIR"
Write-Host "SYSTEMC_AMS_DIR=$env:SYSTEMC_AMS_DIR"
Write-Host "MINGW_DIR=$env:MINGW_DIR"
