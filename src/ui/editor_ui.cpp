#include "ui/editor_ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "scene/length_units.h"
#include "scene/scene_math.h"
#include "ui/ui_identity.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <iomanip>
#include <sstream>

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

struct LengthUnitEntry
{
    LengthUnit unit;
    const char* localization_key;
};

constexpr std::array<LengthUnitEntry, 4> length_unit_entries = {
    {{LengthUnit::millimeter, "unit.millimeter"},
     {LengthUnit::centimeter, "unit.centimeter"},
     {LengthUnit::meter, "unit.meter"},
     {LengthUnit::kilometer, "unit.kilometer"}}};

constexpr std::array<PanelMenuEntry, 4> panel_menu_entries = {
    {{"panel.scene_graph", "ai3_scene_graph", EditorPanel::scene_graph},
     {"panel.viewport", "ai3_viewport", EditorPanel::viewport},
     {"panel.object_inspector", "ai3_object_inspector", EditorPanel::object_inspector},
     {"panel.console", "ai3_console", EditorPanel::console}}};

std::string decimal(float value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

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
            if (ImGui::MenuItem(localization_.text("action.reset_scene").c_str()))
            {
                state_.reset_scene();
                viewport_renderer_.clear_geometry_cache();
                camera_.reset();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(localization_.text("action.quit").c_str()))
                running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(localization_.text("menu.object").c_str()))
        {
            if (ImGui::MenuItem(localization_.text("action.create_sphere").c_str()))
            {
                const ObjectId sphere = state_.create_sphere(localization_.text("object.sphere"));
                state_.select(sphere);
            }
            if (ImGui::MenuItem(localization_.text("action.create_perspective_camera").c_str()))
            {
                const ObjectId camera =
                    state_.create_perspective_camera(localization_.text("object.camera"));
                state_.select(camera);
            }
            if (ImGui::MenuItem(localization_.text("action.create_directional_light").c_str()))
            {
                const ObjectId light =
                    state_.create_directional_light(localization_.text("object.directional_light"));
                state_.select(light);
            }
            ImGui::Separator();
            const bool has_selection = state_.selection() != no_object;
            if (ImGui::MenuItem(localization_.text("action.delete_selected").c_str(), nullptr,
                                false, has_selection))
                state_.delete_object(state_.selection());
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
            ImGui::Separator();
            if (ImGui::BeginMenu(localization_.text("menu.display_units").c_str()))
            {
                for (const LengthUnitEntry& entry : length_unit_entries)
                {
                    const bool selected = display_length_unit_ == entry.unit;
                    if (ImGui::MenuItem(localization_.text(entry.localization_key).c_str(), nullptr,
                                        selected))
                        display_length_unit_ = entry.unit;
                }
                ImGui::EndMenu();
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
    if (object->parent_id() == no_object)
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
            const char* type_key = "type.object";
            if (object->primitive_kind == PrimitiveKind::sphere)
                type_key = "type.sphere";
            else if (object->camera_kind == CameraKind::perspective)
                type_key = "type.perspective_camera";
            else if (object->light_kind == LightKind::directional)
                type_key = "type.directional_light";
            const std::string type_text =
                localization_.format("inspector.type", {{"type", localization_.text(type_key)}});
            ImGui::TextUnformatted(type_text.c_str());
            const std::string enabled_label =
                stable_imgui_label(localization_.text("inspector.enabled"), "inspector_enabled");
            ImGui::Checkbox(enabled_label.c_str(), &object->enabled);
            ImGui::SameLine();
            const std::string visible_label =
                stable_imgui_label(localization_.text("inspector.visible"), "inspector_visible");
            ImGui::Checkbox(visible_label.c_str(), &object->visible);
            if (object->primitive_kind == PrimitiveKind::sphere)
            {
                float displayed_radius =
                    length_from_meters(object->sphere.radius_meters, display_length_unit_);
                const std::string radius_text = localization_.format(
                    "inspector.radius_with_unit",
                    {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
                const std::string radius_label = stable_imgui_label(radius_text, "sphere_radius");
                const float speed = length_from_meters(0.05F, display_length_unit_);
                const float minimum = length_from_meters(0.001F, display_length_unit_);
                if (ImGui::DragFloat(radius_label.c_str(), &displayed_radius, speed, minimum))
                    state_.set_sphere(object->id,
                                      {std::max(0.001F, length_to_meters(displayed_radius,
                                                                         display_length_unit_))});
            }
            if (object->camera_kind == CameraKind::perspective)
            {
                const std::string camera_label = stable_imgui_label(
                    localization_.text("inspector.camera"), "perspective_camera");
                if (ImGui::CollapsingHeader(camera_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    PerspectiveCamera camera = object->perspective_camera;
                    const std::string fov_label = stable_imgui_label(
                        localization_.text("inspector.vertical_fov"), "camera_vertical_fov");
                    bool changed = ImGui::DragFloat(fov_label.c_str(), &camera.vertical_fov_degrees,
                                                    0.5F, 0.1F, 179.9F);
                    float near_display =
                        length_from_meters(camera.near_plane_meters, display_length_unit_);
                    float far_display =
                        length_from_meters(camera.far_plane_meters, display_length_unit_);
                    const std::string near_text = localization_.format(
                        "inspector.near_plane_with_unit",
                        {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
                    const std::string far_text = localization_.format(
                        "inspector.far_plane_with_unit",
                        {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
                    changed |= ImGui::DragFloat(
                        stable_imgui_label(near_text, "camera_near_plane").c_str(), &near_display,
                        length_from_meters(0.01F, display_length_unit_),
                        length_from_meters(0.001F, display_length_unit_));
                    changed |= ImGui::DragFloat(
                        stable_imgui_label(far_text, "camera_far_plane").c_str(), &far_display,
                        length_from_meters(0.1F, display_length_unit_),
                        length_from_meters(0.002F, display_length_unit_));
                    camera.near_plane_meters =
                        std::max(0.001F, length_to_meters(near_display, display_length_unit_));
                    camera.far_plane_meters =
                        std::max(camera.near_plane_meters + 0.001F,
                                 length_to_meters(far_display, display_length_unit_));
                    if (changed)
                        state_.set_perspective_camera(object->id, camera);
                }
            }
            if (object->light_kind == LightKind::directional)
            {
                const std::string light_label =
                    stable_imgui_label(localization_.text("inspector.light"), "directional_light");
                if (ImGui::CollapsingHeader(light_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DirectionalLight light = object->directional_light;
                    const std::string color_label = stable_imgui_label(
                        localization_.text("inspector.color"), "directional_light_color");
                    const std::string intensity_label = stable_imgui_label(
                        localization_.text("inspector.intensity"), "directional_light_intensity");
                    bool changed =
                        ImGui::ColorEdit3(color_label.c_str(), glm::value_ptr(light.color));
                    changed |=
                        ImGui::DragFloat(intensity_label.c_str(), &light.intensity, 0.05F, 0.0F);
                    light.intensity = std::max(0.0F, light.intensity);
                    if (changed)
                        state_.set_directional_light(object->id, light);
                }
            }
            const std::string transform_label =
                stable_imgui_label(localization_.text("inspector.transform"), "transform");
            if (ImGui::CollapsingHeader(transform_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                const std::string position_text = localization_.format(
                    "inspector.position_with_unit",
                    {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
                const std::string position_label = stable_imgui_label(position_text, "position");
                const std::string rotation_label =
                    stable_imgui_label(localization_.text("inspector.rotation"), "rotation");
                const std::string scale_label =
                    stable_imgui_label(localization_.text("inspector.scale"), "scale");
                glm::vec3 displayed_position{
                    length_from_meters(object->transform.position.x, display_length_unit_),
                    length_from_meters(object->transform.position.y, display_length_unit_),
                    length_from_meters(object->transform.position.z, display_length_unit_)};
                const float position_speed = length_from_meters(0.1F, display_length_unit_);
                if (ImGui::DragFloat3(position_label.c_str(), glm::value_ptr(displayed_position),
                                      position_speed))
                    object->transform.position = {
                        length_to_meters(displayed_position.x, display_length_unit_),
                        length_to_meters(displayed_position.y, display_length_unit_),
                        length_to_meters(displayed_position.z, display_length_unit_)};
                glm::vec3 displayed_rotation =
                    euler_degrees_from_orientation(object->transform.orientation);
                if (ImGui::DragFloat3(rotation_label.c_str(), glm::value_ptr(displayed_rotation),
                                      0.5F))
                    object->transform.orientation =
                        orientation_from_euler_degrees(displayed_rotation);
                ImGui::DragFloat3(scale_label.c_str(), glm::value_ptr(object->transform.scale),
                                  0.05F, 0.01F, 100.0F);
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
        const ImVec2 region = ImGui::GetContentRegionAvail();
        if (region.x > 0.0F && region.y > 0.0F)
        {
            const ImVec2 framebuffer_scale = ImGui::GetIO().DisplayFramebufferScale;
            const RenderTargetSize requested =
                render_target_size(region.x, region.y, framebuffer_scale.x, framebuffer_scale.y);
            viewport_renderer_.render(state_, camera_, requested);
            ImGui::Image(static_cast<ImTextureID>(viewport_renderer_.texture()), region,
                         {0.0F, 1.0F}, {1.0F, 0.0F});
            if (ImGui::IsItemHovered())
            {
                ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    camera_.orbit(io.MouseDelta.x * 0.25F, -io.MouseDelta.y * 0.25F);
                if (io.MouseWheel != 0.0F)
                    camera_.zoom(io.MouseWheel);
            }
        }
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
            {
                const std::string text =
                    localization_.format(message.key, {{"object", message.argument}});
                ImGui::TextUnformatted(text.c_str());
            }
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
        const std::string active_locale = localization_.format(
            "diagnostics.active_locale", {{"locale", localization_.active_locale()}});
        const std::string content_scale = localization_.format(
            "diagnostics.content_scale", {{"scale", decimal(content_scale_, 2)}});
        const std::string ui_scale =
            localization_.format("diagnostics.ui_scale", {{"scale", decimal(ui_scale_, 2)}});
        const std::string font =
            localization_.format("diagnostics.font", {{"profile", localization_.font_profile()},
                                                      {"size", decimal(font_size_, 1)}});
        ImGui::TextUnformatted(active_locale.c_str());
        ImGui::TextUnformatted(content_scale.c_str());
        ImGui::TextUnformatted(ui_scale.c_str());
        ImGui::TextUnformatted(font.c_str());
        ImGui::TextUnformatted(localization_.text("diagnostics.world_coordinates").c_str());
        ImGui::TextUnformatted(localization_.text("diagnostics.canonical_length").c_str());
        const std::string display_unit =
            localization_.format("diagnostics.display_length",
                                 {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
        ImGui::TextUnformatted(display_unit.c_str());
        const RenderTargetSize render_size = viewport_renderer_.size();
        const std::string viewport_size = localization_.format(
            "diagnostics.viewport_size", {{"width", std::to_string(render_size.width)},
                                          {"height", std::to_string(render_size.height)}});
        const std::string renderer_status = localization_.text("diagnostics.renderer_ready");
        const std::string gl_info = localization_.format(
            "diagnostics.gl_info", {{"description", viewport_renderer_.gl_description()}});
        ImGui::TextUnformatted(viewport_size.c_str());
        ImGui::TextUnformatted(renderer_status.c_str());
        ImGui::TextUnformatted(gl_info.c_str());
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
