#!/usr/bin/env bash
set -euo pipefail

cmake_version="$(cmake --version 2>/dev/null | sed -n '1s/^cmake version //p')"
if [[ ! "${cmake_version}" =~ ^[0-9]+\.[0-9]+([.][0-9]+)?$ ]] ||
    ! printf '3.25\n%s\n' "${cmake_version}" | sort -V -C; then
    printf 'AI3 verification requires CMake >= 3.25; found "%s". Run bash scripts/bootstrap.sh.\n' \
        "${cmake_version:-not installed}" >&2
    exit 1
fi

printf '[AI3 check] Using CMake %s\n' "${cmake_version}"

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
