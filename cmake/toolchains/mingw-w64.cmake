# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  Native Windows build with a MinGW-w64 GCC (x86_64, SEH), e.g. WinLibs
#  GCC 14.2 msvcrt. The toolchain is referenced through the environment
#  variable MINGW_DIR (the folder containing bin/g++.exe).
#
#  The bundle's lib-mingw-w64 libraries were built with a MinGW-w64 GCC 9.4
#  (msvcrt, SEH); newer GCCs are compatible (libstdc++ is backward compatible).
###############################################################################

if(NOT "$ENV{MINGW_DIR}" STREQUAL "")
  set(MINGW_DIR "$ENV{MINGW_DIR}" CACHE PATH "MinGW-w64 toolchain root (contains bin/g++.exe)" FORCE)
else()
  set(MINGW_DIR "" CACHE PATH "MinGW-w64 toolchain root (contains bin/g++.exe)")
endif()
file(TO_CMAKE_PATH "${MINGW_DIR}" MINGW_DIR)

if(NOT EXISTS "${MINGW_DIR}/bin/g++.exe")
  message(FATAL_ERROR
    "MinGW-w64 toolchain not found (MINGW_DIR='${MINGW_DIR}').\n"
    "Unpack a MinGW-w64 GCC for x86_64/SEH (e.g. WinLibs, https://winlibs.com/) and set "
    "the environment variable MINGW_DIR to the folder containing bin/g++.exe "
    "(see scripts/env.ps1).")
endif()

set(CMAKE_C_COMPILER   "${MINGW_DIR}/bin/gcc.exe" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${MINGW_DIR}/bin/g++.exe" CACHE FILEPATH "C++ compiler")
set(CMAKE_RC_COMPILER  "${MINGW_DIR}/bin/windres.exe" CACHE FILEPATH "Resource compiler")

if(EXISTS "${MINGW_DIR}/bin/ninja.exe" AND NOT CMAKE_MAKE_PROGRAM)
  set(CMAKE_MAKE_PROGRAM "${MINGW_DIR}/bin/ninja.exe" CACHE FILEPATH "Build program")
endif()
