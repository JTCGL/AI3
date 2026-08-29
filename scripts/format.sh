#!/usr/bin/env bash
set -euo pipefail

mode="${1:---write}"
mapfile -t sources < <(find src tests -type f \( -name '*.cpp' -o -name '*.h' \) -print | sort)

case "${mode}" in
    --write)
        clang-format -i "${sources[@]}"
        ;;
    --check)
        clang-format --dry-run --Werror "${sources[@]}"
        ;;
    *)
        printf 'Usage: %s [--write|--check]\n' "$0" >&2
        exit 2
        ;;
esac
