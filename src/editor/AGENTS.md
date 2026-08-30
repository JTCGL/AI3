# Editor Model Agent Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep editor state independent of Dear ImGui and graphical platform APIs.
- Model only behavior required by the current editor milestone; do not introduce ECS, persistence,
  undo/redo, asset, or plugin abstractions.
- Create scene objects only through `EditorState` lifecycle APIs. IDs are stable, monotonic,
  scene-owned identities. Deleting an object deletes only that object; its direct children become roots
  while preserving world-space pose, and selection is cleared only when the deleted object was selected.
- Keep object identity, hierarchy, selection, renderable classification, and authoritative transforms here;
  renderers may consume this state but must not duplicate it.
- Primitive kind and parameters are authoritative semantic scene data. UI and rendering consume them but
  do not own them.
- Authoritative positions are meters and orientations are `glm::quat`; display-unit and Euler-degree
  presentation must convert without replacing those canonical values.
