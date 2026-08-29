#include "ui/imgui_layer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace ai3
{
namespace
{
constexpr float base_font_size = 16.0F;
constexpr float minimum_scale = 0.75F;
constexpr float maximum_scale = 4.0F;
} // namespace
ImGuiLayer::ImGuiLayer(SDL_Window* window, SDL_GLContext gl_context, bool save_settings,
                       float display_scale)
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
    update_display_scale(display_scale);
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
void ImGuiLayer::update_display_scale(float display_scale)
{
    const float requested_scale = std::clamp(display_scale, minimum_scale, maximum_scale);
    if (context_initialized_ && std::abs(requested_scale - ui_scale_) < 0.05F &&
        ImGui::GetIO().Fonts->Fonts.Size > 0)
        return;

    ui_scale_ = requested_scale;
    font_size_ = base_font_size * ui_scale_;
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle{};
    ImGui::StyleColorsDark(&style);
    style.ScaleAllSizes(ui_scale_);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig config;
    config.SizePixels = font_size_;
    io.Fonts->AddFontDefault(&config);
}
} // namespace ai3
