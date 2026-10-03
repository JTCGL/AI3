# Milestone 26: Renderer boundary

## Goal

Establish the concrete GLES renderer as an independently owned architectural boundary with narrow renderer-facing inputs, renderer-owned GPU state and viewport output, and renderer-owned cache synchronization, while preserving existing rendering, editor behavior, Scene semantics, and the single GLES3 implementation.

## Approved scope

- Move concrete `ViewportRenderer` ownership out of `EditorUi` and into application composition. `EditorUi` may continue to invoke rendering and consume renderer output through a non-owning renderer reference, but renderer construction, lifetime, and destruction must no longer depend on UI ownership.
- Preserve deterministic GLES resource teardown while a valid graphics context still exists.
- Establish a concrete `ai3_render` build target for the existing GLES renderer implementation and its GLES resources/programs. The renderer target may depend on the display-independent Core/Scene facilities it actually consumes, but must not depend on `EditorUi`, `ai3_editor`, Dear ImGui, SDL window/input/dialog facilities, or other frontend semantics.
- Keep the renderer input boundary narrow. The concrete renderer may continue to read authoritative `Scene` state directly through renderer-relevant queries and receive resolved view, render-target size, and helper geometry. Do not introduce a duplicate/shadow render scene or generic renderer abstraction without a demonstrated requirement.
- Add non-persisted Scene identity distinct from `DocumentRevision` so renderer-derived caches can distinguish continued evolution of the same authoritative Scene from whole-document replacement.
- Preserve Scene identity across ordinary semantic edits and Undo/Redo. Successful New and successful Open establish a new Scene identity. Failed Open preserves the existing identity. Scene identity is runtime state and is not serialized into Scene Documents.
- Ensure history snapshot capture/restore cannot resurrect an obsolete Scene identity into the authoritative working Scene.
- Move renderer cache synchronization fully below the renderer boundary. Remove frontend calls that explicitly clear renderer geometry caches after New/Open.
- On Scene-identity change, invalidate renderer state whose validity is scoped to the prior Scene. Within one Scene identity, preserve the existing per-object geometry-cache behavior: removed objects release cached geometry, unchanged geometry is reused, and geometry is regenerated only when its geometry-defining inputs change.
- Preserve current rendering semantics for transforms, materials, lights, visibility, Sphere/Box geometry, helper geometry, depth behavior, linear-RGB/sRGB conversion, viewport sizing, and the offscreen viewport target.
- Keep the viewport render target and its GPU resources owned by `ViewportRenderer`. Replace direct raw GLES texture-handle exposure to `EditorUi` with a narrow renderer-owned viewport-output/resource boundary.
- Where Dear ImGui's concrete OpenGL ES backend requires backend-native texture access for presentation, isolate that conversion/access in a narrow concrete graphical presentation seam rather than exposing GLES handles as general UI state or introducing a generic texture/backend abstraction.
- Add focused tests for new display-independent invariants such as Scene identity, document replacement, failed Open, and history restoration. Preserve existing graphical smoke/runtime verification for behavior requiring a real GLES context.
- Update repository architecture documentation, target ownership, and milestone history as required to describe the post-M26 architecture accurately.

## Scene identity and renderer synchronization

`DocumentRevision` remains the authored-change revision and must not become a renderer-wide cache-invalidation counter. Ordinary edits that do not alter primitive geometry must not cause unnecessary geometry regeneration.

Scene identity represents the runtime identity of the authoritative Scene contents across whole-document transitions. It is independent of persisted object IDs and document revision. Two different documents may legitimately contain identical object IDs and identical renderable content while still having different Scene identities.

Renderer caches remain derived GPU state. Frontend/document-session code must not notify the renderer of New/Open or manually synchronize those caches. The renderer determines validity from its renderer-facing inputs, including Scene identity and complete geometry-defining parameters.

## Renderer ownership and output boundary

The application composition root owns the concrete renderer after a valid GLES context has been established and destroys it before that context is destroyed. `EditorUi` receives only non-owning access required to render/present the viewport.

The renderer owns its framebuffer, color texture, depth attachment, shader programs, GPU geometry, and other GLES resources. A renderer-owned viewport-output resource may be consumed by the current graphical presentation path without transferring ownership.

This boundary should leave the concrete renderer usable by a future non-ImGui graphics-capable consumer, but M26 does not implement that consumer.

## Preserved boundaries and non-goals

- OpenGL ES 3 remains the sole renderer/backend. Do not introduce `IRenderer`, a generic renderer abstraction, generic graphics device/context API, Vulkan, desktop OpenGL, GLAD, software rendering, or multi-backend infrastructure.
- Do not introduce a shadow/render-scene copy merely to establish the boundary. Authoritative Scene state remains in Core.
- Do not introduce a generalized event/message bus or frontend-to-renderer invalidation notification system.
- Do not implement offscreen graphics-context creation, EGL context policy, pixel readback, image encoding/output, golden images, or renderer image-regression testing. Those remain future offscreen/headless-rendering work.
- Do not expand Scene Document or Workspace Document formats. Scene identity is not persisted.
- Do not alter material/shader semantics, helper-geometry semantics, primitive tessellation policy, viewport interaction, editor tools, document workflow, or authored dirty semantics.
- Do not pull M27's final dependency-enforcement and graphics-free headless-proof work into M26 except for target/dependency changes directly required to establish the renderer boundary.
- Core remains independent of SDL, Dear ImGui, EGL/GLES/OpenGL, GLSL, windows, contexts, and displays.

## Acceptance criteria

- `ViewportRenderer` is no longer owned by `EditorUi`; application composition owns it with GLES-safe construction/destruction ordering.
- The concrete renderer is represented by an `ai3_render` build target and has no dependency on editor/UI/windowing semantics.
- Renderer inputs contain no aggregate frontend/editor/session/history state and no duplicate authoritative render scene is introduced.
- Scene identity is distinct from `DocumentRevision`, changes on successful whole-document New/Open transitions, survives ordinary edits and Undo/Redo, is unchanged by failed Open, and is not persisted.
- History restoration cannot restore an obsolete Scene identity.
- `EditorUi` no longer explicitly clears or synchronizes renderer geometry caches after document transitions.
- Renderer cache correctness is internal to the renderer. Existing geometry is reused when its complete geometry-defining inputs are unchanged and rebuilt when those inputs change.
- `EditorUi` no longer directly consumes a raw GLES texture integer from `ViewportRenderer`; backend-specific viewport presentation is kept behind the narrow concrete graphical seam required by the current ImGui GLES backend.
- Existing viewport appearance and behavior, materials, lighting, helpers, DPI/render-target sizing, New/Open/Save behavior, Undo/Redo, and editor interaction remain unchanged.
- Scene Document v3 and Workspace Document v1 serialized formats remain unchanged.
- Display-independent tests cover the new Scene-identity/document-transition invariants; repository-required checks pass, followed by normal physical/runtime verification on at least one supported physical system unless the repository workflow explicitly requires otherwise.
