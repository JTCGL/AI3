# Documentation Agent Instructions

These instructions extend the repository root AGENTS.md.

Document decisions that future implementers or agents should not have to rediscover. Keep documents concise, factual, and tied to the actual repository state. Clearly distinguish current architecture from possible future work.

- `PROJECT.md` records concise current architectural truth and invariants.
- `decisions/` records durable decisions and rationale.
- `ROADMAP.md` records planned or deferred areas and known boundaries without promising fixed ordering.
- `MILESTONES.md` is a concise historical ledger, not current architectural authority.
- `milestones/` holds approved milestone briefs and acceptance criteria when useful.
- `WORKFLOW.md` records stable development operations, physical checkout paths, collaboration roles, merge ownership, milestone lifecycle, and handoff policy.

When documentation conflicts, verify against repository implementation, tests, and Git history and correct the
documentation. Avoid copying ADR rationale or milestone history into `PROJECT.md`.

For every pull request, classify the effect on each documentation layer in the repository PR template. Update
affected current-truth documentation alongside implementation where practical, and reconcile all affected
layers again after review and runtime corrections. Automated documentation checks enforce mechanical
consistency only; semantic accuracy remains a merge criterion.
