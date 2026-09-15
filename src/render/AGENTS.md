# Rendering Layer Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- This layer is the concrete OpenGL ES 3 rendering implementation; do not introduce a renderer abstraction
  until another real implementation is required.
- Own GLES resources with deterministic teardown and keep GLES details out of editor state and scene math.
- Migrate toward narrow renderer-relevant Core Scene representations or queries. Do not add dependencies on
  history, document-session, panel, dialog, or other frontend semantics; current direct `EditorState` input may
  remain until the renderer-boundary milestone.
- Renderer synchronization and cache invalidation belong below the frontend boundary. Do not move new
  invalidation authority into ImGui code while the existing UI-owned renderer lifecycle remains in place.
- Render editor scenes to the offscreen viewport target. The application framebuffer remains owned by the
  existing Dear ImGui frame path.
- Do not add materials, asset loading, lighting systems, picking, or gizmos without a later milestone.
