# AI3 Agent Instructions

## Project scope
AI3 is a C++17 application project. Phase one targets:
- Termux on ARM64 using Clang.
- Desktop Linux on x86-64 using GCC.
- OpenGL ES 3.0 only.
- SDL3 for windowing, input, GL context management, and DPI/display integration.
- Dear ImGui from the docking branch, using imgui_impl_sdl3 and imgui_impl_opengl3 with IMGUI_IMPL_OPENGL_ES3.

Do not introduce desktop OpenGL, Vulkan, GLAD, a renderer abstraction, or multi-viewport support unless the project requirements are explicitly changed.

## Scope discipline
- Implement only functionality explicitly requested by the current task, issue, or approved follow-up.
- Do not anticipate later milestones or add optional features merely because they appear useful.
- Treat language such as "optional", "desirable", "nice to have", or "if useful" as deferred unless the user explicitly approves that work for the current milestone.
- When future architecture is obvious but not yet specified, add only the minimum seam required by the current task and leave later behavior unimplemented.
- If completing the requested work appears to require broader functionality, stop and surface that dependency instead of silently expanding scope.
- Existing provisional behavior from earlier milestones may be replaced later when its dedicated design milestone arrives; do not harden provisional choices into permanent architecture without explicit approval.

## User-facing text and localization
- All new user-facing editor text must go through the project localization/string system once that system exists.
- Do not introduce new hard-coded user-facing labels directly in ImGui widgets except while implementing or testing the localization layer itself.
- Diagnostic, developer-only, assertion, and low-level error text may remain direct literals when localization would add no user value.
- Localization resources must be external UTF-8 data files and must not assume ASCII internally.
- English is the required fallback locale. Missing keys must fail visibly in development and fall back predictably rather than silently producing empty UI text.
- Runtime locale switching should update the UI without requiring application restart when the active font atlas already covers the required glyphs.
- Do not conflate translation support with font-glyph coverage. The string system must remain Unicode/UTF-8 capable even when the current ImGui font atlas supports only a smaller character repertoire.

## DPI and UI scaling
- AI3 must remain DPI aware on every supported platform.
- Use SDL3 display/window scaling information as the platform source of truth; do not hard-code assumptions that 1 logical pixel equals 1 physical pixel.
- Dear ImGui style, fonts, and editor layout must scale coherently with display DPI/content scale.
- Font size changes caused by DPI changes must trigger the appropriate ImGui font-atlas rebuild or font reload path.
- Runtime movement between displays with different scale factors must be handled where SDL3 exposes the relevant events/state.
- Avoid fixed pixel measurements in editor UI code when a scaled logical value or content-region measurement is appropriate.
- DPI behavior must not break persisted docking layouts or make localization depend on a specific display scale.

## Spatial conventions
- AI3 world space is right-handed: +X is right, +Y is forward, and +Z is up.
- Scene lengths are stored in meters. Display-unit conversion must not change stored scene values.
- Store authoritative orientations as `glm::quat`; human-facing Euler angles use degrees and the
  documented intrinsic XYZ convention.
- Positive rotations follow the right-hand rule. Centralize quaternion/Euler conversion rather than
  scattering angle conversions through scene or UI code.
- Convert external coordinate systems and units at import/export boundaries; do not alter AI3's internal
  convention.

## Build and dependency policy
- Use CMake and Ninja.
- Keep all builds out of the source tree.
- Use CMake FetchContent for SDL3 and Dear ImGui.
- Use GLM as the canonical library for vectors, matrices, transforms, projections, and view math.
- Allow compiler/toolchain-selected SIMD baselines; do not force ISA flags or use `-march=native`.
- Pin third-party dependencies to known-good immutable revisions. Dear ImGui must come from docking-branch lineage.
- Compile imgui_demo.cpp so the Dear ImGui demo and diagnostic windows remain available.
- Prefer system EGL/GLES headers and libraries; do not vendor generated GL loaders for GLES3.
- Do not vendor SDL3 or Dear ImGui source into this repository.
- Keep core/domain targets buildable and testable without SDL, Dear ImGui, EGL/GLES, or a display system.
- UI, render, and platform layers may depend inward on core/domain layers; core/domain layers must not
  depend outward on SDL, Dear ImGui, or GLES. Place pure feature logic in headless-testable targets whenever
  practical.

## Verification
Before declaring work complete, run:

```sh
bash scripts/check.sh
```

The same verification path should be used locally and by GitHub Actions wherever practical.

### Termux resource safety
- Keep the Termux build preset at one parallel job.
- Run local build and verification commands serially in the foreground.
- Do not leave background build, check, or CI polling processes running.
- Do not use long-lived `gh --watch` commands from Termux.
- After pushing a pull-request update, stop and leave CI monitoring to the user or reviewer unless
  the user explicitly requests a separate CI check.
- Preserve the normal GitHub Actions workflow; these constraints apply to local Termux agent work.

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
