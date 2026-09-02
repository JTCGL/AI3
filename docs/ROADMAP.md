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

- Extend the established axis-translation gizmo incrementally with planar/free translation, rotation, and
  scale only when their interaction semantics are approved.
- Evolve the established snapshot-backed undo/redo representation to focused deltas/coalescing when edit cost
  requires proportional storage and restoration.
- Add snapping where its interaction and storage semantics are specified.
- Extend selection to multi-selection and define multi-object transform/editing behavior.
- Evaluate future editor preferences for overall gizmo size and view-aligned-axis depth behavior. Candidate
  depth policies include screen-vertical depth motion, disabling/fading strongly aligned axes, and stricter
  direct-axis-only interaction requiring a view change. The current view-derived fallback plane remains fixed.
- Define persistent editor preferences separately before adding configurable key bindings or similar options.

AI3's first transform tool provides single-object X/Y/Z translation in Local, Parent, World, and View spaces.
It establishes frozen gesture-start constraints, one history transaction per drag, and an ImGui overlay with
approximately constant apparent size. It does not define planar/free translation, rotation, scale, snapping,
multi-object pivots, a generalized gizmo framework, or persistent preferences. GLES helper rendering remains a
future fallback only if runtime review finds the overlay inadequate.

The viewport now has explicit Selection and Navigation interaction modes plus a minimal contextual toolbar.
Selection uses display-independent CPU picking for enabled, visible spheres and gives the selected object's
translation handles first refusal on pointer-down. Non-sphere objects selected through the Scene Graph can be
translated. No generalized tool or toolbar framework is established.

## Viewports and cameras

- Add display-independent view-construction modes such as FPS and trackball, while leaving room for other
  modes when requirements justify them.
- Add scene camera types beyond the current perspective camera through the existing explicit camera-kind
  dispatch.
- Add camera/frustum visualization and related scene-camera tooling.

`ViewportView` selection and view/projection resolution are already display-independent and outside Dear
ImGui. Orbit navigation dispatch and sphere picking are also display-independent, while Scene Camera navigation
is intentionally inert. The renderer consumes resolved view values. However, the concrete `ViewportRenderer`,
its GLES render resources, and its offscreen target are still constructed and owned by `EditorUi`; rendering is
not yet independent of UI lifetime/ownership.

## Scene content

- Expand light types, primitive types, and their tooling as concrete requirements appear.
- Add material deletion/duplication, library browsing, textures, multiple slots, or PBR only through later
  requirements that define their ownership and workflow.

The current tagged category/subtype model provides explicit dispatch for sphere, perspective-camera, and
directional-light data. It is not a commitment to a component system, renderer abstraction, or speculative
general-purpose framework.

Reusable document-owned Lambert/Phong materials, sphere assignment with unlit fallback color, and the
linear-RGB/sRGB-boundary contract are established. This is not a generalized asset or shader system.
Current material shading models map to distinct concrete GLES programs; future concrete models should follow
that direction without implying arbitrary shaders or a renderer/backend abstraction.

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
