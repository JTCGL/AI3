# Milestone 22: Core Workspace extraction

## Goal

Extract the existing non-authored Workspace state into display-independent AI3 Core while preserving current
history, persistence, document-session, and frontend behavior.

## Implemented scope

- Added one authoritative Core `Workspace` owning object selection, per-object bounds-display state, active
  material selection, and display length unit without storing or referencing a `Scene`.
- Kept `EditorState` as the compatibility/coordinator façade owning exactly one `Scene` and one `Workspace`;
  Scene-dependent selection and bounds validation and coordinated object/reset lifecycles remain there.
- Moved Workspace Document v1 serialization and filesystem helpers into Core. The sidecar still persists only
  `showBoundingBox`, `showBoundingSphere`, and `hoverFeedback`, accepts legacy `helperRenderingMode`, and remains
  transactional on malformed input.
- Preserved history's authored Scene snapshots and special deleted-object bounds restoration without making
  selection, active material selection, display units, or ordinary bounds changes undoable.
- Narrowed bounds helper geometry to Core `Scene` and `Workspace`, removed unnecessary editor includes from
  scene code, and removed the `ai3_scene` to `ai3_editor` target dependency.
- Replaced the Workspace Document lambda capture of a structured-binding name with an ordinary named JSON
  reference so the code conforms to C++17.

## Deferred boundaries

Core semantic operations and undo routing remain M23; continuous operations and the intact `ViewportView`
cluster remain M24; document/session and persistence-boundary redesign remains M25; renderer restructuring and
general dependency enforcement remain M26-M27. Selection, active material selection, display units, viewport
state, and other preferences are not added to Workspace Document persistence.
