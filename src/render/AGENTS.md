# Rendering Layer Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- This layer is the concrete OpenGL ES 3 rendering implementation; do not introduce a renderer abstraction
  until another real implementation is required.
- Own GLES resources with deterministic teardown and keep GLES details out of editor state and scene math.
- Render editor scenes to the offscreen viewport target. The application framebuffer remains owned by the
  existing Dear ImGui frame path.
- Do not add materials, asset loading, lighting systems, picking, or gizmos without a later milestone.
