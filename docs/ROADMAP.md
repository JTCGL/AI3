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
  Known candidates include gizmo size, view-aligned-axis constraint policy, selected/hovered bounds colors,
  the Frame Selected key (initially F), Navigation-mode hover feedback, and navigation/input preferences.

AI3's first transform tool provides single-object X/Y/Z translation in Local, Parent, World, and View spaces.
It establishes frozen gesture-start constraints, one history transaction per drag, shared helper inputs, and
overlay/depth-tested presentations with approximately constant apparent size. It does not define planar/free
translation, rotation, scale, snapping,
multi-object pivots, a generalized gizmo framework, or persistent preferences.

Future gizmos and non-rendered editor objects should share narrowly defined helper-geometry inputs such as
pivot, axes, colors, and apparent scale across the overlay and depth-tested GLES presenters. This common path
must be grown from concrete consumers rather than becoming a speculative generalized gizmo framework. Expected
consumers include transform gizmos; local-light range, radius, and direction indicators; helper/dummy-object
geometry; camera view lines and planes; planar spacing/layout grids; and normal, binormal, and tangent
visualization for points and geometric data.

The viewport now has explicit Selection and Navigation interaction modes plus a minimal contextual toolbar.
Selection uses display-independent CPU picking for enabled, visible spheres and gives the selected object's
translation handles first refusal on pointer-down. Non-sphere objects selected through the Scene Graph can be
translated. Future input behavior should make middle-mouse hold a momentary Navigation override: pressing the
button enters Navigation and releasing it returns to Selection, while wheel scrolling alone does not change
mode. Orbit wheel zoom should remain available regardless of the current Selection/Navigation mode; Scene
Camera data must remain unaffected. The toolbar's retained/effective-mode presentation and interactions with
its existing mode controls require explicit design. No generalized tool or toolbar framework is established.

## Bounds and viewport feedback

- Extend the established cached local AABB/sphere and dual wire presenters to future bounded object types only
  when their authoritative finite-shape semantics are defined. Intersection consumers remain free to choose
  cached bounds, exact shapes, or staged tests.
- Keep directional lights unbounded and without fabricated bounds. Future local lights should derive finite
  bounds from concrete range/radius parameters and participate in the same optional bounds visualization.
- Give perspective-camera objects a finite bounding volume derived from their frustum corners, projection
  parameters, world transform, and aspect policy. The envelope extends along the viewing direction through the
  far clip plane; whether it begins at the camera origin or near plane, and which aspect ratio is authoritative
  outside an active viewport, remain milestone design decisions.
- Add Frame Selected, initially bound to F, after the required bounds semantics exist. In Orbit view it should
  target the selected bounds center and choose a perspective distance satisfying both horizontal and vertical
  FOV constraints. It must not mutate a Scene Camera. Behavior while a Scene Camera source is active must be
  approved explicitly.
- Keep screen-space bounds as a possible later feature rather than conflating them with object/world-space
  bounds.

## Viewports and cameras

- Add display-independent view-construction modes such as FPS and trackball, while leaving room for other
  modes when requirements justify them.
- Add scene camera types beyond the current perspective camera through the existing explicit camera-kind
  dispatch.
- Add camera/frustum visualization and related scene-camera tooling; coordinate its geometry with the
  perspective-camera bounds/frustum envelope rather than maintaining conflicting calculations.

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
