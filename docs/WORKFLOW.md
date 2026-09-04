# AI3 Development Workflow

This document records stable project-development operations that should not have to be reconstructed from chat
history. Architecture belongs in `PROJECT.md`, future product work in `ROADMAP.md`, durable technical rationale
in ADRs, and milestone history in `MILESTONES.md`.

## Physical development checkouts

- Termux ARM64 on Android: `~/Projects/AI3`
- T5600 Linux x86-64: `~/Documents/Projects/AI3`

To return either checkout to current known-good `main` and run full verification:

```sh
bash scripts/sync-and-check.sh
```

To fast-forward and verify the currently checked-out tracked feature/PR branch without switching branches:

```sh
bash scripts/sync-current-and-check.sh
```

Use the command that matches the intent. Platform-specific instructions should use the checkout paths above and
be presented separately for Termux and T5600 when physical verification is appropriate. Continue to provide
post-merge update instructions for both machines so both checkouts remain synchronized, including when one is
updated remotely or is unavailable for runtime testing. Synchronization and verification instructions must not
automatically launch the graphical executable; provide a separate launch command only when a specific runtime
test requires it.

For milestones requiring physical/runtime review, a passing review on either Termux or T5600 is sufficient for
milestone completion unless an approved milestone explicitly requires platform-specific behavior to be checked
on both. The other machine's runtime review may be deferred without removing it from support or from normal
synchronization. Repository verification and CI requirements remain unchanged.

## Roles

### User

- Approves milestone scope and architectural choices before implementation.
- Performs requested physical/runtime verification on Termux or T5600; either supported machine normally
  satisfies the milestone runtime-review requirement.
- Decides when reviewed work is approved for merge.
- May defer the other physical-platform test when switching machines or when one is unavailable. A deferred
  test is not the same as removing that platform from the supported verification or synchronization workflow.

### ChatGPT

- Inspects the actual repository before milestone planning and review.
- Reassesses roadmap priorities with the user rather than assuming the next milestone.
- Produces the complete Codex implementation prompt only after scope/design approval.
- Reviews the actual feature branch/PR, distinguishing blockers from optional improvements.
- Coordinates focused physical verification and final documentation reconciliation.
- After explicit user approval to merge, performs the GitHub merge directly and reports the resulting known-good
  `main` SHA.
- Provides post-merge synchronization instructions for the physical systems. Do not hand the merge operation back
  to the user unless the user explicitly asks to perform it.

### Codex

- Performs substantial repository implementation on a short-lived feature branch.
- Reads root and applicable directory `AGENTS.md` files and relevant project documentation first.
- Keeps scope limited to the approved milestone.
- Adds appropriate headless tests and updates affected durable documentation.
- Runs repository verification, commits, pushes, and opens or updates the PR.
- Never merges the PR.
- Avoids unnecessary CI polling, particularly on Termux.

## Milestone lifecycle

1. Synchronize relevant physical checkouts to known-good `main`.
2. Inspect actual repository state and documentation.
3. Reassess and discuss candidate milestone scope.
4. Resolve architectural choices with the user.
5. User approves the milestone design.
6. ChatGPT provides the Codex implementation prompt.
7. Codex implements, verifies, documents, pushes, and opens/updates a PR.
8. ChatGPT reviews the actual PR/repository changes.
9. User performs appropriate physical/runtime testing.
10. Review/runtime findings are corrected and re-reviewed.
11. Reconcile final milestone documentation with the reviewed implementation.
12. Confirm final CI/repository verification.
13. User explicitly approves merge.
14. ChatGPT merges the PR.
15. ChatGPT reports the new known-good `main` SHA.
16. Synchronize physical checkouts with `scripts/sync-and-check.sh`.

Green CI alone is not sufficient for merge. Appropriate code review, runtime verification, and documentation
reconciliation remain explicit merge criteria.

## New-conversation handoff policy

Do not paste a reconstruction of the complete project history into each new conversation. The repository is the
durable source of truth.

A normal milestone-planning handoff should contain only:

- repository identity;
- current known-good `main` SHA;
- the milestone just completed, if useful;
- instruction to inspect current main and read root/applicable `AGENTS.md`, `docs/WORKFLOW.md`,
  `docs/PROJECT.md`, `docs/ROADMAP.md`, `docs/MILESTONES.md`, and relevant ADRs/milestone briefs;
- a concise list of newly discussed candidate areas that may not yet be fully represented in the roadmap;
- an explicit instruction not to implement or assume the next milestone before inspection/discussion.

Current architecture should live in `PROJECT.md`; durable rationale in ADRs; future/deferred work in
`ROADMAP.md`; historical completion in `MILESTONES.md`; operational collaboration rules in this file.
If an important rule repeatedly has to be carried in handoff prose, put it in the appropriate repository document
instead.

A typical handoff should be roughly 10-20 lines, not a historical project dump.
