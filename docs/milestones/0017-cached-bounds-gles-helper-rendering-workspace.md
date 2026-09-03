# Milestone 17: Cached bounds, GLES helper rendering, and workspace foundation

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
and the translation gizmo. The concrete GLES3 renderer draws them after scene geometry. Depth-tested bounds
use the viewport depth buffer with a centralized visual depth bias, while the separate gizmo subpass disables
depth testing and writes so gizmos remain on top. Dear ImGui presents only the viewport texture. Version-1
workspace files may contain any string-valued obsolete `helperRenderingMode`, which is ignored; new files omit
the field.
Deletion history retains only the removed object's display switches so Undo restores them
without making ordinary workspace edits undoable. World gizmo length is derived from raw axis projection and
corrected against the supplied view/projection so its established DPI-scaled apparent size remains stable.
Gizmos render in Selection and Navigation, while object hover feedback and hit acquisition remain
Selection-only. Active gizmo geometry uses
the frozen view, basis, viewport policy, and apparent length while bounds continue to
follow current transforms. Gizmo hit testing remains screen-based.

## Deferred

Camera/local-light bounds, Frame Selected, preferences, workspace-dirty close protection, GPU
picking, additional gizmos/helpers, middle-mouse Navigation, wheel changes, generalized frameworks, renderer
abstractions, and other backends remain deferred. An ImGui world-helper presenter must not be reintroduced
without a new concrete requirement.

## Verification

Repository checks and headless tests cover cache behavior, v1/v2 scene reconstruction, workspace
format/lifecycle isolation, transformed helper geometry, and explicit GLES depth policies. The GLES bounds and
always-on-top gizmo presentation passed physical runtime review on T5600 Linux x86-64.
