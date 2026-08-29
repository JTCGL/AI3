#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <cstdio>

namespace {

void log_sdl_error(const char* operation)
{
    std::fprintf(stderr, "%s: %s\n", operation, SDL_GetError());
}

} // namespace

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log_sdl_error("SDL_Init failed");
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "AI3", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        log_sdl_error("SDL_CreateWindow failed");
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        log_sdl_error("SDL_GL_CreateContext failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    if (!SDL_GL_SetSwapInterval(1)) {
        log_sdl_error("Warning: SDL_GL_SetSwapInterval failed");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context)) {
        std::fprintf(stderr, "ImGui SDL3 backend initialization failed\n");
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        std::fprintf(stderr, "ImGui OpenGL ES backend initialization failed\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    bool show_hello = true;
    bool show_demo = false;
    bool show_metrics = false;
    bool show_debug_log = false;
    bool show_id_stack = false;
    bool show_about = false;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT
                || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                    && event.window.windowID == SDL_GetWindowID(window))) {
                running = false;
            }
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Application")) {
                ImGui::MenuItem("Hello World", nullptr, &show_hello);
                ImGui::Separator();
                if (ImGui::MenuItem("Quit")) {
                    running = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("ImGui")) {
                ImGui::MenuItem("Demo Window", nullptr, &show_demo);
                ImGui::MenuItem("Metrics/Debugger", nullptr, &show_metrics);
                ImGui::MenuItem("Debug Log", nullptr, &show_debug_log);
                ImGui::MenuItem("ID Stack Tool", nullptr, &show_id_stack);
                ImGui::MenuItem("About Dear ImGui", nullptr, &show_about);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (show_hello) {
            ImGui::Begin("Hello World", &show_hello);
            ImGui::TextUnformatted("Hello from AI3!");
            ImGui::TextUnformatted("SDL3 + OpenGL ES 3 + Dear ImGui docking is ready.");
            ImGui::End();
        }
        if (show_demo) ImGui::ShowDemoWindow(&show_demo);
        if (show_metrics) ImGui::ShowMetricsWindow(&show_metrics);
        if (show_debug_log) ImGui::ShowDebugLogWindow(&show_debug_log);
        if (show_id_stack) ImGui::ShowIDStackToolWindow(&show_id_stack);
        if (show_about) ImGui::ShowAboutWindow(&show_about);

        ImGui::Render();
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.08F, 0.09F, 0.12F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
