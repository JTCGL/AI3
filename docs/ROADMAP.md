# AI3 Roadmap

This is a record of known future areas and architectural boundaries, not a commitment to a fixed sequence or
a promise that every item will ship unchanged. Milestone planning sets approved scope. Ordering may change as
requirements and demonstrated dependencies become clearer.

## Future document workflow

- Define autosave, crash recovery, and recent-file behavior as separate workflow milestones.
- Add multi-document, import/export, assets, and format migration only when their own requirements establish
  the necessary ownership and compatibility policies.
- Design an AI3-owned cross-platform file dialog/browser so the editor can eventually provide consistent file
  workflow UX instead of permanently depending on host-native dialogs.
- If configurable internal/document units are introduced, make them authoritative Scene Document data whose
  changes dirty the document, with an explicit format and conversion policy decision first.

Version 1 Scene Documents and revision-based single-document New/Open/Save/Save As with unsaved-change
protection are established. Scene files intentionally do not include viewport, layout, locale, display-unit,
console, diagnostics, renderer, or other workspace/session state.

## Transform tools and editing workflow

- Build transform gizmos incrementally: axis translation, planar/free translation, rotation, and scale.
- Evolve the established snapshot-backed undo/redo representation to focused deltas/coalescing when edit cost
  requires proportional storage and restoration.
- Add snapping where its interaction and storage semantics are specified.
- Apply the established gesture transaction contract to transform gizmos: begin at gesture start, perform live
  authoritative mutations, commit on completion, and cancel to restore the start.
- Extend selection to multi-selection and define multi-object transform/editing behavior.

AI3 already distinguishes Local, Parent, World, and View reference spaces and centrally resolves world
transforms. Those semantics are inputs to future tools; they do not by themselves define gizmo interaction,
snapping, multi-object pivots, or transaction policy.

## Viewports and cameras

- Add display-independent view-construction modes such as FPS and trackball, while leaving room for other
  modes when requirements justify them.
- Add scene camera types beyond the current perspective camera through the existing explicit camera-kind
  dispatch.
- Add camera/frustum visualization and related scene-camera tooling.

`ViewportView` selection and view/projection resolution are already display-independent and outside Dear
ImGui. The renderer consumes resolved view values. However, the concrete `ViewportRenderer`, its GLES render
resources, and its offscreen target are still constructed and owned by `EditorUi`; rendering is not yet
independent of UI lifetime/ownership.

## Scene content

- Expand light types, primitive types, and their tooling as concrete requirements appear.
- Add material deletion/duplication, library browsing, textures, multiple slots, or PBR only through later
  requirements that define their ownership and workflow.

The current tagged category/subtype model provides explicit dispatch for sphere, perspective-camera, and
directional-light data. It is not a commitment to a component system, renderer abstraction, or speculative
general-purpose framework.

Reusable document-owned Lambert/Phong materials, sphere assignment with unlit fallback color, and the
linear-RGB/sRGB-boundary contract are established. This is not a generalized asset or shader system.

## Offscreen and headless rendering

Enable rendering through the real GLES renderer without Dear ImGui or a visible editor window. This area is
expected to require deliberately separating concerns that are currently coupled:

- creation and ownership of a graphics context and offscreen surface;
- renderer construction and rendering independent of `EditorUi` ownership;
- pixel readback from the render target;
- image output;
- eventual renderer regression tests based on produced images.

The existing `headless-debug` configuration is intentionally graphics-free and tests core/domain behavior.
“Headless rendering” is a different future capability: it still needs a real graphics context and the concrete
renderer, merely without Dear ImGui or a visible editor window. No context strategy, image format, comparison
method, or new renderer abstraction is selected here.
