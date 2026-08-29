#include "ui/editor_ui.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace ai3
{
namespace
{
struct PanelMenuEntry
{
    const char* label;
    EditorPanel panel;
};

constexpr std::array<PanelMenuEntry, 4> panel_menu_entries = {
    {{"Scene Graph", EditorPanel::scene_graph},
     {"Viewport", EditorPanel::viewport},
     {"Object Inspector", EditorPanel::object_inspector},
     {"Console", EditorPanel::console}}};

void build_default_layout(ImGuiID dockspace_id, const ImGuiViewport& viewport)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(dockspace_id, viewport.WorkPos);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport.WorkSize);

    ImGuiID center = dockspace_id;
    const ImGuiID left =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20F, nullptr, &center);
    const ImGuiID right =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24F, nullptr, &center);
    const ImGuiID bottom =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.27F, nullptr, &center);
    ImGui::DockBuilderDockWindow("Scene Graph", left);
    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Object Inspector", right);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderFinish(dockspace_id);
}
} // namespace

void EditorUi::draw_main_menu(bool& running)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit"))
                running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::TextDisabled("No editing commands yet");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            for (const PanelMenuEntry& entry : panel_menu_entries)
            {
                bool visible = state_.panel_visible(entry.panel);
                if (ImGui::MenuItem(entry.label, nullptr, &visible))
                    state_.set_panel_visible(entry.panel, visible);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Reset Layout"))
                state_.request_layout_reset();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::BeginMenu("Dear ImGui Tools"))
            {
                ImGui::MenuItem("Demo Window", nullptr, &show_demo_);
                ImGui::MenuItem("Metrics/Debugger", nullptr, &show_metrics_);
                ImGui::MenuItem("Debug Log", nullptr, &show_debug_log_);
                ImGui::MenuItem("ID Stack Tool", nullptr, &show_id_stack_);
                ImGui::MenuItem("About Dear ImGui", nullptr, &show_about_);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorUi::draw_scene_node(ObjectId id)
{
    const SceneObject* object = state_.find_object(id);
    if (object == nullptr)
        return;
    const std::vector<ObjectId> children = state_.children_of(id);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (state_.selection() == id)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (object->parent == no_object)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::uintptr_t>(id)),
                                        flags, "%s", object->name.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        state_.select(id);
    if (open && !children.empty())
    {
        for (ObjectId child : children)
            draw_scene_node(child);
        ImGui::TreePop();
    }
}

void EditorUi::draw_scene_graph()
{
    bool visible = state_.panel_visible(EditorPanel::scene_graph);
    if (visible)
    {
        ImGui::Begin("Scene Graph", &visible);
        for (ObjectId root : state_.children_of(no_object))
            draw_scene_node(root);
        ImGui::End();
    }
    state_.set_panel_visible(EditorPanel::scene_graph, visible);
}

void EditorUi::draw_object_inspector()
{
    bool visible = state_.panel_visible(EditorPanel::object_inspector);
    if (visible)
    {
        ImGui::Begin("Object Inspector", &visible);
        SceneObject* object = state_.find_object(state_.selection());
        if (object == nullptr)
            ImGui::TextDisabled("Select an object in the Scene Graph.");
        else
        {
            char name[128];
            std::snprintf(name, sizeof(name), "%s", object->name.c_str());
            if (ImGui::InputText("Name", name, sizeof(name)))
                object->name = name;
            ImGui::Text("Type: %s", object->type.c_str());
            ImGui::Checkbox("Enabled", &object->enabled);
            ImGui::SameLine();
            ImGui::Checkbox("Visible", &object->visible);
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position", object->transform.position.data(), 0.1F);
                ImGui::DragFloat3("Rotation", object->transform.rotation.data(), 0.5F);
                ImGui::DragFloat3("Scale", object->transform.scale.data(), 0.05F, 0.01F, 100.0F);
            }
        }
        ImGui::End();
    }
    state_.set_panel_visible(EditorPanel::object_inspector, visible);
}

void EditorUi::draw_viewport()
{
    bool visible = state_.panel_visible(EditorPanel::viewport);
    if (visible)
    {
        ImGui::Begin("Viewport", &visible);
        const SceneObject* selected = state_.find_object(state_.selection());
        ImGui::TextUnformatted("AI3 Editor Viewport");
        ImGui::Text("Selection: %s", selected == nullptr ? "None" : selected->name.c_str());
        const ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::Text("Available: %.0f x %.0f", region.x, region.y);

        const ImVec2 top_left = ImGui::GetCursorScreenPos();
        const ImVec2 bottom_right = {top_left.x + region.x,
                                     top_left.y +
                                         std::max(region.y - ImGui::GetTextLineHeight(), 1.0F)};
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(top_left, bottom_right, IM_COL32(28, 31, 38, 255));
        constexpr float grid_step = 32.0F;
        for (float x = top_left.x; x < bottom_right.x; x += grid_step)
            draw_list->AddLine({x, top_left.y}, {x, bottom_right.y}, IM_COL32(55, 59, 68, 255));
        for (float y = top_left.y; y < bottom_right.y; y += grid_step)
            draw_list->AddLine({top_left.x, y}, {bottom_right.x, y}, IM_COL32(55, 59, 68, 255));
        const char* placeholder = "Scene rendering is intentionally deferred";
        const ImVec2 text_size = ImGui::CalcTextSize(placeholder);
        draw_list->AddText({(top_left.x + bottom_right.x - text_size.x) * 0.5F,
                            (top_left.y + bottom_right.y - text_size.y) * 0.5F},
                           IM_COL32(190, 194, 204, 255), placeholder);
        ImGui::Dummy({region.x, std::max(region.y - ImGui::GetTextLineHeight(), 1.0F)});
        ImGui::End();
    }
    state_.set_panel_visible(EditorPanel::viewport, visible);
}

void EditorUi::draw_console()
{
    bool visible = state_.panel_visible(EditorPanel::console);
    if (visible)
    {
        ImGui::Begin("Console", &visible);
        if (ImGui::Button("Clear"))
            state_.clear_console();
        ImGui::Separator();
        ImGui::BeginChild("ConsoleMessages");
        for (const std::string& message : state_.console_messages())
            ImGui::TextUnformatted(message.c_str());
        ImGui::EndChild();
        ImGui::End();
    }
    state_.set_panel_visible(EditorPanel::console, visible);
}

void EditorUi::draw(bool& running)
{
    draw_main_menu(running);

    constexpr ImGuiID dockspace_id = 0xA13ED170;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool needs_default_layout = ImGui::DockBuilderGetNode(dockspace_id) == nullptr;
    if (needs_default_layout || state_.consume_layout_reset_request())
        build_default_layout(dockspace_id, *viewport);
    ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    draw_scene_graph();
    draw_viewport();
    draw_object_inspector();
    draw_console();

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
