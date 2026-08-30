# Milestone Briefs

This directory is for approved briefs and acceptance criteria for future milestones. Do not create a brief
until that milestone's scope is locked. A brief should be concise and contain:

- the goal;
- approved scope;
- architectural constraints and invariants relevant to the milestone;
- explicit exclusions;
- acceptance criteria;
- expected automated verification;
- expected manual runtime verification.

During implementation, the active brief defines the approved milestone boundary but does not override
applicable `AGENTS.md` instructions or established architectural decisions. Repository implementation and tests
remain authoritative when documentation conflicts.

Before a milestone is merge-ready, reconcile the brief with the final approved scope and any explicitly agreed
scope corrections introduced during implementation, review, or runtime testing. Reconcile the other affected
documentation layers at the same boundary: lasting current architectural truth belongs in
[`../PROJECT.md`](../PROJECT.md), durable decisions and rationale belong in an ADR, roadmap changes belong in
[`../ROADMAP.md`](../ROADMAP.md), and the concise completed result belongs in
[`../MILESTONES.md`](../MILESTONES.md). Update applicable `AGENTS.md` files only for newly established future
development rules. The detailed brief may remain as historical reference, but it must not supersede current
documentation or the final implementation and tests.
