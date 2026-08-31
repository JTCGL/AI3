#pragma once
#include <SDL3/SDL_video.h>
#include <string>
namespace ai3
{
class ImGuiLayer
{
    public:
    ImGuiLayer(SDL_Window* window, SDL_GLContext gl_context, std::string ini_filename,
               float display_scale);
    ~ImGuiLayer();
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
    void begin_frame() const;
    void update_display_scale(float display_scale);
    float ui_scale() const { return ui_scale_; }
    float font_size() const { return font_size_; }

    private:
    bool context_initialized_ = false;
    bool sdl_backend_initialized_ = false;
    bool gl_backend_initialized_ = false;
    std::string ini_filename_;
    float ui_scale_ = 1.0F;
    float font_size_ = 16.0F;
};
} // namespace ai3
