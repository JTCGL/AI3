#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${AI3_PRESET:-}" ]]; then
    preset="${AI3_PRESET}"
elif [[ -n "${PREFIX:-}" && "${PREFIX}" == *"com.termux"* ]]; then
    preset="termux-clang-debug"
else
    preset="linux-gcc-debug"
fi

printf '[AI3 check] Using preset: %s\n' "${preset}"

cmake --preset "${preset}"
cmake --build --preset "${preset}"
ctest --preset "${preset}"
