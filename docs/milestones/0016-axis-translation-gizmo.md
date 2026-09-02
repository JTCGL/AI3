# Milestone 16 — Single-object axis translation gizmo

## Goal

Establish AI3's first direct viewport transform tool: constrained X/Y/Z translation of the currently selected
object, without generalizing the viewport tool, toolbar, rendering, or input architecture.

## Implemented scope

- Selection mode presents a localized, DPI-aware translation tool and Local/Parent/World/View selector. Any
  selected transformable scene object is eligible; existing viewport picking remains sphere-specific. A gizmo
  handle gets pointer-down ownership before ordinary selection, while Navigation remains effective only for an
  Orbit view.
- Display-independent logic resolves the chosen reference-space axis, normalizes projected directions to a
  DPI-derived screen-space length, hit-tests a distinct invisible tolerance, validates the frozen viewport
  coordinate frame, and constrains pointer rays. The overlay itself is drawn through Dear ImGui rather than
  duplicated as GLES helper geometry. Collapsed/view-aligned projected axes are omitted safely.
- Acquisition freezes the object, axis/basis, starting world pivot, resolved view, viewport dimensions,
  DPI-derived size, and constraint method. Every live result is derived from that gesture-start state rather
  than accumulated frame deltas. Changing workspace controls cannot reinterpret an active drag, and the frozen
  pixel length does not rescale as the pivot changes depth. A material viewport-origin or size change
  cancels/restores the gesture rather than combining frozen dimensions with a different coordinate origin.
- The normal constraint is the closest-point parameter between pointer ray and world axis. When the two are
  near parallel at acquisition, the fixed fallback selects the better-conditioned of camera-right- and
  camera-up-derived planes containing the axis, intersects pointer rays with that plane, and projects motion
  back onto the axis. Exact degeneracies fail safely; the method never switches during a drag.
- `EditorState::set_world_position` converts a desired child pivot through the inverse authoritative parent
  world transform and changes only its local position. Root world position maps directly to local position.
  Local orientation, local scale, and hierarchy remain exact; non-finite input and non-invertible parents fail
  transactionally without approximate decomposition.
- One acquired drag owns one existing `EditorHistory` transaction. Live real movement is dirty, release commits
  one entry, Undo/Redo restores the complete state, a no-op commits no entry, and Escape or unsafe mutation
  cancels and restores the gesture start. Tool, reference-space, selection, view, and gesture state remain
  workspace data outside revision, history, dirty state, and Scene Documents.

## Verification

`bash scripts/check.sh` builds the graphical application and runs all tests. Headless regression coverage
includes root and translated/rotated/uniform/non-uniform/reflected parent placement, singular and invalid
failure, exact preserved local orientation/scale/hierarchy, semantic no-ops, all four established bases,
normal/fallback/degenerate constraints, gesture-start determinism, near/far and frozen-depth screen sizing,
collapsed projection, hit testing, viewport-geometry invalidation,
single-transaction live edits, cancellation, Undo/Redo/checkpoint dirty behavior, and workspace exclusions.

Physical review remains required on Termux ARM64 and T5600 Linux x86-64 for overlay appearance, DPI behavior,
constant and gesture-frozen apparent size, handle-first pointer ownership, near-view-aligned stability, all
reference spaces and hierarchy cases, Scene Camera basis behavior, history/dirty behavior, and unchanged
Selection/Navigation behavior.

## Deferred

Planar/free translation, rotation, scale, snapping, multi-selection and multi-object transforms, generalized
gizmo/tool/toolbar/input frameworks, GLES helper rendering, additional picking scope, highlighting/bounds,
Scene Document changes, and persistent preferences remain deferred. Future preference candidates include
overall gizmo size; screen-vertical depth motion, aligned-axis fading/disabling, or strict view-change behavior
as alternatives to the fallback plane; configurable key bindings; and similar editor preferences. No settings
system or preference persistence is introduced here.
