# AI3 Agent Instructions

## Project scope
AI3 is a C++17 application project. Phase one targets:
- Termux on ARM64 using Clang.
- Desktop Linux on x86-64 using GCC.
- OpenGL ES 3.0 only.
- SDL3 for windowing, input, and GL context management.
- Dear ImGui from the docking branch, using imgui_impl_sdl3 and imgui_impl_opengl3 with IMGUI_IMPL_OPENGL_ES3.

Do not introduce desktop OpenGL, Vulkan, GLAD, a renderer abstraction, or multi-viewport support unless the project requirements are explicitly changed.

## Build and dependency policy
- Use CMake and Ninja.
- Keep all builds out of the source tree.
- Use CMake FetchContent for SDL3 and Dear ImGui.
- Pin third-party dependencies to known-good immutable revisions. Dear ImGui must come from docking-branch lineage.
- Compile imgui_demo.cpp so the Dear ImGui demo and diagnostic windows remain available.
- Prefer system EGL/GLES headers and libraries; do not vendor generated GL loaders for GLES3.
- Do not vendor SDL3 or Dear ImGui source into this repository.

## Verification
Before declaring work complete, run:

```sh
bash scripts/check.sh
```

The same verification path should be used locally and by GitHub Actions wherever practical.

## Repository workflow
- Treat main as the known-good branch.
- Do substantial implementation on short-lived feature branches.
- Open a pull request into main.
- Fix failing CI before merge.
- Keep commits focused and explain architectural changes in the pull request.

## Agent behavior
Spend model effort on design, implementation, debugging, tests, and review. Prefer deterministic repository tooling for environment setup, formatting, building, and routine shell work.
Do not silently change established architecture. Record deliberate architectural changes in project documentation.

More local AGENTS.md files may add directory-specific rules.
