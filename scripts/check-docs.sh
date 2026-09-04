#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repository_root}"

failures=0

fail() {
    printf '[AI3 docs] ERROR: %s\n' "$*" >&2
    failures=$((failures + 1))
}

required_documents=(
    AGENTS.md
    README.md
    docs/AGENTS.md
    docs/MILESTONES.md
    docs/PROJECT.md
    docs/ROADMAP.md
    docs/WORKFLOW.md
    docs/milestones/README.md
)

for document in "${required_documents[@]}"; do
    [[ -f "${document}" ]] || fail "required document is missing: ${document}"
done

mapfile -t markdown_files < <(
    find . -path './.git' -prune -o -type f -name '*.md' -print | sort
)

for document in "${markdown_files[@]}"; do
    while IFS= read -r markdown_link; do
        target="${markdown_link#](}"
        target="${target%)}"
        target="${target#<}"
        target="${target%>}"
        target="${target%% *}"
        target="${target%%#*}"
        target="${target%%\?*}"

        case "${target}" in
            ''|\#*|http://*|https://*|mailto:*|data:*)
                continue
                ;;
        esac

        if [[ "${target}" == /* ]]; then
            resolved="${target}"
        else
            resolved="$(dirname "${document}")/${target}"
        fi
        [[ -e "${resolved}" ]] || fail "broken relative link in ${document}: ${target}"
    done < <(grep -oE '\]\([^)]*\)' "${document}" || true)
done

mapfile -t adr_files < <(find docs/decisions -maxdepth 1 -type f -name '*.md' ! -name 'AGENTS.md' | sort)
for adr in "${adr_files[@]}"; do
    filename="$(basename "${adr}")"
    [[ "${filename}" =~ ^[0-9]{4}-[a-z0-9-]+\.md$ ]] ||
        fail "ADR filename does not follow NNNN-kebab-case.md: ${adr}"
    grep -Eq '^(Status: Accepted|Accepted\.)$' "${adr}" ||
        fail "ADR does not record Accepted status: ${adr}"
done

declare -A milestone_numbers=()
mapfile -t milestone_files < <(
    find docs/milestones -maxdepth 1 -type f -name '[0-9][0-9][0-9][0-9]-*.md' | sort
)
for milestone in "${milestone_files[@]}"; do
    filename="$(basename "${milestone}")"
    number_with_zeroes="${filename%%-*}"
    number="$((10#${number_with_zeroes}))"
    if [[ -n "${milestone_numbers[${number}]:-}" ]]; then
        fail "duplicate milestone brief number ${number}: ${milestone}"
    fi
    milestone_numbers[${number}]="${milestone}"
    grep -Eq "^# Milestone ${number}([[:space:]]|:)" "${milestone}" ||
        fail "milestone heading does not match filename number ${number}: ${milestone}"
    grep -Eq "^## Milestone ${number}[[:space:]]" docs/MILESTONES.md ||
        fail "completed milestone brief has no ledger entry: ${milestone}"
    if grep -Eiq 'remain(s)? required' "${milestone}"; then
        fail "completed milestone brief still says work remains required: ${milestone}"
    fi
done

mapfile -t scene_versions < <(
    sed -n 's/.*format_version = \([0-9][0-9]*\).*/\1/p' src/editor/scene_document.cpp
)
if [[ "${#scene_versions[@]}" -ne 1 ]]; then
    fail 'could not determine exactly one Scene Document format version from scene_document.cpp'
else
    scene_version="${scene_versions[0]}"
    grep -Fq "current version \`${scene_version}\`" docs/PROJECT.md ||
        fail "PROJECT.md does not identify Scene Document version ${scene_version} as current"
    grep -Fq "Version ${scene_version} Scene Documents" docs/ROADMAP.md ||
        fail "ROADMAP.md does not describe established Scene Document version ${scene_version}"
fi

if ((failures > 0)); then
    printf '[AI3 docs] Documentation verification failed with %d error(s).\n' "${failures}" >&2
    exit 1
fi

printf '[AI3 docs] Documentation verification passed.\n'
