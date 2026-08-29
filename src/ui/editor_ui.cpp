#include "ui/editor_ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/ui_identity.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace ai3
{
namespace
{
struct PanelMenuEntry
{
    const char* key;
    const char* stable_id;
    EditorPanel panel;
};

constexpr std::array<PanelMenuEntry, 4> panel_menu_entries = {
    {{"panel.scene_graph", "ai3_scene_graph", EditorPanel::scene_graph},
     {"panel.viewport", "ai3_viewport", EditorPanel::viewport},
     {"panel.object_inspector", "ai3_object_inspector", EditorPanel::object_inspector},
     {"panel.console", "ai3_console", EditorPanel::console}}};

void build_default_layout(ImGuiID dockspace_id, const ImGuiViewport& viewport)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(dockspace_id, viewport.WorkPos);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport.WorkSize);

    ImGuiID upper = dockspace_id;
    const ImGuiID bottom =
        ImGui::DockBuilderSplitNode(upper, ImGuiDir_Down, 0.27F, nullptr, &upper);
    ImGuiID center = upper;
    const ImGuiID left =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20F, nullptr, &center);
    const ImGuiID right =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24F, nullptr, &center);
    ImGui::DockBuilderDockWindow("###ai3_scene_graph", left);
    ImGui::DockBuilderDockWindow("###ai3_viewport", center);
    ImGui::DockBuilderDockWindow("###ai3_object_inspector", right);
    ImGui::DockBuilderDockWindow("###ai3_console", bottom);
    ImGui::DockBuilderFinish(dockspace_id);
}
} // namespace

EditorUi::EditorUi(Localization& localization, float content_scale, float ui_scale, float font_size)
    : localization_(localization), content_scale_(content_scale), ui_scale_(ui_scale),
      font_size_(font_size)
{
}

void EditorUi::set_scale_diagnostics(float content_scale, float ui_scale, float font_size)
{
    content_scale_ = content_scale;
    ui_scale_ = ui_scale;
    font_size_ = font_size;
}

std::string EditorUi::window_title(std::string_view key, std::string_view stable_id) const
{
    return stable_imgui_label(localization_.text(key), stable_id);
}

void EditorUi::draw_main_menu(bool& running)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(localization_.text("menu.file").c_str()))
        {
            if (ImGui::MenuItem(localization_.text("action.quit").c_str()))
                running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(localization_.text("menu.edit").c_str()))
        {
            ImGui::TextDisabled("%s", localization_.text("editor.no_commands").c_str());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(localization_.text("menu.view").c_str()))
        {
            for (const PanelMenuEntry& entry : panel_menu_entries)
            {
                bool visible = state_.panel_visible(entry.panel);
                const std::string label = window_title(entry.key, entry.stable_id);
                if (ImGui::MenuItem(label.c_str(), nullptr, &visible))
                    state_.set_panel_visible(entry.panel, visible);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(localization_.text("menu.window").c_str()))
        {
            if (ImGui::MenuItem(localization_.text("action.reset_layout").c_str()))
                state_.request_layout_reset();
            ImGui::MenuItem(localization_.text("diagnostics.title").c_str(), nullptr,
                            &show_ai3_diagnostics_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(localization_.text("menu.help").c_str()))
        {
            if (ImGui::BeginMenu(localization_.text("menu.language").c_str()))
            {
                for (const LocaleInfo& locale : localization_.available_locales())
                {
                    const bool selected = locale.id == localization_.active_locale();
                    if (ImGui::MenuItem(locale.name.c_str(), nullptr, selected))
                        localization_.set_locale(locale.id);
                }
                ImGui::EndMenu();
            }
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
        const std::string title = window_title("panel.scene_graph", "ai3_scene_graph");
        ImGui::Begin(title.c_str(), &visible);
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
        const std::string title = window_title("panel.object_inspector", "ai3_object_inspector");
        ImGui::Begin(title.c_str(), &visible);
        SceneObject* object = state_.find_object(state_.selection());
        if (object == nullptr)
            ImGui::TextDisabled("%s", localization_.text("inspector.select_prompt").c_str());
        else
        {
            char name[128];
            std::snprintf(name, sizeof(name), "%s", object->name.c_str());
            const std::string name_label =
                stable_imgui_label(localization_.text("inspector.name"), "inspector_name");
            if (ImGui::InputText(name_label.c_str(), name, sizeof(name)))
                object->name = name;
            ImGui::Text(localization_.text("inspector.type").c_str(), object->type.c_str());
            const std::string enabled_label =
                stable_imgui_label(localization_.text("inspector.enabled"), "inspector_enabled");
            ImGui::Checkbox(enabled_label.c_str(), &object->enabled);
            ImGui::SameLine();
            const std::string visible_label =
                stable_imgui_label(localization_.text("inspector.visible"), "inspector_visible");
            ImGui::Checkbox(visible_label.c_str(), &object->visible);
            const std::string transform_label =
                stable_imgui_label(localization_.text("inspector.transform"), "transform");
            if (ImGui::CollapsingHeader(transform_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                const std::string position_label =
                    stable_imgui_label(localization_.text("inspector.position"), "position");
                const std::string rotation_label =
                    stable_imgui_label(localization_.text("inspector.rotation"), "rotation");
                const std::string scale_label =
                    stable_imgui_label(localization_.text("inspector.scale"), "scale");
                ImGui::DragFloat3(position_label.c_str(), object->transform.position.data(), 0.1F);
                ImGui::DragFloat3(rotation_label.c_str(), object->transform.rotation.data(), 0.5F);
                ImGui::DragFloat3(scale_label.c_str(), object->transform.scale.data(), 0.05F, 0.01F,
                                  100.0F);
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
        const std::string title = window_title("panel.viewport", "ai3_viewport");
        ImGui::Begin(title.c_str(), &visible);
        const SceneObject* selected = state_.find_object(state_.selection());
        ImGui::TextUnformatted(localization_.text("viewport.heading").c_str());
        ImGui::Text(localization_.text("viewport.selection").c_str(),
                    selected == nullptr ? localization_.text("viewport.none").c_str()
                                        : selected->name.c_str());
        const ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::Text(localization_.text("viewport.available").c_str(), region.x, region.y);

        const ImVec2 top_left = ImGui::GetCursorScreenPos();
        const ImVec2 bottom_right = {top_left.x + region.x,
                                     top_left.y +
                                         std::max(region.y - ImGui::GetTextLineHeight(), 1.0F)};
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(top_left, bottom_right, IM_COL32(28, 31, 38, 255));
        const float grid_step = 32.0F * ui_scale_;
        for (float x = top_left.x; x < bottom_right.x; x += grid_step)
            draw_list->AddLine({x, top_left.y}, {x, bottom_right.y}, IM_COL32(55, 59, 68, 255));
        for (float y = top_left.y; y < bottom_right.y; y += grid_step)
            draw_list->AddLine({top_left.x, y}, {bottom_right.x, y}, IM_COL32(55, 59, 68, 255));
        const char* placeholder = localization_.text("viewport.deferred").c_str();
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
        const std::string title = window_title("panel.console", "ai3_console");
        ImGui::Begin(title.c_str(), &visible);
        if (ImGui::Button(localization_.text("action.clear").c_str()))
            state_.clear_console();
        ImGui::Separator();
        ImGui::BeginChild("ConsoleMessages");
        for (const ConsoleMessage& message : state_.console_messages())
        {
            if (message.argument.empty())
                ImGui::TextUnformatted(localization_.text(message.key).c_str());
            else
                ImGui::Text(localization_.text(message.key).c_str(), message.argument.c_str());
        }
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

    if (show_ai3_diagnostics_)
    {
        const std::string title = window_title("diagnostics.title", "ai3_diagnostics");
        ImGui::Begin(title.c_str(), &show_ai3_diagnostics_);
        ImGui::Text(localization_.text("diagnostics.active_locale").c_str(),
                    localization_.active_locale().c_str());
        ImGui::Text(localization_.text("diagnostics.content_scale").c_str(), content_scale_);
        ImGui::Text(localization_.text("diagnostics.ui_scale").c_str(), ui_scale_);
        ImGui::Text(localization_.text("diagnostics.font").c_str(),
                    localization_.font_profile().c_str(), font_size_);
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
