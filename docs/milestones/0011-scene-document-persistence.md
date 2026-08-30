# Milestone 11 — Scene Document persistence foundation

## Goal

Establish AI3's first durable, versioned Scene Document format and real Open, Save, and Save As editor
workflow while preserving current scene-model invariants and keeping persistence independent of Dear ImGui,
SDL, GLES, renderer state, and a graphical display.

## Approved scope

- Add a UTF-8 JSON `.ai3scene` document with format identifier `ai3-scene` and version `1`.
- Persist exact object IDs and ordering, names, enabled/visible flags, parent IDs and hierarchy, authoritative
  local position in meters, normalized quaternion orientation in a documented stable component order, scale,
  durable category/subtype names, sphere/perspective-camera/directional-light payloads, the next object-ID
  allocator value, and all current subtype default-name counters.
- Add a narrow `EditorState` reconstruction seam that validates a complete candidate with explicit restored
  identity, hierarchy, allocator state, and naming-counter state without exposing arbitrary ID mutation to
  normal editor operations.
- Parse and validate a complete candidate before atomically replacing the active scene. Reject malformed
  JSON, wrong format or version, duplicate/zero IDs, missing parents, self-parenting/cycles, inconsistent
  category/subtype data, non-finite or invalid transforms and quaternions, invalid semantic payloads, and
  inconsistent allocator or name-counter metadata.
- Reconstruct stored local hierarchy data directly rather than through the world-pose-preserving interactive
  reparent operation.
- Add localized File-menu Open, Save, and Save As operations using SDL3 native file dialogs. Successful Open
  and Save As associate their path with the current document; Save uses that path and delegates to Save As
  when no path is associated.
- On successful Open, clear selection, reset the viewport to default Orbit, clear renderer-derived geometry
  caches, and visibly report success. On failure, preserve the existing scene, viewport, selection, path
  association, and renderer caches, and visibly report the error.
- Keep filesystem and dialog ownership outside `EditorState`; keep serialization and filesystem persistence
  headless-testable.

Scene Documents contain scene content only. They do not contain selection, viewport source/camera/Orbit
state, console data, panel visibility, layout reset or docking state, diagnostics windows, locale, display
length unit, GLES/renderer state, or generated/derived geometry. World transforms and Euler presentation
values are never serialized.

## Architectural constraints and invariants

- Core persistence must not depend on SDL, Dear ImGui, EGL/GLES, renderer state, or a display.
- Existing `EditorState` lifecycle APIs remain the only path for normal object creation and allocation.
- Object IDs remain stable, scene-owned, monotonic, and nonzero; stored allocator and naming metadata must
  continue those semantics after reload, including after deletions.
- Stored transforms remain local-to-parent TRS in meters with normalized `glm::quat` orientation. Quaternion
  component order in version 1 is explicitly stable and documented in the format.
- Durable category/subtype strings, not C++ enum ordinals, represent semantic types.
- Loading is transactional and replaces only Scene Document state. Workspace/session state remains outside
  the document boundary except for the explicit successful-load selection and viewport reset behavior.
- Use a small JSON dependency pinned to an immutable revision through CMake FetchContent; do not create a
  general serialization framework.

## Explicit exclusions

Dirty-state tracking, unsaved-change prompts, autosave, crash recovery, recent files, multiple open documents,
import/export formats, assets or asset references, undo/redo, viewport/workspace persistence, binary formats,
pre-v1 migration, arbitrary forward compatibility with unknown object types, renderer abstraction, and
offscreen/headless GLES rendering are excluded.

## Acceptance criteria

- Saving and loading empty and mixed scenes round-trips every approved field exactly enough to preserve the
  authoritative scene semantics, IDs, ordering, hierarchy, allocator continuity, and naming continuity.
- Unicode names and valid reflected/negative scales round-trip.
- Every required invalid-document class is rejected before destination mutation.
- Successful Open associates the path, resets selection and viewport, and invalidates derived renderer
  geometry; failed Open leaves the current valid document and its associated state unchanged and reports a
  visible error.
- Save writes the associated path; Save on a never-saved document invokes Save As; successful Save As changes
  the association.
- All new user-facing editor strings are present in both current locale resources.

## Automated verification

Headless tests cover empty and mixed semantic round trips; exact IDs/order/hierarchy/local transforms/flags;
Unicode names; semantic payloads; allocator and default-name continuity after deletion; reflected/negative
scale; malformed and invalid documents; duplicate/zero IDs; missing parents; cycles; invalid category/subtype,
transform, quaternion, semantic payload, allocator, and counter metadata; unsupported format version;
transactional failure; and temporary-file filesystem round trips without a display. Run `bash scripts/check.sh`.

## Manual runtime verification

1. Create a mixed hierarchical scene with transforms, semantic parameters, hidden/disabled state, and custom
   names, then Save As an `.ai3scene` file.
2. Modify or Reset Scene, Open the saved file, and confirm hierarchy, properties, and rendering are restored.
3. Confirm selection is cleared and the viewport resets to default Orbit.
4. Create more objects and confirm IDs and default names continue monotonically.
5. Open an invalid document and confirm the valid scene is unchanged and the error is visible.
6. Confirm Save updates the associated path and Save As changes that association.
