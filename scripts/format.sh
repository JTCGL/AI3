#!/usr/bin/env bash
set -euo pipefail

mode="${1:---write}"
mapfile -t sources < <(find src tests -type f \( -name '*.cpp' -o -name '*.h' \) -print | sort)

formatter=""
for candidate in clang-format-21 clang-format; do
    if ! command -v "${candidate}" >/dev/null 2>&1; then
        continue
    fi

    version="$(${candidate} --version 2>/dev/null || true)"
    if [[ "${version}" =~ clang-format[[:space:]]+version[[:space:]]+21([.[:space:]]|$) ]]; then
        formatter="$(command -v "${candidate}")"
        break
    fi
done

if [[ -z "${formatter}" ]]; then
    printf 'AI3 formatting requires clang-format major 21. Run bash scripts/bootstrap.sh to install it.\n' >&2
    exit 1
fi

printf '[AI3 format] Using %s (%s)\n' "${formatter}" "$(${formatter} --version)"

case "${mode}" in
    --write)
        "${formatter}" -i "${sources[@]}"
        ;;
    --check)
        "${formatter}" --dry-run --Werror "${sources[@]}"
        ;;
    *)
        printf 'Usage: %s [--write|--check]\n' "$0" >&2
        exit 2
        ;;
esac
