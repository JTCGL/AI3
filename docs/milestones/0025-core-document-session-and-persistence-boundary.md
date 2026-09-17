# Milestone 25: Core document/session and persistence boundary

## Goal

Move document/session lifecycle authority from the transitional editor layer into Core while preserving the
established single-document workflow, authored dirty semantics, persistence formats, frontend presentation, and
renderer behavior.

## Implemented scope

- Moved `DocumentSession` into `ai3_core`. It non-owningly references `Scene`, `Workspace`, and `EditHistory`, which
  outlive the session, and owns only document/session metadata such as path, clean checkpoints, and pending
  destructive transition state. Construction does not implicitly rebaseline the supplied Core authorities.
- Kept semantic editing separate from document lifecycle. `EditOperations` remains the authority for authored and
  Workspace semantic edits; Reset Scene and interactive bounds-display changes do not become `DocumentSession`
  operations. `DocumentSession` does not depend on `EditOperations` or `EditorState`.
- Preserved authored dirty semantics: history/checkpoint differences and real changes in an active authoritative
  transaction determine dirty state; Workspace-only changes and Workspace-sidecar failures do not.
- Preserved New/Open/Save/Save As and Save/Discard/Cancel behavior while moving their session authority into Core.
  Failed Scene Open is transactional and leaves the existing Scene, Workspace, history, path, checkpoints, and
  dirty relationship unchanged. Failed Save As does not adopt the requested path.
- Treated Scene persistence as authoritative and Workspace persistence as ancillary. A successful Scene Save/Open is
  not rolled back by Workspace-sidecar failure. Missing Workspace sidecars are normal; failed sidecar reads/writes
  are separately reportable.
- Replaced session-to-Console coupling with narrow operation-specific Core persistence results. Diagnostics are
  technical/nonlocalized; localization, Console messages, dialogs, and other presentation remain frontend-owned.
- Added a Workspace-owned document-transition operation. On successful New/Open it clears selection, per-object
  bounds-display state, active material identity, and scene-camera identity; forces Editor View with
  `scene_camera_id == no_object`; and preserves Editor View pose, display-length unit, interaction mode, transform
  tool, and reference space.
- Kept Workspace Document at version 1 with its existing serialized bounds-display scope. After successful Open,
  apply the document-transition policy and then overlay only valid v1 sidecar fields. Discard persisted object-keyed
  entries for objects absent from the loaded Scene.
- Migrated document-session tests to exercise Core directly and added focused regression coverage for the approved
  transition, persistence-result, sidecar, stale-ID, and failed-Save-As semantics.
- Updated `EditorUi`, CMake target ownership, and repository documentation as required while preserving the existing
  graphical document workflow and post-transition renderer-cache behavior.

## Persistence and preference boundary

M25 distinguishes three independent questions: whether state survives an in-memory document transition, whether it
belongs to per-document Workspace persistence, and whether it belongs to future application/user preferences.
Survival across New/Open does not classify a field as an application-wide persisted preference. M25 defines this
boundary but does not expand `.ai3workspace` or introduce preference persistence.

## Preserved boundaries and non-goals

- Scene Document remains version 3 with existing v1/v2 migration behavior; Workspace Document remains version 1.
- No `ai3::Core` aggregate, service locator, generic context, event/message bus, diagnostics framework, or generic
  filesystem abstraction is introduced.
- `EditorState` remains a transitional compatibility/presentation facade; M25 does not require its wholesale
  removal.
- Core remains independent of Dear ImGui, SDL, EGL/GLES/OpenGL, renderer code, windows, dialogs, and displays.
- Native file dialogs, unsaved-change presentation, localization, and Console presentation remain frontend-owned.
- Renderer ownership/synchronization restructuring remains M26. Existing explicit frontend renderer invalidation
  after document transitions remains functional and transitional.
- Workspace Document v2/new persisted viewport fields, application-preference persistence, autosave/recovery,
  recent files, multi-document support, new Close workflow, import/export, assets, and file-browser work are not
  part of M25.
- Final dependency enforcement and graphics-free headless proof remain M27.

## Acceptance criteria

- Core owns `DocumentSession`; `ai3_editor` no longer owns its implementation, and no outward Core dependency is
  introduced.
- `DocumentSession` directly uses only the approved Core authorities and is display-independent/headless-testable.
- Existing dirty/checkpoint, Undo/Redo, active-transaction, New/Open/Save/Save As, and destructive-transition
  semantics remain correct.
- New/Open implement the approved Workspace transition policy; Workspace v1 restoration overlays only its supported
  valid object state and cannot carry stale Scene identities across documents.
- Missing and failed Workspace sidecars are distinguishable, and Workspace failure remains ancillary to successful
  authoritative Scene persistence.
- Core persistence reporting has no frontend localization/Console dependency; the graphical frontend preserves the
  established user-facing workflow.
- Scene Document v3 and Workspace Document v1 serialized scopes remain unchanged.
- Direct display-independent tests cover the migrated session and new M25 invariants; repository-required checks
  pass, followed by normal physical/runtime verification on at least one supported physical system unless the
  repository workflow explicitly requires otherwise.
