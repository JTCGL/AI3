#include "ui/editor_ui.h"
#include "imgui.h"
namespace ai3
{
void EditorUi::draw(bool& running)
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Application"))
        {
            ImGui::MenuItem("Hello World", nullptr, &show_hello_);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
                running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("ImGui"))
        {
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_);
            ImGui::MenuItem("Metrics/Debugger", nullptr, &show_metrics_);
            ImGui::MenuItem("Debug Log", nullptr, &show_debug_log_);
            ImGui::MenuItem("ID Stack Tool", nullptr, &show_id_stack_);
            ImGui::MenuItem("About Dear ImGui", nullptr, &show_about_);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (show_hello_)
    {
        ImGui::Begin("Hello World", &show_hello_);
        ImGui::TextUnformatted("Hello from AI3!");
        ImGui::TextUnformatted("SDL3 + OpenGL ES 3 + Dear ImGui docking is ready.");
        ImGui::End();
    }
    if (show_demo_)
        ImGui::ShowDemoWindow(&show_demo_);
    if (show_metrics_)
        ImGui::ShowMetricsWindow(&show_metrics_);
    if (show_debug_log_)
        ImGui::ShowDebugLogWindow(&show_debug_log_);
    if (show_id_stack_)
        ImGui::ShowIDStackToolWindow(&show_id_stack_);
    if (show_about_)
        ImGui::ShowAboutWindow(&show_about_);
}
} // namespace ai3
