#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
# Configure, build and (optionally) test the stand-alone GTM simulation.
#
#   scripts/build.sh [preset] [--test] [--clean]
#
# preset: linux-release (default) or linux-debug. Requires GTM_BUNDLE_DIR and
# SYSTEMC_AMS_DIR in the environment (see scripts/env.sh).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset=linux-release
run_tests=0
clean=0
for arg in "$@"; do
    case "$arg" in
        --test) run_tests=1 ;;
        --clean) clean=1 ;;
        -h|--help) sed -n '4,9p' "$0"; exit 0 ;;
        *) preset="$arg" ;;
    esac
done

# shellcheck source=env.sh
. "$root/scripts/env.sh"
cd "$root"

if [ "$clean" = 1 ]; then rm -rf "build/$preset"; fi
cmake --preset "$preset"
cmake --build --preset "$preset"
if [ "$run_tests" = 1 ]; then ctest --preset "$preset"; fi
