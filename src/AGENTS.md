# Source Agent Instructions

These instructions extend the repository root AGENTS.md.

- Keep platform-specific differences out of application logic whenever SDL3 or CMake can absorb them.
- Phase one uses one renderer: OpenGL ES 3.0.
- SDL3 owns window creation, event handling, GL context creation, swap interval, buffer swapping, and platform DPI/display-scale discovery.
- Use Khronos GLES3 headers and the system GLES library. Do not add GLAD.
- Dear ImGui must use imgui_impl_sdl3 and imgui_impl_opengl3 with IMGUI_IMPL_OPENGL_ES3.
- Enable ImGui docking and keyboard navigation.
- Do not enable ImGui multi-viewport support yet.
- Keep imgui_demo.cpp compiled and expose useful Dear ImGui demo/debug windows through the application UI.
- User-facing source text must use the localization layer once available; keep translation keys stable and semantic rather than derived from English display text.
- Treat all localized strings as UTF-8.
- Keep DPI/content-scale handling centralized; UI and editor modules should consume scaled metrics rather than querying platform details ad hoc.
- Prefer straightforward code over speculative abstractions. Add abstractions when a second real implementation or clear requirement justifies them.
