# ADR 0005: Display-independent viewport view ownership

## Status

Accepted.

## Decision

Each editor viewport has display-independent `ViewportView` state owned outside Dear ImGui. It records a
selected view source, preserves the persistent Orbit construction state, and optionally identifies one
Perspective Camera scene object. The application owns the current scene and viewport state and supplies
references to the UI, which presents controls and translates input without becoming their authoritative
owner.

Orbit is one view-construction mode. A Perspective Camera remains an ordinary scene object with its normal
identity, hierarchy, local transform, and semantic projection parameters. Camera selection belongs to a
viewport; AI3 has no global active scene camera. Selecting a scene camera retains the independent Orbit pose,
and switching back restores it unchanged.

Resolving a viewport view derives view and projection matrices on demand. Orbit uses its retained pose. A
scene-camera view uses `EditorState`'s authoritative resolved world position and orientation plus the
camera's current vertical FOV and near/far planes and the viewport's current aspect ratio. It does not cache
camera matrices or traverse hierarchy independently. Invalid selection attempts are rejected. If a selected
camera is later deleted or ceases to be a Perspective Camera, resolution clears its ID and deterministically
falls back to Orbit.

The concrete GLES3 renderer accepts only the resolved view and projection matrices needed for drawing. It
does not know which view mode constructed them. This is the local seam for adding later view-construction
modes without changing renderer fundamentals; no plugin, renderer abstraction, or additional view mode is
introduced by this decision.

## Consequences

Viewport selection, Orbit interaction, scene-camera hierarchy resolution, and projection construction are
headless-testable without SDL, Dear ImGui, GLES, or a display. Dear ImGui remains a presenter/controller and
the renderer remains the single concrete GLES3 implementation. Reset Scene resets both the scene lifecycle
and the viewport to the default Orbit state through their owning application model instances.
