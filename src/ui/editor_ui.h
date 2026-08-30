#pragma once
#include "editor/editor_state.h"
#include "localization/localization.h"
#include "render/viewport_renderer.h"
#include "scene/length_units.h"
#include "scene/viewport_view.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

struct SDL_Window;

namespace ai3
{
class SceneDialogState;
class EditorUi
{
    public:
    EditorUi(EditorState& state, ViewportView& viewport_view, Localization& localization,
             SDL_Window* window, float content_scale, float ui_scale, float font_size);
    void draw(bool& running);
    void set_scale_diagnostics(float content_scale, float ui_scale, float font_size);

    private:
    void draw_main_menu(bool& running);
    void draw_scene_graph();
    void draw_scene_node(ObjectId id);
    void accept_reparent_drop(ObjectId new_parent);
    void apply_pending_reparent();
    void draw_viewport();
    void draw_object_inspector();
    void draw_console();
    void request_open_dialog();
    void request_save_as_dialog();
    void save_document();
    void process_dialog_result();
    void report_document_result(std::string_view key, const std::string& detail);
    std::string window_title(std::string_view key, std::string_view stable_id) const;

    EditorState& state_;
    ViewportView& viewport_view_;
    ViewportRenderer viewport_renderer_;
    Localization& localization_;
    SDL_Window* window_ = nullptr;
    std::shared_ptr<SceneDialogState> dialog_state_;
    std::filesystem::path document_path_;
    LengthUnit display_length_unit_ = default_display_length_unit;
    float content_scale_ = 1.0F;
    float ui_scale_ = 1.0F;
    float font_size_ = 16.0F;
    bool show_demo_ = false;
    bool show_metrics_ = false;
    bool show_debug_log_ = false;
    bool show_id_stack_ = false;
    bool show_about_ = false;
    bool show_ai3_diagnostics_ = false;
    std::pair<ObjectId, ObjectId> pending_reparent_ = {no_object, no_object};
};
} // namespace ai3
