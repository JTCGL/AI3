#include "ui/imgui_layer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <stdexcept>
namespace ai3
{
ImGuiLayer::ImGuiLayer(SDL_Window* window, SDL_GLContext gl_context, bool save_settings)
{
    IMGUI_CHECKVERSION();
    if (ImGui::CreateContext() == nullptr)
        throw std::runtime_error("ImGui context initialization failed");
    context_initialized_ = true;
    ImGuiIO& io = ImGui::GetIO();
    if (!save_settings)
        io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context))
    {
        ImGui::DestroyContext();
        context_initialized_ = false;
        throw std::runtime_error("ImGui SDL3 backend initialization failed");
    }
    sdl_backend_initialized_ = true;
    if (!ImGui_ImplOpenGL3_Init("#version 300 es"))
    {
        ImGui_ImplSDL3_Shutdown();
        sdl_backend_initialized_ = false;
        ImGui::DestroyContext();
        context_initialized_ = false;
        throw std::runtime_error("ImGui OpenGL ES backend initialization failed");
    }
    gl_backend_initialized_ = true;
}
ImGuiLayer::~ImGuiLayer()
{
    if (gl_backend_initialized_)
        ImGui_ImplOpenGL3_Shutdown();
    if (sdl_backend_initialized_)
        ImGui_ImplSDL3_Shutdown();
    if (context_initialized_)
        ImGui::DestroyContext();
}
void ImGuiLayer::begin_frame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}
} // namespace ai3
