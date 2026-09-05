#include "ui/editor_ui.h"
#include "editor/scene_document.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "scene/color_space.h"
#include "scene/helper_geometry.h"
#include "scene/length_units.h"
#include "scene/scene_math.h"
#include "scene/translation_gizmo.h"
#include "scene/viewport_picking.h"
#include "ui/ui_identity.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <utility>

namespace ai3
{
enum class SceneDialogKind
{
    open,
    save
};

struct SceneDialogResult
{
    SceneDialogKind kind;
    std::string path;
    std::string error;
};

class SceneDialogState
{
    public:
    std::mutex mutex;
    bool active = false;
    std::optional<SceneDialogResult> result;
    std::string filter_name;
    SDL_DialogFileFilter filter{};
};

namespace
{
struct SceneDialogCallback
{
    std::weak_ptr<SceneDialogState> state;
    SceneDialogKind kind;
};

void SDLCALL scene_dialog_callback(void* userdata, const char* const* files, int)
{
    std::unique_ptr<SceneDialogCallback> callback(static_cast<SceneDialogCallback*>(userdata));
    const std::shared_ptr<SceneDialogState> state = callback->state.lock();
    if (state == nullptr)
        return;
    SceneDialogResult result{callback->kind, {}, {}};
    if (files == nullptr)
        result.error = SDL_GetError();
    else if (files[0] != nullptr)
        result.path = files[0];
    std::lock_guard<std::mutex> lock(state->mutex);
    state->active = false;
    state->result = std::move(result);
}

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

constexpr char scene_object_payload[] = "AI3_SCENE_OBJECT";

std::string decimal(float value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

template <typename Mutation>
void apply_continuous_edit(EditorHistory& history, bool changed, Mutation&& mutation)
{
    if (ImGui::IsItemActivated())
        history.begin_transaction();
    if (changed)
        std::forward<Mutation>(mutation)();
    if (ImGui::IsItemDeactivated())
    {
        if (ImGui::IsItemDeactivatedAfterEdit())
            history.commit_transaction();
        else
            history.cancel_transaction();
    }
}

template <typename Mutation> void apply_discrete_edit(EditorHistory& history, Mutation&& mutation)
{
    if (!history.begin_transaction())
        return;
    std::forward<Mutation>(mutation)();
    history.commit_transaction();
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

EditorUi::EditorUi(EditorState& state, ViewportView& viewport_view, Localization& localization,
                   SDL_Window* window, float content_scale, float ui_scale, float font_size)
    : state_(state), document_session_(state), viewport_view_(viewport_view),
      localization_(localization), window_(window),
      dialog_state_(std::make_shared<SceneDialogState>()), content_scale_(content_scale),
      ui_scale_(ui_scale), font_size_(font_size)
{
}

bool EditorUi::request_quit()
{
    return document_session_.request_transition(DocumentTransition::quit) ==
           TransitionRequestResult::proceed;
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

void EditorUi::report_document_result(std::string_view key, const std::string& detail)
{
    state_.add_console_message(std::string(key), detail);
    state_.set_panel_visible(EditorPanel::console, true);
}

void EditorUi::request_open_dialog()
{
    {
        std::lock_guard<std::mutex> lock(dialog_state_->mutex);
        if (dialog_state_->active)
            return;
        dialog_state_->filter_name = localization_.text("dialog.scene_document_filter");
        dialog_state_->filter = {dialog_state_->filter_name.c_str(), "ai3scene"};
        dialog_state_->active = true;
    }
    auto* callback = new SceneDialogCallback{dialog_state_, SceneDialogKind::open};
    SDL_ShowOpenFileDialog(scene_dialog_callback, callback, window_, &dialog_state_->filter, 1,
                           nullptr, false);
}

void EditorUi::request_save_as_dialog()
{
    finish_translation_gesture();
    {
        std::lock_guard<std::mutex> lock(dialog_state_->mutex);
        if (dialog_state_->active)
            return;
        dialog_state_->filter_name = localization_.text("dialog.scene_document_filter");
        dialog_state_->filter = {dialog_state_->filter_name.c_str(), "ai3scene"};
        dialog_state_->active = true;
    }
    auto* callback = new SceneDialogCallback{dialog_state_, SceneDialogKind::save};
    SDL_ShowSaveFileDialog(scene_dialog_callback, callback, window_, &dialog_state_->filter, 1,
                           nullptr);
}

void EditorUi::save_document()
{
    finish_translation_gesture();
    if (document_session_.document_path().empty())
    {
        request_save_as_dialog();
        return;
    }
    std::string scene_error;
    std::string workspace_error;
    const DocumentSaveResult result = document_session_.save(&scene_error, &workspace_error);
    if (result.scene_saved)
    {
        report_document_result("console.document_saved",
                               document_session_.document_path().string());
        if (!result.workspace_saved)
            report_document_result("console.workspace_error", workspace_error);
        if (document_session_.pending_transition() != DocumentTransition::none)
            ready_transition_ = document_session_.saved_and_take_pending_transition();
    }
    else
    {
        report_document_result("console.document_save_failed", scene_error);
        document_session_.save_failed();
    }
}

void EditorUi::process_dialog_result()
{
    std::optional<SceneDialogResult> result;
    {
        std::lock_guard<std::mutex> lock(dialog_state_->mutex);
        result = std::move(dialog_state_->result);
        dialog_state_->result.reset();
    }
    if (!result.has_value())
        return;
    if (!result->error.empty())
    {
        report_document_result(result->kind == SceneDialogKind::open
                                   ? "console.document_open_failed"
                                   : "console.document_save_failed",
                               result->error);
        if (result->kind == SceneDialogKind::save)
            document_session_.save_failed();
        return;
    }
    if (result->path.empty())
    {
        if (result->kind == SceneDialogKind::save)
            document_session_.cancel_pending_transition();
        return;
    }

    std::filesystem::path path = std::filesystem::u8path(result->path);
    if (result->kind == SceneDialogKind::open)
    {
        std::string error;
        if (!document_session_.open(path, &error))
        {
            report_document_result("console.document_open_failed", error);
            return;
        }
        viewport_view_.reset();
        viewport_renderer_.clear_geometry_cache();
        report_document_result("console.document_opened",
                               document_session_.document_path().string());
        return;
    }

    if (path.extension().empty())
        path += scene_document_extension;
    std::string scene_error;
    std::string workspace_error;
    const DocumentSaveResult save_result =
        document_session_.save_as(path, &scene_error, &workspace_error);
    if (!save_result.scene_saved)
    {
        report_document_result("console.document_save_failed", scene_error);
        document_session_.save_failed();
        return;
    }
    report_document_result("console.document_saved", document_session_.document_path().string());
    if (!save_result.workspace_saved)
        report_document_result("console.workspace_error", workspace_error);
    if (document_session_.pending_transition() != DocumentTransition::none)
        ready_transition_ = document_session_.saved_and_take_pending_transition();
}

void EditorUi::perform_transition(DocumentTransition transition, bool& running)
{
    switch (transition)
    {
    case DocumentTransition::new_document:
        document_session_.new_document();
        viewport_view_.reset();
        viewport_renderer_.clear_geometry_cache();
        break;
    case DocumentTransition::open_document:
        request_open_dialog();
        break;
    case DocumentTransition::quit:
        running = false;
        break;
    case DocumentTransition::none:
        break;
    }
}

void EditorUi::request_transition(DocumentTransition transition, bool& running)
{
    finish_translation_gesture();
    if (document_session_.request_transition(transition) == TransitionRequestResult::proceed)
        perform_transition(transition, running);
}

void EditorUi::draw_unsaved_changes_modal(bool& running)
{
    bool dialog_active = false;
    {
        std::lock_guard<std::mutex> lock(dialog_state_->mutex);
        dialog_active = dialog_state_->active;
    }
    if (document_session_.pending_transition() != DocumentTransition::none && !dialog_active)
        ImGui::OpenPopup("###ai3_unsaved_changes");
    const std::string title =
        stable_imgui_label(localization_.text("dialog.unsaved.title"), "ai3_unsaved_changes");
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    ImGui::TextWrapped("%s", localization_.text("dialog.unsaved.message").c_str());
    if (ImGui::Button(localization_.text("dialog.unsaved.save").c_str()))
    {
        save_document();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(localization_.text("dialog.unsaved.discard").c_str()))
    {
        const DocumentTransition transition =
            document_session_.discard_and_take_pending_transition();
        ImGui::CloseCurrentPopup();
        perform_transition(transition, running);
    }
    ImGui::SameLine();
    if (ImGui::Button(localization_.text("dialog.unsaved.cancel").c_str()))
    {
        document_session_.cancel_pending_transition();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void EditorUi::draw_main_menu(bool& running)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(localization_.text("menu.file").c_str()))
        {
            bool dialog_active = false;
            {
                std::lock_guard<std::mutex> lock(dialog_state_->mutex);
                dialog_active = dialog_state_->active;
            }
            if (ImGui::MenuItem(localization_.text("action.new").c_str(), nullptr, false,
                                !dialog_active))
                request_transition(DocumentTransition::new_document, running);
            if (ImGui::MenuItem(localization_.text("action.open").c_str(), nullptr, false,
                                !dialog_active))
                request_transition(DocumentTransition::open_document, running);
            if (ImGui::MenuItem(localization_.text("action.save").c_str(), nullptr, false,
                                !dialog_active))
                save_document();
            if (ImGui::MenuItem(localization_.text("action.save_as").c_str(), nullptr, false,
                                !dialog_active))
                request_save_as_dialog();
            ImGui::Separator();
            if (ImGui::MenuItem(localization_.text("action.reset_scene").c_str()))
            {
                finish_translation_gesture();
                document_session_.reset_scene();
                viewport_renderer_.clear_geometry_cache();
                viewport_view_.reset();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(localization_.text("action.quit").c_str()))
                request_transition(DocumentTransition::quit, running);
            ImGui::EndMenu();
        }
        EditorHistory& history = document_session_.history();
        if (ImGui::BeginMenu(localization_.text("menu.edit").c_str()))
        {
            if (ImGui::MenuItem(localization_.text("action.undo").c_str(),
                                localization_.text("shortcut.undo").c_str(), false,
                                history.can_undo()))
            {
                history.undo();
                viewport_renderer_.clear_geometry_cache();
            }
            if (ImGui::MenuItem(localization_.text("action.redo").c_str(),
                                localization_.text("shortcut.redo").c_str(), false,
                                history.can_redo()))
            {
                history.redo();
                viewport_renderer_.clear_geometry_cache();
            }
            ImGui::EndMenu();
        }
        if (!ImGui::GetIO().WantTextInput &&
            ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z,
                            ImGuiInputFlags_RouteGlobal))
        {
            if (history.redo())
                viewport_renderer_.clear_geometry_cache();
        }
        else if (!ImGui::GetIO().WantTextInput &&
                 ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
        {
            if (history.undo())
                viewport_renderer_.clear_geometry_cache();
        }
        if (ImGui::BeginMenu(localization_.text("menu.object").c_str()))
        {
            if (ImGui::MenuItem(localization_.text("action.create_sphere").c_str()))
            {
                apply_discrete_edit(history,
                                    [&]
                                    {
                                        const ObjectId sphere = state_.create_sphere(
                                            localization_.text("object.sphere"));
                                        state_.select(sphere);
                                    });
            }
            if (ImGui::MenuItem(localization_.text("action.create_box").c_str()))
            {
                apply_discrete_edit(history,
                                    [&]
                                    {
                                        const ObjectId box =
                                            state_.create_box(localization_.text("object.box"));
                                        state_.select(box);
                                    });
            }
            if (ImGui::MenuItem(localization_.text("action.create_perspective_camera").c_str()))
            {
                apply_discrete_edit(history,
                                    [&]
                                    {
                                        const ObjectId camera = state_.create_perspective_camera(
                                            localization_.text("object.camera"));
                                        state_.select(camera);
                                    });
            }
            if (ImGui::MenuItem(localization_.text("action.create_directional_light").c_str()))
            {
                apply_discrete_edit(history,
                                    [&]
                                    {
                                        const ObjectId light = state_.create_directional_light(
                                            localization_.text("object.directional_light"));
                                        state_.select(light);
                                    });
            }
            ImGui::Separator();
            const bool has_selection = state_.selection() != no_object;
            if (ImGui::MenuItem(localization_.text("action.delete_selected").c_str(), nullptr,
                                false, has_selection))
                apply_discrete_edit(history, [&] { state_.delete_object(state_.selection()); });
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
            ImGui::MenuItem(localization_.text("panel.material_editor").c_str(), nullptr,
                            &show_material_editor_);
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

void EditorUi::draw_editor_toolbar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float toolbar_height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0F;
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::BeginViewportSideBar("###ai3_editor_toolbar", viewport, ImGuiDir_Up, toolbar_height,
                                     flags))
    {
        ImGui::End();
        return;
    }

    const bool selection = viewport_view_.interaction_mode() == ViewportInteractionMode::selection;
    const std::string& selection_text = localization_.text("toolbar.selection");
    const std::string selection_label =
        stable_imgui_label(selection_text, "viewport_interaction_selection");
    const ImVec2 selection_size{ImGui::CalcTextSize(selection_text.c_str()).x +
                                    ImGui::GetStyle().FramePadding.x * 2.0F,
                                ImGui::GetFrameHeight()};
    if (ImGui::Selectable(selection_label.c_str(), selection, 0, selection_size))
        viewport_view_.set_interaction_mode(ViewportInteractionMode::selection);
    ImGui::SameLine();
    const bool navigation =
        viewport_view_.interaction_mode() == ViewportInteractionMode::navigation;
    const std::string& navigation_text = localization_.text("toolbar.navigation");
    const std::string navigation_label =
        stable_imgui_label(navigation_text, "viewport_interaction_navigation");
    const ImVec2 navigation_size{ImGui::CalcTextSize(navigation_text.c_str()).x +
                                     ImGui::GetStyle().FramePadding.x * 2.0F,
                                 ImGui::GetFrameHeight()};
    if (ImGui::Selectable(navigation_label.c_str(), navigation, 0, navigation_size))
        viewport_view_.set_interaction_mode(ViewportInteractionMode::navigation);
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    if (selection)
    {
        const std::string& translate_text = localization_.text("toolbar.translate");
        const std::string translate_label =
            stable_imgui_label(translate_text, "viewport_transform_translate");
        const ImVec2 translate_size{ImGui::CalcTextSize(translate_text.c_str()).x +
                                        ImGui::GetStyle().FramePadding.x * 2.0F,
                                    ImGui::GetFrameHeight()};
        if (ImGui::Selectable(translate_label.c_str(), true, 0, translate_size))
            viewport_view_.set_transform_tool(ViewportTransformTool::translation);
        ImGui::SameLine();
        const std::array<std::pair<CoordinateSpace, const char*>, 4> spaces = {
            {{CoordinateSpace::local, "space.local"},
             {CoordinateSpace::parent, "space.parent"},
             {CoordinateSpace::world, "space.world"},
             {CoordinateSpace::view, "space.view"}}};
        ImGui::Text("%s:", localization_.text("toolbar.reference_space").c_str());
        ImGui::SameLine();
        constexpr const char* space_combo_id = "###viewport_reference_space";
        const auto current =
            std::find_if(spaces.begin(), spaces.end(), [&](const auto& entry)
                         { return entry.first == viewport_view_.reference_space(); });
        ImGui::SetNextItemWidth(9.0F * font_size_);
        if (ImGui::BeginCombo(space_combo_id, localization_.text(current->second).c_str()))
        {
            for (const auto& [space, key] : spaces)
            {
                const bool selected = viewport_view_.reference_space() == space;
                if (ImGui::Selectable(localization_.text(key).c_str(), selected))
                    viewport_view_.set_reference_space(space);
            }
            ImGui::EndCombo();
        }
    }
    else
        ImGui::TextDisabled("%s", localization_.text("toolbar.navigation_context").c_str());
    ImGui::End();
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
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload(scene_object_payload, &id, sizeof(id));
        ImGui::TextUnformatted(object->name.c_str());
        ImGui::EndDragDropSource();
    }
    accept_reparent_drop(id);
    if (open && !children.empty())
    {
        for (ObjectId child : children)
            draw_scene_node(child);
        ImGui::TreePop();
    }
}

void EditorUi::accept_reparent_drop(ObjectId new_parent)
{
    if (!ImGui::BeginDragDropTarget())
        return;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(scene_object_payload);
        payload != nullptr && payload->DataSize == sizeof(ObjectId))
    {
        const ObjectId dragged = *static_cast<const ObjectId*>(payload->Data);
        pending_reparent_ = {dragged, new_parent};
    }
    ImGui::EndDragDropTarget();
}

void EditorUi::apply_pending_reparent()
{
    const auto [dragged, new_parent] = pending_reparent_;
    if (dragged == no_object)
        return;
    pending_reparent_ = {no_object, no_object};
    const SceneObject* object = state_.find_object(dragged);
    if (object == nullptr)
        return;
    const std::string object_name = object->name;
    EditorHistory& history = document_session_.history();
    history.begin_transaction();
    if (!state_.reparent_object(dragged, new_parent))
    {
        history.cancel_transaction();
        state_.add_console_message("console.reparent_rejected", object_name);
        state_.set_panel_visible(EditorPanel::console, true);
    }
    else
        history.commit_transaction();
}

void EditorUi::draw_scene_graph()
{
    bool visible = state_.panel_visible(EditorPanel::scene_graph);
    if (visible)
    {
        const std::string title = window_title("panel.scene_graph", "ai3_scene_graph");
        ImGui::Begin(title.c_str(), &visible);
        const std::string root_label =
            stable_imgui_label(localization_.text("scene_graph.root"), "scene_graph_root");
        ImGui::Selectable(root_label.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth);
        accept_reparent_drop(no_object);
        ImGui::Separator();
        for (ObjectId root : state_.children_of(no_object))
            draw_scene_node(root);
        apply_pending_reparent();
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
        const SceneObject* object = state_.find_object(state_.selection());
        if (object == nullptr)
            ImGui::TextDisabled("%s", localization_.text("inspector.select_prompt").c_str());
        else
        {
            char name[128];
            std::snprintf(name, sizeof(name), "%s", object->name.c_str());
            const std::string name_label =
                stable_imgui_label(localization_.text("inspector.name"), "inspector_name");
            const bool name_changed = ImGui::InputText(name_label.c_str(), name, sizeof(name));
            apply_continuous_edit(document_session_.history(), name_changed,
                                  [&] { state_.rename_object(object->id, name); });
            const char* type_key = "type.object";
            if (object->primitive_kind == PrimitiveKind::sphere)
                type_key = "type.sphere";
            else if (object->primitive_kind == PrimitiveKind::box)
                type_key = "type.box";
            else if (object->camera_kind == CameraKind::perspective)
                type_key = "type.perspective_camera";
            else if (object->light_kind == LightKind::directional)
                type_key = "type.directional_light";
            const std::string type_text =
                localization_.format("inspector.type", {{"type", localization_.text(type_key)}});
            ImGui::TextUnformatted(type_text.c_str());
            const std::string enabled_label =
                stable_imgui_label(localization_.text("inspector.enabled"), "inspector_enabled");
            bool enabled = object->enabled;
            if (ImGui::Checkbox(enabled_label.c_str(), &enabled))
                apply_discrete_edit(document_session_.history(),
                                    [&] { state_.set_object_enabled(object->id, enabled); });
            ImGui::SameLine();
            const std::string visible_label =
                stable_imgui_label(localization_.text("inspector.visible"), "inspector_visible");
            bool object_visible = object->visible;
            if (ImGui::Checkbox(visible_label.c_str(), &object_visible))
                apply_discrete_edit(document_session_.history(),
                                    [&] { state_.set_object_visible(object->id, object_visible); });
            if (object->primitive_kind == PrimitiveKind::sphere)
            {
                BoundsDisplayState bounds_display = state_.bounds_display(object->id);
                bool workspace_changed = ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.show_bounding_box"),
                                       "show_bounding_box")
                        .c_str(),
                    &bounds_display.show_bounding_box);
                workspace_changed |= ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.show_bounding_sphere"),
                                       "show_bounding_sphere")
                        .c_str(),
                    &bounds_display.show_bounding_sphere);
                workspace_changed |= ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.hover_feedback"),
                                       "hover_feedback")
                        .c_str(),
                    &bounds_display.hover_feedback);
                if (workspace_changed)
                    document_session_.set_bounds_display(object->id, bounds_display);
                float displayed_radius =
                    length_from_meters(object->sphere.radius_meters, display_length_unit_);
                const std::string radius_text = localization_.format(
                    "inspector.radius_with_unit",
                    {{"unit", std::string(length_unit_symbol(display_length_unit_))}});
                const std::string radius_label = stable_imgui_label(radius_text, "sphere_radius");
                const float speed = length_from_meters(0.05F, display_length_unit_);
                const float minimum = length_from_meters(0.001F, display_length_unit_);
                const bool changed =
                    ImGui::DragFloat(radius_label.c_str(), &displayed_radius, speed, minimum);
                apply_continuous_edit(
                    document_session_.history(), changed,
                    [&]
                    {
                        SpherePrimitive sphere = object->sphere;
                        sphere.radius_meters = std::max(
                            0.001F, length_to_meters(displayed_radius, display_length_unit_));
                        state_.set_sphere(object->id, sphere);
                    });
                const Material* assigned = state_.find_material(object->sphere.material_id);
                const std::string material_name =
                    assigned == nullptr ? localization_.text("material.none") : assigned->name;
                ImGui::Text("%s: %s", localization_.text("inspector.material").c_str(),
                            material_name.c_str());
                glm::vec3 fallback_srgb = linear_to_srgb(object->sphere.fallback_color);
                const bool fallback_changed = ImGui::ColorEdit3(
                    stable_imgui_label(localization_.text("inspector.fallback_color"),
                                       "sphere_fallback_color")
                        .c_str(),
                    glm::value_ptr(fallback_srgb));
                apply_continuous_edit(document_session_.history(), fallback_changed,
                                      [&]
                                      {
                                          state_.set_sphere_fallback_color(
                                              object->id, srgb_to_linear(fallback_srgb));
                                      });
            }
            if (object->primitive_kind == PrimitiveKind::box)
            {
                BoundsDisplayState display = state_.bounds_display(object->id);
                bool workspace_changed = ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.show_bounding_box"),
                                       "box_bounds")
                        .c_str(),
                    &display.show_bounding_box);
                workspace_changed |= ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.show_bounding_sphere"),
                                       "box_sphere")
                        .c_str(),
                    &display.show_bounding_sphere);
                workspace_changed |= ImGui::Checkbox(
                    stable_imgui_label(localization_.text("inspector.hover_feedback"), "box_hover")
                        .c_str(),
                    &display.hover_feedback);
                if (workspace_changed)
                    document_session_.set_bounds_display(object->id, display);
                BoxPrimitive box = object->box;
                const auto dimension = [&](const char* key, const char* stable, float& value)
                {
                    float shown = length_from_meters(value, display_length_unit_);
                    const bool changed = ImGui::DragFloat(
                        stable_imgui_label(
                            localization_.format(key, {{"unit", std::string(length_unit_symbol(
                                                                    display_length_unit_))}}),
                            stable)
                            .c_str(),
                        &shown, length_from_meters(0.05F, display_length_unit_),
                        length_from_meters(0.001F, display_length_unit_),
                        length_from_meters(9999.0F, display_length_unit_));
                    if (changed)
                        value = std::clamp(length_to_meters(shown, display_length_unit_), 0.001F,
                                           9999.0F);
                    return changed;
                };
                bool changed =
                    dimension("inspector.width_with_unit", "box_width", box.width_meters);
                changed |= dimension("inspector.length_with_unit", "box_length", box.length_meters);
                changed |= dimension("inspector.height_with_unit", "box_height", box.height_meters);
                changed |= ImGui::DragInt(
                    stable_imgui_label(localization_.text("inspector.width_segments"),
                                       "box_width_segments")
                        .c_str(),
                    &box.width_segments, 1, 1, 999);
                box.width_segments = std::clamp(box.width_segments, 1, 999);
                changed |= ImGui::DragInt(
                    stable_imgui_label(localization_.text("inspector.length_segments"),
                                       "box_length_segments")
                        .c_str(),
                    &box.length_segments, 1, 1, 999);
                box.length_segments = std::clamp(box.length_segments, 1, 999);
                changed |= ImGui::DragInt(
                    stable_imgui_label(localization_.text("inspector.height_segments"),
                                       "box_height_segments")
                        .c_str(),
                    &box.height_segments, 1, 1, 999);
                box.height_segments = std::clamp(box.height_segments, 1, 999);
                apply_continuous_edit(document_session_.history(), changed,
                                      [&] { state_.set_box(object->id, box); });
                const Material* assigned = state_.find_material(box.material_id);
                ImGui::Text("%s: %s", localization_.text("inspector.material").c_str(),
                            assigned == nullptr ? localization_.text("material.none").c_str()
                                                : assigned->name.c_str());
                glm::vec3 fallback_srgb = linear_to_srgb(box.fallback_color);
                const bool fallback_changed = ImGui::ColorEdit3(
                    stable_imgui_label(localization_.text("inspector.fallback_color"),
                                       "box_fallback_color")
                        .c_str(),
                    glm::value_ptr(fallback_srgb));
                apply_continuous_edit(document_session_.history(), fallback_changed,
                                      [&]
                                      {
                                          BoxPrimitive changed_box = object->box;
                                          changed_box.fallback_color =
                                              srgb_to_linear(fallback_srgb);
                                          state_.set_box(object->id, changed_box);
                                      });
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
                    apply_continuous_edit(document_session_.history(), changed, [&]
                                          { state_.set_perspective_camera(object->id, camera); });
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
                    changed = ImGui::DragFloat(
                        stable_imgui_label(near_text, "camera_near_plane").c_str(), &near_display,
                        length_from_meters(0.01F, display_length_unit_),
                        length_from_meters(0.001F, display_length_unit_));
                    camera.near_plane_meters =
                        std::max(0.001F, length_to_meters(near_display, display_length_unit_));
                    camera.far_plane_meters =
                        std::max(camera.near_plane_meters + 0.001F, camera.far_plane_meters);
                    apply_continuous_edit(document_session_.history(), changed, [&]
                                          { state_.set_perspective_camera(object->id, camera); });
                    camera = state_.find_object(object->id)->perspective_camera;
                    near_display =
                        length_from_meters(camera.near_plane_meters, display_length_unit_);
                    changed = ImGui::DragFloat(
                        stable_imgui_label(far_text, "camera_far_plane").c_str(), &far_display,
                        length_from_meters(0.1F, display_length_unit_),
                        length_from_meters(0.002F, display_length_unit_));
                    camera.far_plane_meters =
                        std::max(camera.near_plane_meters + 0.001F,
                                 length_to_meters(far_display, display_length_unit_));
                    apply_continuous_edit(document_session_.history(), changed, [&]
                                          { state_.set_perspective_camera(object->id, camera); });
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
                    glm::vec3 color_srgb = linear_to_srgb(light.color);
                    bool changed =
                        ImGui::ColorEdit3(color_label.c_str(), glm::value_ptr(color_srgb));
                    apply_continuous_edit(document_session_.history(), changed,
                                          [&]
                                          {
                                              light.color = srgb_to_linear(color_srgb);
                                              state_.set_directional_light(object->id, light);
                                          });
                    light = state_.find_object(object->id)->directional_light;
                    changed =
                        ImGui::DragFloat(intensity_label.c_str(), &light.intensity, 0.05F, 0.0F);
                    light.intensity = std::max(0.0F, light.intensity);
                    apply_continuous_edit(document_session_.history(), changed,
                                          [&] { state_.set_directional_light(object->id, light); });
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
                Transform transform = object->transform;
                bool transform_changed = false;
                glm::vec3 displayed_position{
                    length_from_meters(transform.position.x, display_length_unit_),
                    length_from_meters(transform.position.y, display_length_unit_),
                    length_from_meters(transform.position.z, display_length_unit_)};
                const float position_speed = length_from_meters(0.1F, display_length_unit_);
                if (ImGui::DragFloat3(position_label.c_str(), glm::value_ptr(displayed_position),
                                      position_speed))
                {
                    transform.position = {
                        length_to_meters(displayed_position.x, display_length_unit_),
                        length_to_meters(displayed_position.y, display_length_unit_),
                        length_to_meters(displayed_position.z, display_length_unit_)};
                    transform_changed = true;
                }
                apply_continuous_edit(document_session_.history(), transform_changed,
                                      [&] { state_.set_local_transform(object->id, transform); });
                transform = state_.find_object(object->id)->transform;
                transform_changed = false;
                glm::vec3 displayed_rotation =
                    euler_degrees_from_orientation(transform.orientation);
                if (ImGui::DragFloat3(rotation_label.c_str(), glm::value_ptr(displayed_rotation),
                                      0.5F))
                {
                    transform.orientation = orientation_from_euler_degrees(displayed_rotation);
                    transform_changed = true;
                }
                apply_continuous_edit(document_session_.history(), transform_changed,
                                      [&] { state_.set_local_transform(object->id, transform); });
                transform = state_.find_object(object->id)->transform;
                transform_changed = ImGui::DragFloat3(
                    scale_label.c_str(), glm::value_ptr(transform.scale), 0.05F, 0.01F, 100.0F);
                apply_continuous_edit(document_session_.history(), transform_changed,
                                      [&] { state_.set_local_transform(object->id, transform); });
            }
        }
        ImGui::End();
    }
    state_.set_panel_visible(EditorPanel::object_inspector, visible);
}

void EditorUi::draw_material_editor()
{
    if (!show_material_editor_)
        return;
    const std::string title = window_title("panel.material_editor", "ai3_material_editor");
    ImGui::SetNextWindowSize(ImVec2{380.0F * ui_scale_, 420.0F * ui_scale_},
                             ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title.c_str(), &show_material_editor_, ImGuiWindowFlags_NoDocking))
    {
        ImGui::End();
        return;
    }
    EditorHistory& history = document_session_.history();
    if (state_.find_material(active_material_id_) == nullptr && !state_.materials().empty())
        active_material_id_ = state_.materials().front().id;
    const Material* active_material = state_.find_material(active_material_id_);
    const std::string active_name =
        active_material == nullptr ? localization_.text("material.none") : active_material->name;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(localization_.text("material.instance").c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() * 4.0F);
    if (ImGui::BeginCombo(stable_imgui_label("", "active_material").c_str(), active_name.c_str()))
    {
        for (const Material& candidate : state_.materials())
        {
            ImGui::PushID(reinterpret_cast<void*>(static_cast<std::uintptr_t>(candidate.id)));
            if (ImGui::Selectable(candidate.name.c_str(), candidate.id == active_material_id_))
                active_material_id_ = candidate.id;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button(localization_.text("material.new").c_str()))
        apply_discrete_edit(history,
                            [&]
                            {
                                active_material_id_ = state_.create_material(
                                    localization_.text("material.default_name"));
                            });
    if (state_.materials().empty())
    {
        ImGui::End();
        return;
    }
    Material material = *state_.find_material(active_material_id_);
    char name[128];
    std::snprintf(name, sizeof(name), "%s", material.name.c_str());
    const bool name_changed = ImGui::InputText(
        stable_imgui_label(localization_.text("material.name"), "material_name").c_str(), name,
        sizeof(name));
    apply_continuous_edit(history, name_changed,
                          [&] { state_.rename_material(material.id, name); });
    material = *state_.find_material(active_material_id_);
    const char* shading =
        material.shading == MaterialShading::lambert ? "material.lambert" : "material.phong";
    if (ImGui::BeginCombo(
            stable_imgui_label(localization_.text("material.shading"), "material_shading").c_str(),
            localization_.text(shading).c_str()))
    {
        for (MaterialShading value : {MaterialShading::lambert, MaterialShading::phong})
        {
            const char* key =
                value == MaterialShading::lambert ? "material.lambert" : "material.phong";
            if (ImGui::Selectable(localization_.text(key).c_str(), material.shading == value))
                apply_discrete_edit(history,
                                    [&]
                                    {
                                        material.shading = value;
                                        state_.set_material(material.id, material);
                                    });
        }
        ImGui::EndCombo();
    }
    material = *state_.find_material(active_material_id_);
    const auto color_control = [&](const char* key, const char* stable, glm::vec3 Material::* field)
    {
        glm::vec3 srgb = linear_to_srgb(material.*field);
        const bool changed = ImGui::ColorEdit3(
            stable_imgui_label(localization_.text(key), stable).c_str(), glm::value_ptr(srgb));
        apply_continuous_edit(history, changed,
                              [&]
                              {
                                  Material changed_material =
                                      *state_.find_material(active_material_id_);
                                  changed_material.*field = srgb_to_linear(srgb);
                                  state_.set_material(active_material_id_, changed_material);
                              });
    };
    color_control("material.ambient", "material_ambient", &Material::ambient_color);
    color_control("material.diffuse", "material_diffuse", &Material::diffuse_color);
    if (material.shading == MaterialShading::phong)
    {
        color_control("material.specular", "material_specular", &Material::specular_color);
        material = *state_.find_material(active_material_id_);
        const bool changed = ImGui::DragFloat(
            stable_imgui_label(localization_.text("material.shininess"), "material_shininess")
                .c_str(),
            &material.specular_power, 1.0F, 1.0F, 1024.0F);
        apply_continuous_edit(history, changed,
                              [&] { state_.set_material(material.id, material); });
    }
    const SceneObject* selected = state_.find_object(state_.selection());
    const bool assignable =
        selected != nullptr && (selected->primitive_kind == PrimitiveKind::sphere ||
                                selected->primitive_kind == PrimitiveKind::box);
    ImGui::BeginDisabled(!assignable);
    if (ImGui::Button(localization_.text("material.assign_selected").c_str()))
        apply_discrete_edit(history,
                            [&] { state_.assign_material(selected->id, active_material_id_); });
    ImGui::EndDisabled();
    ImGui::End();
}

void EditorUi::draw_viewport()
{
    bool visible = state_.panel_visible(EditorPanel::viewport);
    if (visible)
    {
        const std::string title = window_title("panel.viewport", "ai3_viewport");
        ImGui::Begin(title.c_str(), &visible);
        const std::string source_label =
            stable_imgui_label(localization_.text("viewport.view_source"), "viewport_view_source");
        const SceneObject* current_camera = state_.find_object(viewport_view_.scene_camera_id());
        const std::string current_source =
            viewport_view_.source() == ViewSource::editor_view || current_camera == nullptr
                ? localization_.text("viewport.view_editor")
                : current_camera->name;
        if (ImGui::BeginCombo(source_label.c_str(), current_source.c_str()))
        {
            const bool orbit_selected = viewport_view_.source() == ViewSource::editor_view;
            if (ImGui::Selectable(localization_.text("viewport.view_editor").c_str(),
                                  orbit_selected))
                viewport_view_.use_editor_view();
            for (const SceneObject* camera : state_.cameras(CameraKind::perspective))
            {
                ImGui::PushID(reinterpret_cast<void*>(static_cast<std::uintptr_t>(camera->id)));
                const bool selected = viewport_view_.source() == ViewSource::scene_camera &&
                                      viewport_view_.scene_camera_id() == camera->id;
                if (ImGui::Selectable(camera->name.c_str(), selected))
                    viewport_view_.use_scene_camera(state_, camera->id);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        const ImVec2 region = ImGui::GetContentRegionAvail();
        if (region.x > 0.0F && region.y > 0.0F)
        {
            const ImVec2 framebuffer_scale = ImGui::GetIO().DisplayFramebufferScale;
            const RenderTargetSize requested =
                render_target_size(region.x, region.y, framebuffer_scale.x, framebuffer_scale.y);
            const float aspect_ratio =
                static_cast<float>(requested.width) / static_cast<float>(requested.height);
            const ResolvedViewportView resolved = viewport_view_.resolve(state_, aspect_ratio);
            if (viewport_view_.source() != ViewSource::editor_view)
                transient_navigation_gesture_.release();
            const ObjectId helper_object_id = translation_gesture_.has_value()
                                                  ? translation_gesture_->object_id
                                                  : state_.selection();
            const SceneObject* helper_object = state_.find_object(helper_object_id);
            glm::vec3 helper_pivot{};
            glm::mat3 helper_basis{1.0F};
            if (helper_object != nullptr)
            {
                helper_pivot = state_.world_position(helper_object->id);
                helper_basis =
                    translation_gesture_.has_value()
                        ? translation_gesture_->frozen_basis
                        : coordinate_space_basis(state_, helper_object->id,
                                                 viewport_view_.reference_space(), resolved.view);
            }
            const ImVec2 future_minimum = ImGui::GetCursorScreenPos();
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            if (!transient_navigation_gesture_.active() &&
                viewport_view_.interaction_mode() == ViewportInteractionMode::selection &&
                mouse.x >= future_minimum.x && mouse.y >= future_minimum.y &&
                mouse.x < future_minimum.x + region.x && mouse.y < future_minimum.y + region.y)
            {
                const glm::vec2 coordinates{(mouse.x - future_minimum.x) / region.x,
                                            (mouse.y - future_minimum.y) / region.y};
                hovered_object_ = viewport_view_.helper_hover_object(
                    pick_primitive(state_, viewport_world_ray(coordinates, resolved)));
            }
            else
                hovered_object_ = no_object;
            int highlighted = translation_gesture_.has_value()
                                  ? static_cast<int>(translation_gesture_->selected_axis)
                                  : -1;
            if (helper_object != nullptr && !translation_gesture_.has_value() &&
                !transient_navigation_gesture_.active() &&
                viewport_view_.interaction_mode() == ViewportInteractionMode::selection)
            {
                const auto projected = project_translation_gizmo(
                    helper_pivot, helper_basis, resolved, {region.x, region.y}, 72.0F * ui_scale_);
                if (projected)
                    highlighted = static_cast<int>(pick_translation_axis(
                        {mouse.x - future_minimum.x, mouse.y - future_minimum.y}, *projected,
                        10.0F * ui_scale_));
            }
            const ResolvedViewportView& helper_gizmo_view =
                translation_gesture_.has_value() ? translation_gesture_->frozen_view : resolved;
            const glm::vec2 helper_gizmo_viewport = translation_gesture_.has_value()
                                                        ? translation_gesture_->frozen_viewport_size
                                                        : glm::vec2{region.x, region.y};
            const float helper_gizmo_length = translation_gesture_.has_value()
                                                  ? translation_gesture_->frozen_screen_axis_length
                                                  : 72.0F * ui_scale_;
            const ObjectId helper_id = helper_object == nullptr ? no_object : helper_object->id;
            const HelperGeometry bounds_helpers =
                resolve_bounds_helper_geometry(state_, helper_id, hovered_object_);
            const HelperGeometry gizmo_helpers = resolve_translation_helper_geometry(
                helper_id, helper_pivot, helper_basis, helper_gizmo_view, helper_gizmo_viewport,
                helper_gizmo_length, highlighted);
            const ViewportHelperInputs helpers{&bounds_helpers, &gizmo_helpers, &helper_gizmo_view};
            viewport_renderer_.render(state_, resolved, requested, helpers);
            ImGui::Image(static_cast<ImTextureID>(viewport_renderer_.texture()), region,
                         {0.0F, 1.0F}, {1.0F, 0.0F});
            const ImVec2 minimum = ImGui::GetItemRectMin();
            const glm::vec2 viewport_origin{minimum.x, minimum.y};
            const glm::vec2 viewport_size{region.x, region.y};
            ImGuiIO& io = ImGui::GetIO();
            const bool translation_owned_input = translation_gesture_.has_value();

            if (translation_gesture_.has_value())
            {
                AxisTranslationGesture& gesture = *translation_gesture_;
                if (!viewport_geometry_matches(gesture.frozen_viewport_origin,
                                               gesture.frozen_viewport_size, viewport_origin,
                                               viewport_size) ||
                    ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                    state_.find_object(gesture.object_id) == nullptr)
                    cancel_translation_gesture();
                else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    const glm::vec2 coordinates{(io.MousePos.x - gesture.frozen_viewport_origin.x) /
                                                    gesture.frozen_viewport_size.x,
                                                (io.MousePos.y - gesture.frozen_viewport_origin.y) /
                                                    gesture.frozen_viewport_size.y};
                    const WorldRay ray = viewport_world_ray(coordinates, gesture.frozen_view);
                    const std::optional<glm::vec3> desired =
                        constrained_axis_position(gesture.constraint, ray);
                    if (desired.has_value() &&
                        !state_.set_world_position(gesture.object_id, *desired))
                        cancel_translation_gesture();
                }
                else
                    finish_translation_gesture();
            }

            if (!translation_owned_input && !translation_gesture_.has_value())
            {
                if (!transient_navigation_gesture_.active() && ImGui::IsItemHovered() &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Middle) &&
                    viewport_view_.source() == ViewSource::editor_view)
                    transient_navigation_gesture_.acquire(io.KeyShift);
                if (transient_navigation_gesture_.active())
                {
                    if (viewport_view_.source() != ViewSource::editor_view ||
                        !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                        transient_navigation_gesture_.release();
                    else
                        transient_navigation_gesture_.dispatch(
                            viewport_view_, {io.MouseDelta.x, io.MouseDelta.y}, region.y);
                }
            }

            const ObjectId gizmo_object = translation_gesture_.has_value()
                                              ? translation_gesture_->object_id
                                              : state_.selection();
            const SceneObject* selected = state_.find_object(gizmo_object);
            std::optional<ProjectedTranslationGizmo> projected_gizmo;
            glm::vec3 pivot{};
            glm::mat3 basis{1.0F};
            if (selected != nullptr)
            {
                pivot = state_.world_position(selected->id);
                basis =
                    translation_gesture_.has_value()
                        ? translation_gesture_->frozen_basis
                        : coordinate_space_basis(state_, selected->id,
                                                 viewport_view_.reference_space(), resolved.view);
                const float screen_axis_length =
                    translation_gesture_.has_value()
                        ? translation_gesture_->frozen_screen_axis_length
                        : 72.0F * ui_scale_;
                const ResolvedViewportView& presentation_view =
                    translation_gesture_.has_value() ? translation_gesture_->frozen_view : resolved;
                projected_gizmo = project_translation_gizmo(pivot, basis, presentation_view,
                                                            viewport_size, screen_axis_length);
            }

            bool acquired_gizmo = false;
            TranslationAxis hovered_axis = TranslationAxis::none;
            if (projected_gizmo.has_value() && !translation_gesture_.has_value() &&
                !transient_navigation_gesture_.active() && ImGui::IsItemHovered())
            {
                const glm::vec2 pointer{io.MousePos.x - minimum.x, io.MousePos.y - minimum.y};
                hovered_axis = pick_translation_axis(pointer, *projected_gizmo, 10.0F * ui_scale_);
            }
            if (projected_gizmo.has_value() && !translation_gesture_.has_value() &&
                !transient_navigation_gesture_.active() &&
                viewport_view_.interaction_mode() == ViewportInteractionMode::selection &&
                ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const glm::vec2 pointer{io.MousePos.x - minimum.x, io.MousePos.y - minimum.y};
                const float hit_tolerance = 10.0F * ui_scale_;
                const TranslationAxis axis = hovered_axis;
                if (axis != TranslationAxis::none)
                {
                    const std::size_t index = static_cast<std::size_t>(axis);
                    const glm::vec2 coordinates{pointer.x / region.x, pointer.y / region.y};
                    const WorldRay ray = viewport_world_ray(coordinates, resolved);
                    const glm::vec3 direction = glm::normalize(basis[index]);
                    const AxisDragConstraint constraint =
                        begin_axis_drag_constraint(ray, pivot, direction, resolved);
                    if (constraint.valid && document_session_.history().begin_transaction())
                    {
                        translation_gesture_ =
                            AxisTranslationGesture{selected->id,  axis,
                                                   pivot,         direction,
                                                   basis,         72.0F * ui_scale_,
                                                   hit_tolerance, viewport_origin,
                                                   viewport_size, resolved,
                                                   constraint};
                        acquired_gizmo = true;
                    }
                }
            }

            if (!translation_owned_input && ImGui::IsItemHovered() &&
                !translation_gesture_.has_value() && !transient_navigation_gesture_.active())
            {
                if (io.MouseWheel != 0.0F)
                    viewport_view_.zoom(io.MouseWheel);
                if (viewport_view_.interaction_mode() == ViewportInteractionMode::navigation)
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                        viewport_view_.navigate(io.MouseDelta.x * 0.25F, -io.MouseDelta.y * 0.25F);
                }
                else if (!acquired_gizmo && !translation_gesture_.has_value() &&
                         ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    const ImVec2 maximum = ImGui::GetItemRectMax();
                    const glm::vec2 coordinates{
                        (io.MousePos.x - minimum.x) / (maximum.x - minimum.x),
                        (io.MousePos.y - minimum.y) / (maximum.y - minimum.y)};
                    const ObjectId hit =
                        pick_primitive(state_, viewport_world_ray(coordinates, resolved));
                    if (hit == no_object)
                        state_.clear_selection();
                    else
                        state_.select(hit);
                }
            }
        }
        else
            transient_navigation_gesture_.release();
        ImGui::End();
        if (!visible)
            transient_navigation_gesture_.release();
    }
    else
        transient_navigation_gesture_.release();
    state_.set_panel_visible(EditorPanel::viewport, visible);
}

void EditorUi::cancel_translation_gesture()
{
    if (!translation_gesture_.has_value())
        return;
    document_session_.history().cancel_transaction();
    translation_gesture_.reset();
}

void EditorUi::finish_translation_gesture()
{
    if (!translation_gesture_.has_value())
        return;
    document_session_.history().commit_transaction();
    translation_gesture_.reset();
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
                    localization_.format(message.key, {{"object", message.argument},
                                                       {"detail", message.argument},
                                                       {"path", message.argument}});
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
    process_dialog_result();
    if (ready_transition_ != DocumentTransition::none)
    {
        const DocumentTransition transition =
            std::exchange(ready_transition_, DocumentTransition::none);
        perform_transition(transition, running);
    }
    const std::string document_name = document_session_.document_path().empty()
                                          ? localization_.text("document.untitled")
                                          : document_session_.document_path().filename().string();
    const std::string application_title =
        "AI3 - " + document_name + (document_session_.dirty() ? " *" : "");
    SDL_SetWindowTitle(window_, application_title.c_str());
    draw_main_menu(running);
    draw_unsaved_changes_modal(running);
    draw_editor_toolbar();

    constexpr ImGuiID dockspace_id = 0xA13ED170;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool needs_default_layout = ImGui::DockBuilderGetNode(dockspace_id) == nullptr;
    if (needs_default_layout || state_.consume_layout_reset_request())
        build_default_layout(dockspace_id, *viewport);
    ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    draw_scene_graph();
    draw_viewport();
    draw_object_inspector();
    draw_material_editor();
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
