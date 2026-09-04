# Milestone 18: Editor View navigation foundation

## Goal

Establish the non-authored Editor View vocabulary and conventional transient viewport navigation without
mutating authored Scene Camera objects or adding broader viewport-control frameworks.

## Approved scope

- Rename the Orbit view source to Editor View while retaining the internal `OrbitCamera` implementation.
- Preserve the independent Editor View pose and pivot across Scene Camera switching, fall back to it when an
  active camera is deleted, and restore its default state on Reset Scene.
- Support MMB pan, Shift+MMB orbit, and wheel zoom in Selection and Navigation modes without changing the retained
  mode. Preserve Navigation-mode left-drag orbit.
- Freeze the MMB operation at acquisition, continue it outside the viewport through release, and cancel it when
  the viewport is unavailable or Scene Camera becomes active.
- Give active translation priority over transient navigation, then give acquired transient navigation priority
  over gizmo acquisition, hover, picking, and retained orbit.
- Keep pan policy and view-plane mathematics display-independent. Scale motion from Editor View distance,
  vertical FOV, and logical viewport height, and reject invalid input without mutation.
- Keep every navigation path inert for Scene Camera and preserve selection, document revision, dirty state,
  history, retained mode, transform tool, reference space, and authored camera data.

## Explicit exclusions

Frame Selected/All, navigation buttons, viewport-header redesign, retained-mode redesign, orthographic or
fixed-axis views, multiple viewports, persistent navigation tools, Orbit Selected, Scene Camera manipulation,
walkthrough navigation, Zoom Region, view history, Create Camera From View, delete shortcuts, generalized input
or tool frameworks, renderer abstractions, and unrelated editor work are excluded.

## Verification

Run `bash scripts/check.sh`. Headless coverage includes Editor View source/fallback state, pan direction and
scaling, invalid input, transient dispatch and Shift acquisition freezing, both-mode wheel zoom, Navigation-only
retained orbit, Scene Camera inertness, source-switch preservation, and document/workspace invariants. Build,
format, documentation, and GitHub Actions verification passed. Mouse-dependent physical verification was
unavailable and explicitly deferred by the user; it was accepted as the M18 runtime disposition without claiming
that those interactions passed. The behavior may be checked later without reopening M18 unless a defect is found.

## Completion disposition

Code review, automated verification, final documentation reconciliation, and the completed ledger entry establish
M18 acceptance. Physical mouse verification remains deferred/unavailable by explicit user acceptance and is not
represented as a passing result.
