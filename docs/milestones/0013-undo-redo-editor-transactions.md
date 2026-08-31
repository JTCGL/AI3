# Milestone 13 — Undo/Redo and editor transaction foundation

## Goal

Introduce display-independent linear undo/redo and intentional editor transaction boundaries before transform
gizmos, while retaining `EditorState` as mutation authority and `DocumentSession` as saved-state/workflow owner.

## Implemented scope

- `EditorHistory` provides begin, commit, cancel, undo, redo, availability, state identity, and rebaseline
  operations without exposing its storage representation.
- Internal before/after snapshots restore exact authoritative Scene Document state: identity, ordering,
  hierarchy, local transforms, names, flags, semantic payloads, allocator state, and name counters. They exclude
  selection and all workspace, session, derived, renderer, and GPU state; history neither serializes JSON nor
  persists in `.ai3scene` files.
- No-op commits create no entry; cancel restores the start; undo followed by editing discards the redo suffix.
- `DocumentSession` compares current and saved history-state identities for dirty behavior while retaining
  monotonic document revisions. Real authoritative changes in an active, not-yet-committed transaction also
  count as dirty, while an unchanged active transaction does not. Save checkpoints the current state. New and
  successful Open rebaseline history; failed Open preserves it. Reset Scene is one undoable edit and retains
  its path.
- Create sphere/camera/light, delete, reparent/unparent, enabled/visible, and Reset Scene use discrete
  transactions. Name, semantic numeric/color fields, and position/rotation/scale controls use ImGui lifecycle
  boundaries so repeated live mutations form one undo step.
- The localized Edit menu exposes enabled Undo/Redo and Ctrl+Z/Ctrl+Shift+Z. Undo/Redo clear the derived sphere
  geometry cache so rendering rebuilds from restored authoritative data.

## Future interaction contract

A future gizmo begins a transaction when its gesture starts, applies ordinary validated `EditorState` mutations
during the live drag, commits on completion, and cancels to restore its starting state. Snapshot storage may
later evolve to focused deltas/coalescing without changing callers; no generalized command framework exists.

## Verification

Headless tests cover grouped and discrete edits, undo/redo/cancel/no-op behavior, redo invalidation, exact scene
and allocator restoration, invalid mutations, monotonic revisions, saved-checkpoint transitions, document
replacement boundaries, failed Open preservation, and workspace exclusion. Run `bash scripts/check.sh`.

## Completion evidence

- Physical runtime verification passed on Termux ARM64.
- Physical runtime verification passed on T5600 Linux x86-64.
- The verified behavior included menu and shortcut access, continuous edit grouping, rendered sphere-geometry
  regeneration, rename and flag edits, hierarchy pose restoration, delete, saved checkpoints, Reset Scene,
  redo invalidation, document history boundaries, and active-edit unsaved-change protection.
- GitHub Actions passed for the final corrected implementation.
