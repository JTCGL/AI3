# Milestone 17: Cached bounds, dual helper rendering, and workspace foundation

## Delivered scope

Sphere objects cache an object-local AABB and bounding sphere derived solely from semantic radius. The cache is
created and reconstructed with objects, refreshed by real radius changes, omitted from Scene Documents, and
unchanged by transforms or unrelated edits. Directional lights and perspective cameras remain unbounded.

Three default-off per-sphere controls select box display, sphere display, and hover feedback. They are stored
by stable object ID in a version-1 `.ai3workspace` JSON sidecar. Missing files and fields are safe defaults;
malformed or unsupported files leave the loaded scene usable and report a Console error. Save and Save As use
atomic replacement and reassociate the sidecar. Workspace state is not authored, undoable, or dirtying.

A narrow headless helper resolver emits world-space colored lines and triangles for transformed wire bounds
and the translation gizmo. Dear ImGui projects them as an always-visible overlay; the concrete GLES3 renderer
can instead draw them after scene geometry against the viewport depth buffer with a centralized visual depth
bias. The localized toolbar selects Overlay (default) or Depth Tested without persisting that choice. Gizmos
render in Selection and Navigation, while hit acquisition remains Selection-only. Depth-tested hit testing is
still screen-based, so an occluded handle may be acquired.

## Deferred

Camera/local-light bounds, Frame Selected, preferences, persistence of helper-rendering mode, depth-aware or GPU
picking, additional gizmos/helpers, middle-mouse Navigation, wheel changes, generalized frameworks, renderer
abstractions, and other backends remain deferred.

## Verification

Repository checks and headless tests cover cache behavior, v1/v2 scene reconstruction, workspace
format/lifecycle isolation, transformed helper geometry, and transient viewport helper mode. Physical runtime
comparison on Termux ARM64 or T5600 Linux x86-64 remains required.
