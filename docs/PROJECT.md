# AI3 Current Architecture

This document describes the current repository state. Architectural rationale belongs in
[`decisions/`](decisions/), future work in [`ROADMAP.md`](ROADMAP.md), and completed milestone history in
[`MILESTONES.md`](MILESTONES.md). Implementation and tests are authoritative if documentation disagrees.

## Supported phase-one platform

AI3 is a C++17 application built with CMake and Ninja. It targets Termux ARM64 with Clang and desktop Linux
x86-64 with GCC. The graphical application uses SDL3 for its window, input, OpenGL context, and display-scale
integration; OpenGL ES 3.0 is the only renderer API. Dear ImGui comes from docking-branch lineage and uses
`imgui_impl_sdl3` and `imgui_impl_opengl3` with `IMGUI_IMPL_OPENGL_ES3`. SDL3, Dear ImGui, GLM, nlohmann/json,
and doctest are fetched at pinned revisions. There is no desktop OpenGL path, Vulkan path, GLAD, renderer abstraction, or
ImGui multi-viewport support.

On Termux, SDL uses its X11 backend and runs as a normal process against Termux:X11 rather than as an Android
Activity. See [ADR 0001](decisions/0001-termux-x11-backend.md).

## Build and dependency boundaries

The canonical verification entry point is `bash scripts/check.sh`. Existing development checkouts can return
to `origin/main` and run verification with `bash scripts/sync-and-check.sh`, or fast-forward and verify the
currently checked-out tracked branch without switching with `bash scripts/sync-current-and-check.sh`. Both
commands require a clean working tree and use fast-forward-only updates; the branch-preserving command refuses
detached HEAD and missing upstream configuration. Known-good main synchronization examples are:

- Termux: `cd ~/Projects/AI3 && bash scripts/sync-and-check.sh`
- T5600: `cd ~/Documents/Projects/AI3 && bash scripts/sync-and-check.sh`

The main synchronization command explicitly switches to `main`; it is not the feature-branch test command.
Both commands delegate all formatting, configuration, build, and test behavior to `check.sh`. Verification
requires CMake 3.25 or newer and defines source formatting with clang-format 21.x; the scripts validate both
contracts before using the tools. CMake presets keep builds outside the source tree. The `headless-debug`
preset builds the options, editor, scene, localization, and test targets without fetching SDL or Dear ImGui,
discovering EGL/GLES, or defining the graphical executable and smoke test.

Core/domain targets do not depend on SDL, Dear ImGui, EGL/GLES, or a display. Platform, render, and UI code may
depend inward on core/domain code, but core/domain dependencies do not point outward. The current ownership
split is:

- `app`: command-line and run-loop policy; `Application::run` creates the application-lifetime editor and
  viewport state.
- `editor`: display-independent object identity, lifecycle, hierarchy, selection, authoritative transforms,
  semantic object and material data, undo/redo transactions, document revision/session workflow, transactional Scene Document
  serialization/filesystem I/O, panel visibility, layout-reset intent, and console data.
- `scene`: display-independent units, transform and camera math, procedural sphere geometry, render-target
  sizing, orbit view construction, and viewport-view selection/resolution.
- `localization`: external resource discovery and UTF-8 string lookup.
- `platform`: SDL window, event, GLES-context, swap, and display-scale ownership.
- `render`: the single concrete GLES3 `ViewportRenderer`, including shaders, sphere geometry caches, and the
  offscreen viewport framebuffer. A concrete GLES program helper owns compilation/linking and lifetime.
- `ui`: Dear ImGui lifecycle and editor presentation/control. `EditorUi` receives references to authoritative
  `EditorState` and `ViewportView`, presents the display-independent document-session policy and SDL
  native-dialog result handoff, and currently owns the concrete `ViewportRenderer` and its GLES resources.

## Editor and scene model

Scenes start empty. `EditorState` is the only scene-object lifecycle and hierarchy authority. Object IDs are
scene-owned, stable, monotonically allocated, and not reused after deletion. Objects share identity, name,
enabled/visible state, a local transform, and a two-level semantic tag. Current concrete object subtypes are
sphere primitive, perspective camera, and directional light; their payloads are plain tagged data rather
than polymorphic objects or components. Default-name counters are monotonic per category/subtype. Ordinary
persistent mutations use explicit `EditorState` operations; public object lookup is read-only. Normal editor
mutations participate in `EditorHistory` transactions whose boundaries represent one intentional edit. A
monotonic document revision advances for each real serialized-state change and authoritative history
restoration, never rewinds on Undo, and ignores no-op assignments and workspace interaction.

Deleting an object deletes only that object. Its direct children become scene roots with preserved world-space
poses; deeper descendants retain their existing parents. Deletion is transactional if any direct child cannot
be faithfully unparented as TRS. Selection is cleared only when the deleted object was selected.

Sphere radius/fallback color/material assignment, perspective projection parameters, directional-light
parameters, and independent reusable materials are authoritative semantic editor data. Sphere meshes are
deterministic derived data. Unassigned spheres render their unlit fallback; assigned spheres use Lambert or
classic Phong. Materials have separate monotonic identity, may exist unassigned, and are not owned by cameras
or lights. The Material Editor navigates all document materials while its active material remains workspace
state and it edits one material at a time.

## Scene Documents

AI3 Scene Documents are strict, versioned UTF-8 JSON files with the `.ai3scene` extension, format identifier
`ai3-scene`, and current version `2`, with strict v1 migration. They persist exact nonzero object and material
IDs and ordering, names, enabled and
visible state, parent IDs, authoritative local TRS, durable category/subtype names, current semantic payloads,
object/material allocators and default-name counters, material definitions, sphere assignments/fallback colors,
and linear light colors. Quaternion arrays have stable `[w, x, y, z]` order
and are normalized. World transforms, Euler presentation, derived geometry, renderer state, and editor
workspace/session state are outside the format.

The headless editor target owns serialization, validation, and ordinary-filesystem helpers through pinned
nlohmann/json. A narrowly friended codec reconstructs a complete candidate with explicit identity and local
hierarchy data; normal object creation still uses the lifecycle allocator. Load validates the entire candidate
before replacing scene-owned state, and failure leaves the destination unchanged. Successful load clears
selection while preserving non-document editor state such as console, panel visibility, and layout intent.

`EditorHistory` uses internal authoritative before/after snapshots and exposes representation-independent
begin/commit/cancel/undo/redo operations. Snapshots contain exact document state but exclude selection,
viewport/session state, derived geometry, renderer caches, and GPU state. `DocumentSession` owns the active
document's associated path, saved history checkpoint, dirty determination, filesystem workflow, and pending
New/Open/Quit transition. New and successful Open rebaseline history; failed Open preserves it. Reset Scene is
one undoable edit that retains the path. New, Open, Quit, and window close share
Save/Discard/Cancel protection. The graphical UI presents that policy and uses SDL3 asynchronous native file
dialogs for Open and Save As. After successful Open, the UI resets the viewport to default Orbit and clears
renderer geometry caches before subsequent rendering. See
[ADR 0006](decisions/0006-scene-document-format.md) and
[ADR 0007](decisions/0007-document-revision-and-session.md).

Dirty determination includes real authoritative changes made during an active transaction before it commits,
so destructive-transition protection remains effective during live inspector edits and event-loop close
requests. An active transaction with no authoritative change remains clean.

## Spatial and hierarchy invariants

AI3 uses a right-handed, Z-up world: +X right, +Y forward, and +Z up. Stored scene lengths are meters.
Authoritative orientations are normalized `glm::quat` values. Human-facing Euler rotations are degrees in
intrinsic XYZ order and use centralized conversion helpers. Metric display units affect presentation only.
See [ADR 0003](decisions/0003-spatial-conventions.md).

Object transforms are local-to-parent TRS; a root local transform is also its world transform. `EditorState`
recursively resolves world transforms for every consumer. All object categories may parent one another.
Reparenting rejects missing parents, self-parenting, descendants, cycles, non-invertible parents, and affine
results that cannot be reconstructed faithfully as TRS. Successful reparenting is transactional and preserves
the object's world matrix, including supported reflected/negative-scale cases.

The Scene Graph presents the authoritative hierarchy as nested nodes. Dropping an object onto another delegates
to `EditorState::reparent_object`; dropping it onto the UI-only Scene Root requests unparenting. Rejections are
reported in the Console. Local, Parent, World, and View are explicit tool reference spaces and do not change
transform storage semantics. See [ADR 0004](decisions/0004-hierarchy-world-transforms.md).

## Viewport and rendering

The application has one viewport with display-independent `ViewportView` state outside Dear ImGui. Its source
is Orbit or Scene Camera. Orbit state remains independent of scene hierarchy. Scene-camera selection belongs
to the viewport, not to a global active-camera concept, and currently accepts perspective-camera objects only.

Views are resolved on demand from current state. Scene-camera resolution uses the authoritative resolved world
position/orientation, current projection parameters, and current viewport aspect ratio. Deleting the selected
camera falls back deterministically to the unchanged Orbit state; Reset Scene resets the viewport to default
Orbit state. `ViewportRenderer` consumes only resolved view/projection values and does not know which mode
constructed them. See [ADR 0005](decisions/0005-viewport-view-ownership.md).

Resolved views include derived world-space eye position for view-dependent shading in both Orbit and scene-camera
paths. The renderer draws the scene into its GLES color/depth target, sized from the ImGui viewport content region
after framebuffer scaling. Dear ImGui presents that texture in the visible editor window. Display-independent
view semantics are already separated from UI ownership, but construction and lifetime of `ViewportRenderer`
and its GLES resources remain inside `EditorUi`.

Unlit fallback, Lambert, and Phong use distinct linked GLES3 programs with only their required uniforms.
Material shading models map to concrete programs rather than a runtime-branched uber-shader; this direction
does not introduce a generalized shader system or renderer/backend abstraction. Material ambient color is an
artist-authored contribution in the simplified lighting equations, not a global ambient-light source.

## Localization, DPI, and editor shell

Project-owned user-facing strings are external UTF-8 JSON resources in `assets/locales`. `en-US` is mandatory
and is the initial and per-key fallback locale; missing keys render visibly. Locale switching is runtime, and
stable hidden ImGui IDs preserve docking identities. The current embedded font profile is Latin-focused;
translation capability and glyph coverage are separate concerns.

SDL window display scale is the UI-scale source of truth. Scale changes rebuild the ImGui font atlas and
derive style metrics from unscaled defaults without replacing editor or docking state. See
[ADR 0002](decisions/0002-localization-and-ui-scale.md).

The Edit menu provides localized Undo and Redo with enabled states and Ctrl+Z/Ctrl+Shift+Z shortcuts. Current
name, numeric, color, and transform controls group one ImGui interaction into one transaction. The docked shell
contains Scene Graph, Viewport, Object Inspector, and Console panels. Normal Dear ImGui `.ini` persistence owns
user layout after first-use construction; its `imgui.ini` is stored beside the running executable rather than
relative to the shell working directory. Smoke mode disables settings persistence. A future packaged/read-only
installation may require a per-user configuration location, which is not implemented by the current
development runtime. Reset Layout requests reconstruction. The demo and diagnostic windows remain compiled and
available.

Authored material, directional-light, and primitive fallback colors are finite `[0,1]` linear RGB.
Artist-facing controls present sRGB through centralized standard transfer functions; shaders light in linear
RGB and explicitly encode output to sRGB for the ordinary RGBA8 target. See
[ADR 0008](decisions/0008-materials-and-linear-color.md).
