# Scene Layer Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep scene data, transform/camera math, procedural geometry, and render-target sizing independent of
  ImGui, SDL, and graphics APIs.
- Use GLM as the canonical vector and matrix math library; do not add parallel project-owned math types.
- Use the project-wide right-handed Z-up world (+X right, +Y forward, +Z up) and meters for stored
  lengths. Display units are presentation only.
- Store orientations as `glm::quat`. Human-facing Euler values are degrees using intrinsic XYZ and must
  use the centralized scene conversion helpers.
- The editor model remains the owner of object identity and authoritative transforms.
- Procedural primitive meshes are derived from editor-owned semantic parameters and must never become
  authoritative scene data.
- Add only math and scene behavior required by an implemented editor feature; do not grow this layer into
  an ECS, asset system, or serialization framework.
