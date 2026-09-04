# AI3

AI3 is a C++ project targeting OpenGL ES 3 through SDL3, with Dear ImGui (docking branch) for the user
interface and GLM as its canonical vector and matrix math library.

Project documentation: [current architecture](docs/PROJECT.md),
[architectural decisions](docs/decisions/), [roadmap](docs/ROADMAP.md), and
[completed milestones](docs/MILESTONES.md).

The initial supported build targets are:

- Termux on ARM64 using Clang.
- Desktop Linux on x86-64 using GCC.

Configure, build, and test with the preset for your platform:

```sh
bash scripts/bootstrap.sh
bash scripts/check.sh
```

For an existing checkout, choose the synchronization command according to intent:

```sh
# Return the checkout to current known-good main, then verify it.
bash scripts/sync-and-check.sh

# Fast-forward and verify the currently checked-out tracked branch without switching branches.
bash scripts/sync-current-and-check.sh
```

Both synchronization commands require a clean working tree and use fast-forward-only updates. The
current-branch command refuses detached HEAD and branches without a configured upstream.

Bootstrap installs the supported platform prerequisites without changing the platform compiler policy, then
validates CMake 3.25 or newer and clang-format 21.x. On desktop Linux, GCC remains the build compiler; on
Termux, Clang remains the build compiler. Ubuntu Jammy and Noble (including derivatives such as Linux Mint)
use their underlying Ubuntu codename when an official LLVM repository is needed for `clang-format-21` or the
official Kitware repository is needed for a sufficiently new CMake.

The canonical display-independent configuration needs no SDL, Dear ImGui, EGL/GLES, X11, or Wayland
development packages:

```sh
AI3_PRESET=headless-debug bash scripts/check.sh
```

Equivalently, run `cmake --preset headless-debug`, `cmake --build --preset headless-debug`, and
`ctest --preset headless-debug`. This configuration builds the options, editor model, scene/math, units,
localization, GLM, and unit-test targets, but does not define the graphical `ai3` executable or smoke test.

Run `../build/AI3/termux-clang-debug/ai3` on Termux or
`../build/AI3/linux-gcc-debug/ai3` on desktop Linux. The application creates an
SDL3-managed OpenGL ES 3 context and presents a docked editor shell with a Scene Graph,
rendered Viewport, Object Inspector, and Console. The Object menu creates scene objects and deletes the
selected object; sphere radius is edited in the active display unit while remaining stored in meters. Dragging
an object onto another Scene Graph object reparents it, while dragging it onto Scene Root unparents it. These
category-agnostic operations preserve world-space pose through the authoritative editor hierarchy operation
and are visibly rejected when the resulting local transform cannot be represented. The View menu controls editor panels,
the Window menu can restore the default layout and show AI3 scale/locale diagnostics. The Help menu switches
between external English and Spanish UTF-8 locale resources at runtime, while stable internal window IDs
preserve docking. Dear ImGui diagnostics remain available from Help.

Dear ImGui docking/layout persistence is stored as `imgui.ini` beside the running `ai3` executable, independent
of the shell working directory. Smoke mode disables ImGui settings persistence. A future packaged or read-only
installation may require an appropriate per-user configuration directory; the current development runtime does
not implement that packaging policy.

SDL3 window display scale drives font-atlas size, ImGui style metrics, and deliberate editor drawing metrics.
Scale changes are applied at runtime. The current embedded font uses a Latin-focused profile; UTF-8 translation
support is intentionally independent from broader future glyph/font coverage. Locale selection is not yet
persisted because the project does not otherwise have a settings system. See [ADR 0002](docs/decisions/0002-localization-and-ui-scale.md).

Run `ai3 --smoke-test` with a reachable X server to initialize the real SDL/GLES/ImGui stack, render three
hidden frames, and exit. `scripts/check.sh` runs this integration test when `DISPLAY` is set and always runs
display-independent unit tests. Documentation consistency and source formatting are checked by the same script;
use `bash scripts/check-docs.sh` to run only the deterministic documentation checks, and use
`bash scripts/format.sh --write` to apply the repository style. Formatting is defined by clang-format 21.x;
the script prefers `clang-format-21`, accepts an unversioned `clang-format` only when its reported major is
21, and fails instead of falling back to another major.

The Termux preset identifies the target as Linux for CMake platform detection.
Termux's compiler still targets Android's Bionic environment, but SDL must select
its Unix/X11 backend rather than its Android application backend when AI3 runs
through Termux:X11. The preset suppresses SDL's Android Activity conditionals,
links Termux's shared-memory compatibility library, and disables non-X11 Unix
video backends. The Termux build preset is limited to one build job. SDL is
linked statically as an application-owned FetchContent dependency.

The application is divided into run-loop policy (`src/app`), project-owned editor state (`src/editor`),
SDL/GLES ownership (`src/platform`), ImGui lifecycle and editor-shell drawing (`src/ui`), and a thin
process entry point. The editor state is independent of ImGui and owns object lifecycle, hierarchy,
selection, primitive semantics, transforms, panel state, and console data. The Termux X11 rationale is recorded in
[ADR 0001](docs/decisions/0001-termux-x11-backend.md).

Core/domain targets are intentionally independent of SDL, Dear ImGui, and GLES. Platform, rendering, and UI
targets may depend on those core targets; dependencies must not point back outward from core into graphics.
Scene objects use stable monotonic IDs and public create/delete APIs. Deleting an object deletes only that
object; its direct children survive as scene roots and preserve their world-space poses.
Sphere parameters are semantic editor state; procedural meshes are derived in the headless scene layer.

The application's single viewport has display-independent view state outside Dear ImGui. Its source is Editor
View or Scene Camera; the localized selector currently lists Perspective Camera scene objects because that is
the only implemented camera subtype. The Editor View retains an independent orbit-style pose and pivot.
Scene-camera matrices are derived from current authoritative world transforms and projection parameters, while
the GLES3 renderer consumes only resolved view/projection matrices. Deleting a viewed camera falls back to the
Editor View, and Reset Scene restores its default state. See
[ADR 0005](docs/decisions/0005-viewport-view-ownership.md).

Scene Documents use strict versioned `.ai3scene` JSON with transactional loading, native Open/Save/Save As,
dirty-state protection, and linear Undo/Redo transactions. Version 2 stores reusable Lambert and Phong
materials and linear authored colors while retaining strict v1 migration. Per-document `.ai3workspace`
sidecars currently persist optional sphere-bound display controls separately from authored scene data.

The viewport provides explicit Selection and Navigation modes, exact CPU picking for transformed spheres, and
single-object X/Y/Z translation in Local, Parent, World, and View reference spaces. Cached sphere AABB and
bounding-sphere helpers are rendered through GLES: bounds remain depth tested while translation gizmos render
on top. Middle-button drag temporarily pans the Editor View, Alt+middle-button drag temporarily orbits it, and
the mouse wheel zooms it without changing the retained interaction mode. These interactions remain
display-independent outside their concrete ImGui input and GLES presentation boundaries; Scene Camera
navigation remains inert.
