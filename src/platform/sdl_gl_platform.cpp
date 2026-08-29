#include "platform/sdl_gl_platform.h"
#include <SDL3/SDL.h>
#include <cstdio>
namespace ai3
{
namespace
{
void set_gl_attribute(SDL_GLAttr attribute, int value, const char* name)
{
    if (!SDL_GL_SetAttribute(attribute, value))
        throw SdlError(name);
}
} // namespace
SdlError::SdlError(const std::string& operation)
    : std::runtime_error(operation + ": " + SDL_GetError())
{
}
SdlGlPlatform::SdlGlPlatform(const char* title, int width, int height, bool hidden)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw SdlError("SDL_Init failed");
    try
    {
        set_gl_attribute(SDL_GL_CONTEXT_FLAGS, 0, "SDL_GL_SetAttribute(context flags) failed");
        set_gl_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES,
                         "SDL_GL_SetAttribute(ES profile) failed");
        set_gl_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3,
                         "SDL_GL_SetAttribute(major version) failed");
        set_gl_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 0,
                         "SDL_GL_SetAttribute(minor version) failed");
        set_gl_attribute(SDL_GL_DOUBLEBUFFER, 1, "SDL_GL_SetAttribute(double buffer) failed");
        set_gl_attribute(SDL_GL_DEPTH_SIZE, 24, "SDL_GL_SetAttribute(depth size) failed");
        set_gl_attribute(SDL_GL_STENCIL_SIZE, 8, "SDL_GL_SetAttribute(stencil size) failed");
        SDL_WindowFlags flags =
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (hidden)
            flags |= SDL_WINDOW_HIDDEN;
        window_ = SDL_CreateWindow(title, width, height, flags);
        if (window_ == nullptr)
            throw SdlError("SDL_CreateWindow failed");
        gl_context_ = SDL_GL_CreateContext(window_);
        if (gl_context_ == nullptr)
            throw SdlError("SDL_GL_CreateContext failed");
        if (!SDL_GL_MakeCurrent(window_, gl_context_))
            throw SdlError("SDL_GL_MakeCurrent failed");
        if (!SDL_GL_SetSwapInterval(hidden ? 0 : 1))
            std::fprintf(stderr, "ai3: warning: SDL_GL_SetSwapInterval failed: %s\n",
                         SDL_GetError());
    }
    catch (...)
    {
        if (gl_context_ != nullptr)
            SDL_GL_DestroyContext(gl_context_);
        if (window_ != nullptr)
            SDL_DestroyWindow(window_);
        SDL_Quit();
        throw;
    }
}
SdlGlPlatform::~SdlGlPlatform()
{
    if (gl_context_ != nullptr)
        SDL_GL_DestroyContext(gl_context_);
    if (window_ != nullptr)
        SDL_DestroyWindow(window_);
    SDL_Quit();
}
void SdlGlPlatform::swap_window() const
{
    if (!SDL_GL_SwapWindow(window_))
        throw SdlError("SDL_GL_SwapWindow failed");
}
float SdlGlPlatform::display_scale() const
{
    const float scale = SDL_GetWindowDisplayScale(window_);
    if (scale <= 0.0F)
        throw SdlError("SDL_GetWindowDisplayScale failed");
    return scale;
}
} // namespace ai3
