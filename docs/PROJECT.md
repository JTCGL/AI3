# AI3 Phase-One Contract

## Supported targets

| Environment | Architecture | Compiler | Graphics |
| --- | --- | --- | --- |
| Termux | ARM64 | Clang | OpenGL ES 3.0 |
| Desktop Linux | x86-64 | GCC | OpenGL ES 3.0 |
| GitHub Actions | x86-64 Ubuntu | GCC | compile/test validation |

## Technology
- C++17
- CMake
- Ninja
- GLM fetched by CMake as the canonical vector and matrix math library
- SDL3 fetched by CMake
- Dear ImGui docking branch lineage fetched by CMake
- Dear ImGui SDL3 platform backend
- Dear ImGui OpenGL3 renderer backend configured for OpenGL ES 3
- System EGL/GLES implementation
- No GLAD

## Build policy
Build directories must live outside the source tree. CMake presets define the canonical developer configurations.
`headless-debug` is the canonical core configuration and builds all display-independent libraries and tests
without fetching SDL or Dear ImGui, discovering EGL/GLES, or defining the graphical executable. Core/domain
targets must remain independent of SDL, Dear ImGui, and GLES; platform, rendering, and UI layers depend inward
on them. Pure feature logic belongs in headless-testable targets whenever practical.

## First application milestone
The first real application feature must:
1. Fetch pinned SDL3 and Dear ImGui revisions with CMake.
2. Build Dear ImGui core plus imgui_demo.cpp.
3. Create an SDL3 window and OpenGL ES 3 context.
4. Initialize the ImGui SDL3 and OpenGL3 backends.
5. Enable docking and keyboard navigation.
6. Create a dockspace.
7. Show a basic Hello World window.
8. Provide menu access to the Dear ImGui demo and useful diagnostic windows.
9. Build on Termux ARM64/Clang and desktop Linux x86-64/GCC.
10. Pass the repository verification command and GitHub CI.

Desktop OpenGL, Vulkan, renderer abstraction, and ImGui multi-viewport support are explicitly deferred.

## Editor shell milestone

The visible editor shell adds a project-owned, display-independent state model for a fixed dummy scene,
selection, object properties, panel visibility, and console messages. Dear ImGui draws the Scene Graph,
Viewport placeholder, Object Inspector, and Console from that shared state. Dock layout construction is a
first-use or explicit reset operation; normal Dear ImGui `.ini` persistence owns later user arrangements.

Scene rendering, ECS, serialization, undo/redo, asset management, and plugin frameworks remain deferred.

## Localization and DPI foundation

The next editor-wide infrastructure milestone establishes:
- external UTF-8 locale resources,
- stable localization keys with English fallback,
- runtime locale switching,
- a clear separation between translation capability and font-glyph coverage,
- SDL3-driven DPI/content-scale awareness,
- scale-aware Dear ImGui fonts and style metrics,
- stable docking/window identities across both language and DPI changes.

Initial glyph coverage may remain Latin-focused, but the localization architecture must not assume ASCII or
prevent later Cyrillic/CJK font profiles. Font-atlas rebuilding or font replacement for larger glyph sets is
a separate concern from string lookup and locale switching.

## Basic interactive scene milestone

The first scene vertical slice keeps four responsibilities distinct:

- `editor` owns object identity, hierarchy, selection, visibility, and authoritative transforms without a
  display dependency;
- `scene` owns GLM-backed transform composition, the orbit camera, procedural primitive data, and framebuffer
  sizing policy without ImGui or graphics APIs;
- `render` owns the single concrete OpenGL ES 3 viewport renderer and its shader, mesh, and offscreen
  framebuffer resources;
- `ui` presents the rendered texture and translates viewport-local mouse input into camera changes.

The Viewport attachment follows the ImGui content region converted through the backend-provided framebuffer
scale. It is resized only when the resulting pixel dimensions change. Renderable objects and their semantic
parameters are supplied by the editor model and each object transform drives its model matrix directly.

This milestone intentionally defers object picking/manipulation, pan and fly controls, gizmos, lighting and
material systems, asset loading, serialization, undo/redo, ECS, and a renderer abstraction. Orbit and zoom
are the only direct viewport interactions.

## Spatial foundation

AI3 uses a right-handed, Z-up world: +X is right, +Y is forward, and +Z is up. Scene lengths are stored in
meters. The editor defaults to meters and can present millimeters, centimeters, meters, or kilometers
without rescaling stored data.

Authoritative orientations are normalized `glm::quat` values. Human-facing rotation values are Euler
degrees using intrinsic XYZ rotations (local X, then local Y, then local Z), with positive angles following
the right-hand rule. Quaternion/Euler conversion is centralized in scene math; an Euler result is one
equivalent representation and is not assumed to be globally unique.

## Object lifecycle and semantic primitives

Scenes begin empty and create objects through the display-independent `EditorState` lifecycle API. Object
IDs are stable, scene-owned, monotonically allocated identities and are not reused after deletion. Deleting
a parent recursively deletes its descendants; selection is cleared when its object is in that subtree.

The first semantic primitive is a sphere whose authoritative radius is stored in meters. Procedural vertex
and index data is deterministic derived data in the headless scene layer. The UI edits radius through the
active display-length conversion, while the concrete GLES3 renderer enumerates all enabled and visible
spheres and generates geometry from their current semantic radii. Neither UI nor renderer owns object
identity or primitive parameters.
