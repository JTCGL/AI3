#!/usr/bin/env bash
set -euo pipefail

print_failure() {
    local stage="$1"
    local status="$2"

    printf '\n========================================\n' >&2
    printf ' AI3 synchronization FAILED\n' >&2
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

printf '[AI3 sync] Fetching and pruning origin.\n'
git -C "${repository_root}" fetch --prune origin || print_failure "fetch origin" "$?"

printf '[AI3 sync] Switching to main.\n'
git -C "${repository_root}" switch main || print_failure "switch to main" "$?"

printf '[AI3 sync] Fast-forwarding main from origin/main.\n'
git -C "${repository_root}" pull --ff-only origin main || print_failure "fast-forward main" "$?"

printf '[AI3 sync] Repository state:\n'
git -C "${repository_root}" status --short --branch || print_failure "status reporting" "$?"
head_sha="$(git -C "${repository_root}" rev-parse HEAD)" || print_failure "SHA reporting" "$?"
printf '%s\n' "${head_sha}"

printf '[AI3 sync] Running repository verification.\n'
(
    cd -- "${repository_root}" || exit 1
    bash scripts/check.sh
) || print_failure "repository verification" "$?"

printf '\n========================================\n'
printf ' AI3 synchronization successful\n'
printf ' Branch: main\n'
printf ' SHA:    %s\n' "${head_sha}"
printf ' Check:  PASSED\n'
printf '========================================\n'
