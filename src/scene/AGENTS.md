# Scene Layer Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep scene data, transform/camera math, procedural geometry, and render-target sizing independent of
  ImGui, SDL, and graphics APIs.
- Use GLM as the canonical vector and matrix math library; do not add parallel project-owned math types.
- Use the project-wide right-handed Z-up world (+X right, +Y forward, +Z up) and meters for stored
  lengths. Display units are presentation only.
- Store orientations as `glm::quat`. Human-facing Euler values are degrees using intrinsic XYZ and must
  use the centralized scene conversion helpers.
- Core Scene owns object identity, hierarchy, authoritative transforms, primitive semantic parameters,
  materials, lights, cameras, revision, and persisted naming-counter semantics. `EditorState` remains a
  compatibility façade around its one authoritative Core Scene.
- Scene state must not acquire selection, bounds-display state, panels, layout, dialogs, console presentation,
  or other frontend/workspace state.
- Procedural primitive meshes are derived from Core Scene semantic parameters and must never become
  authoritative scene data.
- Add only math and scene behavior required by an implemented editor feature; do not grow this layer into
  an ECS, asset system, or serialization framework.
