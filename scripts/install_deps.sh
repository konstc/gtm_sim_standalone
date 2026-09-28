#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
# Installs the host tools for the Linux (WSL Ubuntu) build.
# g++-9 matches the compiler of the prebuilt GTM libraries (GCC 9.4) and is
# picked automatically by CMake when present; newer GCCs work as well.
set -euo pipefail
sudo apt-get update
sudo apt-get install -y g++-9 gcc-9 cmake ninja-build make gdb gtkwave python3 curl
