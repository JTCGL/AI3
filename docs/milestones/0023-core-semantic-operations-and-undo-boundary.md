# Milestone 23: Core semantic operations and undo boundary

## Goal

Move semantic editing operations and snapshot-based undo/redo authority into display-independent AI3 Core
without changing persistence formats or observable editing behavior.

## Implemented scope

- Moved the existing snapshot representation into Core `EditHistory`, which depends directly on `Scene` and
  `Workspace` and preserves transaction, Undo/Redo, redo-branch, checkpoint, revision, allocator-restoration,
  cancellation, and no-op semantics.
- Preserved the deletion-specific bounds-display lifecycle snapshot while keeping selection, ordinary bounds
  changes, active material selection, and display units outside authored history.
- Added typed Core `EditOperations` for the existing object, hierarchy, transform, primitive, camera, light,
  material, deletion, reset, selection, and bounds-validation semantics.
- Made discrete authored operations self-transactional. Operations join an existing Core transaction so current
  continuous ImGui controls and the translation gizmo still produce one undo step per gesture.
- Kept `EditorState` as a compatibility/presentation façade that owns one Scene, Workspace, EditHistory, and
  EditOperations; moved cross-domain lifecycle coordination into the Core operation boundary.
- Changed `DocumentSession` to reference the Core history authority while preserving checkpoints, dirty state,
  paths, persistence, and pending document transitions.
- Routed frontend mutations through `EditOperations`, removed discrete frontend transaction ceremony, retained
  renderer invalidation behavior, and added direct graphics-free Core editing tests.

## Deferred boundaries

Continuous-operation and viewport/tool ownership remain M24; full document/session and persistence extraction
remains M25; renderer ownership and invalidation extraction remain M26; final dependency enforcement remains
M27. Snapshot history is retained; no command framework, deltas, event bus, renderer abstraction, or new
persistence format is introduced.
