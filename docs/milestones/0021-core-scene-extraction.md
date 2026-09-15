# Milestone 21: Core Scene extraction

## Goal

Extract authored Scene and Scene Document ownership from `EditorState` into display-independent AI3 Core
without changing user-visible behavior or the Scene Document format.

## Implemented scope

- Added the real `ai3_core` target containing `Scene`, Scene Document persistence, and color validation.
- Made `Scene` the sole owner of authored objects, materials, IDs and counters, hierarchy/transforms, semantic
  payloads, derived bounds, naming state, queries, validation, and document revision.
- Kept one `Scene` inside `EditorState`; retained authored APIs forward to it while selection, bounds-display,
  panels, Console presentation, and layout-reset state remain outside `Scene`.
- Adapted history and `DocumentSession` without redesigning their architecture.
- Moved `scene_document.h/.cpp` into `src/core` and changed the codec to operate directly on `Scene` while
  preserving version 3 output, strict v1/v2 migration, validation, identity restoration, and revision behavior.
- Changed pure authored consumers in scene math, viewport camera resolution, primitive picking, and the concrete
  renderer to consume `Scene`. Higher-level helper geometry and continuous translation remain transitional
  `EditorState` consumers where they also need Workspace or history behavior.

## Deferred boundaries

Workspace extraction, semantic-operation routing, continuous-operation/tool restructuring, document/session
redesign, final renderer restructuring, and final dependency enforcement remain assigned to M22-M27.
