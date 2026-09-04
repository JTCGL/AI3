# ADR 0005: Display-independent viewport view ownership

## Status

Accepted.

## Decision

Each editor viewport has display-independent `ViewportView` state owned outside Dear ImGui. Its source is
Editor View or Scene Camera; it preserves the persistent Editor View construction state and optionally
identifies one scene-camera object. The application owns the current scene and viewport state and supplies
references to the UI, which presents controls and translates input without becoming their authoritative
owner.

Each viewport also owns an independent interaction mode with the currently required values Selection and
Navigation. View source answers what constructs the view; interaction mode answers what pointer input operates
on. Interaction mode is workspace state and is excluded from Scene Document revision, dirty state, history,
serialization, scene selection, and scene-camera selection. Changing source does not rewrite interaction mode.

Editor View is the non-authored source; its current construction uses an internal orbit camera. Orbit is an
operation on Editor View rather than the source identity. A Perspective Camera remains an ordinary scene object
with its normal identity, hierarchy, local transform, and semantic projection parameters. Camera selection
belongs to a viewport; AI3 has no global active scene camera. Selecting a scene camera retains the independent
Editor View pose and pivot, and switching back restores it unchanged.

Scene Camera is the viewport source category; Perspective Camera is only the currently implemented camera
subtype. The display-independent view layer validates and resolves supported subtypes through an explicit
`CameraKind` dispatch. That dispatch currently accepts only `CameraKind::perspective` and does not add or
imply another camera type.

Resolving a viewport view derives view and projection matrices on demand. Editor View uses its retained pose. A
scene-camera view uses `EditorState`'s authoritative resolved world position and orientation plus the
camera's current vertical FOV and near/far planes and the viewport's current aspect ratio. It does not cache
camera matrices or traverse hierarchy independently. Invalid or currently unsupported scene-camera subtype
selection attempts are rejected. If a selected camera is later deleted or ceases to be a supported scene
camera, resolution clears its ID and deterministically falls back to Editor View.

The concrete GLES3 renderer accepts only the resolved view and projection matrices needed for drawing. It
does not know which view mode constructed them. This is the local seam for adding later view-construction
modes without changing renderer fundamentals; no plugin, renderer abstraction, or additional view mode is
introduced by this decision.

Navigation intent enters through `ViewportView`. Retained left-drag orbit mutates Editor View only in Navigation
mode. Transient MMB pan, Alt+MMB orbit, and wheel zoom operate in either retained mode without changing it.
The MMB operation is frozen at acquisition and continues outside the viewport until release. Pan translates
the retained pivot parallel to the resolved view plane with world-units-per-pixel derived from distance,
vertical FOV, and logical viewport height. Dear ImGui owns the concrete gesture handoff; policy and math remain
display-independent.
Navigation against a Scene Camera is inert because its transform is authoritative object data, not retained
viewport navigation state. Selection uses display-independent CPU picking: normalized viewport coordinates and
the resolved matrices construct a near-to-far world ray, and enabled, visible spheres are tested in their local
space through the inverse authoritative world transform. Hit ordering uses the shared world-ray parameter, so
non-uniform and reflected scale remain exact; non-invertible candidates are ignored safely. This is a narrow
sphere operation, not a collision system or generic geometry interface.

Selection mode also owns single-object axis translation. `ViewportView` retains the active translation tool and
Local/Parent/World/View reference-space selection as non-document workspace state. A visible gizmo handle gets
first refusal on pointer-down; only a miss proceeds to sphere picking. Acquisition freezes the selected object,
resolved reference axis and view, constraint method, viewport dimensions, and DPI-derived sizing so later
workspace changes cannot reinterpret the drag. Scene Camera views use their resolved basis exactly as Editor View
does. The existing Selection/Navigation enum is not a Move/Rotate/Scale tool enum.

The M16 overlay projects the authoritative pivot and reference axes through the current resolved view. Its
display-independent drag policy selects once between a ray/axis closest-point solve and, when near parallel, a
camera-derived plane that contains the axis; plane displacement is projected back onto the axis. This avoids
switching constraints or accumulating frame deltas during a gesture. The choice establishes neither a general
gizmo framework nor a permanent preference policy for view-aligned axes.

## Consequences

Viewport selection, interaction-mode state, Editor View interaction, sphere picking, scene-camera hierarchy
resolution, and projection construction are
headless-testable without SDL, Dear ImGui, GLES, or a display. Dear ImGui remains a presenter/controller and
the renderer remains the single concrete GLES3 implementation. Reset Scene resets both the scene lifecycle
and the viewport to the default Editor View state through their owning application model instances.
