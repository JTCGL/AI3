# Documentation Agent Instructions

These instructions extend the repository root AGENTS.md.

Document decisions that future implementers or agents should not have to rediscover. Keep documents concise, factual, and tied to the actual repository state. Clearly distinguish current architecture from possible future work.

- `PROJECT.md` records concise current architectural truth and invariants.
- `decisions/` records durable decisions and rationale.
- `ROADMAP.md` records planned or deferred areas and known boundaries without promising fixed ordering.
- `MILESTONES.md` is a concise historical ledger, not current architectural authority.
- `milestones/` holds approved milestone briefs and acceptance criteria when useful.

When documentation conflicts, verify against repository implementation, tests, and Git history and correct the
documentation. Avoid copying ADR rationale or milestone history into `PROJECT.md`.
