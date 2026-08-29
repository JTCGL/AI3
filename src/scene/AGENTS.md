# Scene Layer Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep scene data, transform/camera math, procedural geometry, and render-target sizing independent of
  ImGui, SDL, and graphics APIs.
- Use GLM as the canonical vector and matrix math library; do not add parallel project-owned math types.
- The editor model remains the owner of object identity and authoritative transforms.
- Add only math and scene behavior required by an implemented editor feature; do not grow this layer into
  an ECS, asset system, or serialization framework.
