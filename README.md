# AI3

AI3 is a C++ project targeting OpenGL ES 3 through SDL3, with Dear ImGui (docking branch) for the user interface.

The initial supported build targets are:

- Termux on ARM64 using Clang.
- Desktop Linux on x86-64 using GCC.

Configure, build, and test with the preset for your platform:

```sh
bash scripts/check.sh
```

Run `../build/AI3/termux-clang-debug/ai3` on Termux or
`../build/AI3/linux-gcc-debug/ai3` on desktop Linux. The application creates an
SDL3-managed OpenGL ES 3 context and presents a docked editor shell with a Scene Graph,
Viewport placeholder, Object Inspector, and Console. The View menu controls editor panels,
the Window menu can restore the default layout and show AI3 scale/locale diagnostics. The Help menu switches
between external English and Spanish UTF-8 locale resources at runtime, while stable internal window IDs
preserve docking. Dear ImGui diagnostics remain available from Help.

SDL3 window display scale drives font-atlas size, ImGui style metrics, and deliberate editor drawing metrics.
Scale changes are applied at runtime. The current embedded font uses a Latin-focused profile; UTF-8 translation
support is intentionally independent from broader future glyph/font coverage. Locale selection is not yet
persisted because the project does not otherwise have a settings system. See [ADR 0002](docs/decisions/0002-localization-and-ui-scale.md).

Run `ai3 --smoke-test` with a reachable X server to initialize the real SDL/GLES/ImGui stack, render three
hidden frames, and exit. `scripts/check.sh` runs this integration test when `DISPLAY` is set and always runs
display-independent unit tests. Source formatting is checked by the same script; use
`bash scripts/format.sh --write` to apply the repository style.

The Termux preset identifies the target as Linux for CMake platform detection.
Termux's compiler still targets Android's Bionic environment, but SDL must select
its Unix/X11 backend rather than its Android application backend when AI3 runs
through Termux:X11. The preset suppresses SDL's Android Activity conditionals,
links Termux's shared-memory compatibility library, and disables non-X11 Unix
video backends. The Termux build preset is limited to one build job. SDL is
linked statically as an application-owned FetchContent dependency.

The application is divided into run-loop policy (`src/app`), project-owned editor state (`src/editor`),
SDL/GLES ownership (`src/platform`), ImGui lifecycle and editor-shell drawing (`src/ui`), and a thin
process entry point. The editor state is independent of ImGui and currently contains only dummy hierarchy,
selection, panel, property, and console data. The Termux X11 rationale is recorded in
[ADR 0001](docs/decisions/0001-termux-x11-backend.md).
