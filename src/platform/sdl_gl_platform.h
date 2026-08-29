#pragma once
#include <SDL3/SDL_video.h>
#include <stdexcept>
#include <string>
namespace ai3
{
class SdlError : public std::runtime_error
{
    public:
    explicit SdlError(const std::string& operation);
};
class SdlGlPlatform
{
    public:
    SdlGlPlatform(const char* title, int width, int height, bool hidden);
    ~SdlGlPlatform();
    SdlGlPlatform(const SdlGlPlatform&) = delete;
    SdlGlPlatform& operator=(const SdlGlPlatform&) = delete;
    SDL_Window* window() const { return window_; }
    SDL_GLContext gl_context() const { return gl_context_; }
    float display_scale() const;
    void swap_window() const;

    private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
};
} // namespace ai3
