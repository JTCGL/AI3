#pragma once
#include <SDL3/SDL_video.h>
namespace ai3
{
class ImGuiLayer
{
    public:
    ImGuiLayer(SDL_Window* window, SDL_GLContext gl_context, bool save_settings);
    ~ImGuiLayer();
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
    void begin_frame() const;

    private:
    bool context_initialized_ = false;
    bool sdl_backend_initialized_ = false;
    bool gl_backend_initialized_ = false;
};
} // namespace ai3
