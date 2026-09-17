# Milestone 24: Continuous operations and viewport/tools boundary

## Goal

Move continuous authored-edit lifetime and authoritative viewport/editor-view state inward while preserving
existing inspector, viewport, navigation, translation, history, persistence, and renderer behavior.

## Implemented scope

- Added move-only Core `ContinuousEdit` lifetime ownership through `EditOperations`; commit groups typed
  semantic updates into one snapshot-history entry, cancel restores the exact pre-edit Scene, no-op commit
  creates no entry, and abandonment cancels safely.
- Replaced ordinary continuous ImGui transaction ceremony with a retained Core continuous-edit handle spanning
  widget activation through deactivation. Existing typed operations remain the update API.
- Added authoritative in-memory viewport state to Core `Workspace`: view source, scene-camera identity, retained
  Editor View state, interaction mode, translation tool, and reference space.
- Refactored `ViewportView` into an `ai3_scene` controller over Workspace-owned state while retaining view
  resolution, camera fallback, orbit/navigation policy, picking, and display-independent spatial behavior.
- Added the concrete display-independent `TranslationInteractionController`, which owns frozen translation
  gesture semantics and Core continuous-edit lifetime. Dear ImGui now supplies pointer, button, Escape,
  viewport-geometry, and DPI-derived sizing facts.
- Preserved acquisition-time navigation operation freezing, gizmo-first left-click handling, exact cancellation,
  one history entry per completed drag, no-op behavior, and Local/Parent/World/View reference semantics.
- Added direct Core and headless scene/tool coverage for continuous edits, Workspace viewport-state invariants,
  navigation regressions, translation acquisition/update/commit/cancel/failure, and Undo/Redo.

## Preserved boundaries

- Snapshot-based history remains intentional; no command/delta framework was introduced.
- Workspace viewport state is authoritative in memory but is not added to Workspace Document v1 or
  `.ai3workspace` persistence.
- `ai3_scene` continues to depend on `ai3_core`, not `ai3_editor`; Core remains independent of SDL, Dear ImGui,
  EGL/GLES, GLSL, renderer code, windows, and displays.
- Renderer ownership and invalidation remain unchanged. Document/session persistence restructuring, renderer
  restructuring, and final dependency enforcement remain M25, M26, and M27 respectively.
