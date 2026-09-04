# Milestone 18 — Editor View navigation foundation

## Goal

Establish AI3's non-authored Editor View vocabulary and conventional transient viewport navigation without
turning navigation into scene-camera mutation or prematurely implementing the broader viewport-control suite.

## Approved scope

- Rename the non-scene-camera `ViewportView` source from Orbit to Editor View while retaining the existing
  orbit-style implementation internally where useful. Switching to a Scene Camera preserves the Editor View;
  switching back restores it unchanged.
- Middle-button drag temporarily pans the Editor View parallel to its resolved view plane. Pan scale derives
  from distance, vertical field of view, and logical viewport height so it remains proportional to visible
  world scale across viewport sizes and DPI scales.
- Alt+middle-button drag temporarily orbits the Editor View. The gesture chooses pan or orbit at acquisition;
  changing Alt while the button remains down does not reinterpret it.
- Mouse-wheel zoom operates on the Editor View in either retained Selection or Navigation mode.
- Existing Navigation-mode left drag continues to orbit. Transient navigation does not change the retained
  mode, selection, transform tool, reference space, document revision, history, or dirty state.
- Scene Camera navigation remains inert and never mutates the camera object. An active translation gesture has
  priority over transient navigation; otherwise transient navigation gets priority over gizmo acquisition and
  picking.

## Architectural constraints

- Editor View state remains display-independent per-viewport workspace data outside Scene Documents, document
  revision, and history.
- View math and navigation policy belong in the headless scene layer. Dear ImGui translates pointer input and
  owns only the concrete gesture handoff.
- Invalid or non-finite deltas and viewport dimensions fail without changing view state.
- No renderer, backend, generalized input, tool, or viewport framework is introduced.

## Explicit exclusions

Frame Selected/All, viewport navigation buttons, viewport-local header redesign, removal of the retained
Selection/Navigation modes, orthographic or fixed-axis views, multiple viewports, persistent Pan/Orbit/Zoom
tools, Orbit Selected, Scene Camera manipulation, walkthrough, Zoom Region, view undo/redo, Create Camera From
View, and unrelated editing shortcuts remain deferred.

## Acceptance criteria

- MMB pan, Alt+MMB orbit, and wheel zoom behave conventionally in the Editor View without changing retained
  editing state or selection.
- Gesture ownership is frozen at MMB acquisition and continues until release even if Alt changes.
- Scene Camera source remains unchanged by all established editor-navigation paths.
- Switching between Editor View and Scene Camera preserves the complete Editor View pose, including its pivot.
- Existing Selection, Navigation, picking, translation-gizmo, helper-rendering, history, and dirty-state
  behavior remains intact.

## Automated verification

Headless tests cover pan direction/magnitude, viewport-height and distance scaling, invalid input, transient
orbit/pan dispatch, zoom from both retained modes, gesture-policy ownership, Scene Camera immutability, retained
workspace state, and Editor View preservation. Run `bash scripts/check.sh`.

## Manual runtime verification

Verify MMB pan, Alt+MMB orbit, and wheel zoom in both retained modes; confirm no accidental selection or gizmo
movement; change Alt during an acquired MMB gesture; switch between Editor View and a Scene Camera; and repeat
at materially different viewport sizes and UI scales.
