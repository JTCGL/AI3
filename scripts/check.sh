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

bash scripts/format.sh --check
cmake --preset "${preset}"
cmake --build --preset "${preset}"
if [[ -n "${DISPLAY:-}" ]]; then
    ctest --preset "${preset}"
else
    printf '[AI3 check] DISPLAY is unset; running display-independent tests only.\n'
    ctest --preset "${preset}" --exclude-regex ai3_smoke_test
fi
