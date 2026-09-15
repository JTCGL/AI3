# Editor Model Agent Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- `EditorState` is the current compatibility façade for responsibilities that will be extracted incrementally
  into AI3 Core. Do not rename it to `Core` or treat its present mixed ownership as the permanent architecture.
- Keep current display-independent editor code independent of Dear ImGui, SDL, GLES/OpenGL/EGL, GLSL, windows,
  graphics contexts, and display servers.
- Model only behavior required by the current editor milestone; do not introduce ECS, asset, or plugin
  abstractions.
- With current interfaces, every normal authoritative Scene Document mutation must use an `EditorState` path
  that can participate in transactions/history. The approved direction is for typed Core semantic operations
  to own mutation and transaction semantics; frontends must not assemble them from lower-level calls.
- With current interfaces, create scene objects only through `EditorState` lifecycle APIs. IDs are stable,
  monotonic, scene-owned identities. Deleting an object deletes only that object; its direct children become
  roots while preserving world-space pose, and selection is cleared only when the deleted object was selected.
- Object identity, hierarchy, renderable classification, authoritative transforms, and other authored content
  belong to Core Scene. Selection and bounds-display state belong to Core Workspace. Existing co-location in
  `EditorState` is temporary; renderers may consume current state but must not duplicate its authority.
- Core owns undo/redo and semantic transaction behavior. Frontends may report concrete interaction lifecycle
  intent, but new transaction authority must not accumulate in UI code.
- Primitive kind and parameters are authoritative semantic scene data. UI and rendering consume them but
  do not own them.
- Authoritative positions are meters and orientations are `glm::quat`; display-unit and Euler-degree
  presentation must convert without replacing those canonical values.
