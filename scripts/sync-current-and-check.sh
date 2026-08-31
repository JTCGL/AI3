#!/usr/bin/env bash
set -euo pipefail

print_failure() {
    local stage="$1"
    local status="$2"

    printf '\n========================================\n' >&2
    printf ' AI3 current-branch synchronization FAILED\n' >&2
    printf ' Stage: %s\n' "${stage}" >&2
    printf '========================================\n' >&2
    exit "${status}"
}

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" ||
    print_failure "repository validation" 1
expected_root="$(cd -- "${script_dir}/.." && pwd -P)" ||
    print_failure "repository validation" 1

repository_root="$(git -C "${expected_root}" rev-parse --show-toplevel 2>/dev/null)" || {
    printf 'This script must be run from within the AI3 Git checkout.\n' >&2
    print_failure "repository validation" 1
}
repository_root="$(cd -- "${repository_root}" && pwd -P)" ||
    print_failure "repository validation" 1

if [[ "${repository_root}" != "${expected_root}" || ! -f "${repository_root}/scripts/check.sh" ]]; then
    printf 'The script location does not match the root of the intended AI3 Git checkout.\n' >&2
    print_failure "repository validation" 1
fi

worktree_status="$(git -C "${repository_root}" status --porcelain --untracked-files=all)" ||
    print_failure "working-tree inspection" 1
if [[ -n "${worktree_status}" ]]; then
    printf 'The AI3 working tree is not clean. Resolve local modifications and untracked files before synchronizing.\n' >&2
    printf '%s\n' "${worktree_status}" >&2
    print_failure "clean working-tree check" 1
fi

current_branch="$(git -C "${repository_root}" symbolic-ref --quiet --short HEAD)" || {
    printf 'The AI3 checkout is in detached HEAD state. Switch to a tracked branch before synchronizing.\n' >&2
    print_failure "current branch detection" 1
}

upstream_ref="$(git -C "${repository_root}" rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null)" || {
    printf 'Branch %s has no configured upstream. Configure tracking before synchronizing.\n' \
        "${current_branch}" >&2
    print_failure "upstream detection" 1
}

printf '[AI3 current sync] Branch: %s\n' "${current_branch}"
printf '[AI3 current sync] Upstream: %s\n' "${upstream_ref}"
printf '[AI3 current sync] Fetching and pruning origin.\n'
git -C "${repository_root}" fetch --prune origin || print_failure "fetch origin" "$?"

printf '[AI3 current sync] Fast-forwarding %s from its configured upstream.\n' "${current_branch}"
git -C "${repository_root}" pull --ff-only || print_failure "fast-forward current branch" "$?"

result_branch="$(git -C "${repository_root}" symbolic-ref --quiet --short HEAD)" ||
    print_failure "resulting branch validation" 1
if [[ "${result_branch}" != "${current_branch}" ]]; then
    printf 'The checked-out branch changed unexpectedly from %s to %s.\n' \
        "${current_branch}" "${result_branch}" >&2
    print_failure "branch preservation" 1
fi

head_sha="$(git -C "${repository_root}" rev-parse HEAD)" || print_failure "SHA reporting" "$?"
printf '[AI3 current sync] Current branch: %s\n' "${current_branch}"
printf '[AI3 current sync] Resulting HEAD: %s\n' "${head_sha}"

printf '[AI3 current sync] Running repository verification.\n'
(
    cd -- "${repository_root}" || exit 1
    bash scripts/check.sh
) || print_failure "repository verification" "$?"

printf '\n========================================\n'
printf ' AI3 current-branch synchronization successful\n'
printf ' Branch: %s\n' "${current_branch}"
printf ' SHA:    %s\n' "${head_sha}"
printf ' Check:  PASSED\n'
printf '========================================\n'
