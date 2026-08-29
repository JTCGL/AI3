#include "app/application.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "platform/sdl_gl_platform.h"
#include "ui/editor_ui.h"
#include "ui/imgui_layer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <cstdio>
#include <exception>
namespace ai3
{
Application::Application(ApplicationOptions options) : options_(options) {}
int Application::run()
{
    try
    {
        SdlGlPlatform platform("AI3", 1280, 720, options_.frame_limit > 0);
        ImGuiLayer imgui(platform.window(), platform.gl_context(), options_.frame_limit == 0);
        EditorUi editor_ui;
        bool running = true;
        int rendered_frames = 0;
        while (running && (options_.frame_limit == 0 || rendered_frames < options_.frame_limit))
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT ||
                    (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                     event.window.windowID == SDL_GetWindowID(platform.window())))
                    running = false;
            }
            if (!running)
                break;
            if ((SDL_GetWindowFlags(platform.window()) & SDL_WINDOW_MINIMIZED) != 0)
            {
                SDL_Delay(10);
                continue;
            }
            imgui.begin_frame();
            editor_ui.draw(running);
            ImGui::Render();
            int width = 0;
            int height = 0;
            if (!SDL_GetWindowSizeInPixels(platform.window(), &width, &height))
                throw SdlError("SDL_GetWindowSizeInPixels failed");
            glViewport(0, 0, width, height);
            glClearColor(0.08F, 0.09F, 0.12F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            platform.swap_window();
            ++rendered_frames;
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "ai3: %s\n", error.what());
        return 1;
    }
}
} // namespace ai3
