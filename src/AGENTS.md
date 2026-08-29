# Source Agent Instructions

These instructions extend the repository root AGENTS.md.

- Keep platform-specific differences out of application logic whenever SDL3 or CMake can absorb them.
- Phase one uses one renderer: OpenGL ES 3.0.
- SDL3 owns window creation, event handling, GL context creation, swap interval, and buffer swapping.
- Use Khronos GLES3 headers and the system GLES library. Do not add GLAD.
- Dear ImGui must use imgui_impl_sdl3 and imgui_impl_opengl3 with IMGUI_IMPL_OPENGL_ES3.
- Enable ImGui docking and keyboard navigation.
- Do not enable ImGui multi-viewport support yet.
- Keep imgui_demo.cpp compiled and expose useful Dear ImGui demo/debug windows through the application UI.
- Prefer straightforward code over speculative abstractions. Add abstractions when a second real implementation or clear requirement justifies them.
