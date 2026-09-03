# Milestone 17: Cached bounds, dual helper rendering, and workspace foundation

## Delivered scope

Sphere objects cache an object-local AABB and bounding sphere derived solely from semantic radius. The cache is
created and reconstructed with objects, refreshed by real radius changes, omitted from Scene Documents, and
unchanged by transforms or unrelated edits. Directional lights and perspective cameras remain unbounded.

Three default-off per-sphere controls select box display, sphere display, and hover feedback. They are stored
by stable object ID in a version-1 `.ai3workspace` JSON sidecar. Missing files and fields are safe defaults;
malformed or unsupported files leave the loaded scene usable and report a Console error. Save and Save As use
atomic replacement and reassociate the sidecar. Sidecars are written only by explicit Save or Save As;
workspace-only changes remain in memory otherwise. Scene-save and workspace-save outcomes are distinct, so a
workspace failure neither misreports nor blocks an otherwise successful scene save. Workspace-only edits are
not authored, undoable, or dirtying.

A narrow headless helper resolver emits world-space colored lines and triangles for transformed wire bounds
and the translation gizmo. Dear ImGui projects them as an always-visible overlay; the concrete GLES3 renderer
can instead draw them after scene geometry against the viewport depth buffer with a centralized visual depth
bias. Depth-tested bounds retain scene occlusion, while the GLES gizmo subpass disables depth testing and
writes so gizmos remain on top. The localized toolbar selects Depth Tested (default) or provisional Overlay
and persists that choice in the workspace sidecar; missing sidecars and missing mode fields use Depth Tested.
Deletion history retains only the removed object's display switches so Undo restores them
without making ordinary workspace edits undoable. World gizmo length is derived from raw axis projection and
corrected against the supplied view/projection so its established DPI-scaled apparent size remains stable. Gizmos render in Selection and
Navigation, while object hover feedback and hit acquisition remain Selection-only. Active gizmo geometry uses
the same frozen view, basis, viewport policy, and apparent length in both presenters while bounds continue to
follow current transforms. Gizmo hit testing remains screen-based.

## Deferred

Camera/local-light bounds, Frame Selected, preferences, workspace-dirty close protection, GPU
picking, additional gizmos/helpers, middle-mouse Navigation, wheel changes, generalized frameworks, renderer
abstractions, and other backends remain deferred. Overlay remains temporarily available despite showing
occluded bounds wires on top; later evaluation may remove its Dear ImGui presentation path.

## Verification

Repository checks and headless tests cover cache behavior, v1/v2 scene reconstruction, workspace
format/lifecycle isolation, transformed helper geometry, and persisted viewport helper mode. Physical runtime
comparison on Termux ARM64 or T5600 Linux x86-64 remains required.
