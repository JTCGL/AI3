#pragma once
#include "editor/editor_state.h"
#include "localization/localization.h"
#include "render/viewport_renderer.h"
#include "scene/length_units.h"
#include "scene/viewport_view.h"

#include <string>
#include <string_view>

namespace ai3
{
class EditorUi
{
    public:
    EditorUi(EditorState& state, ViewportView& viewport_view, Localization& localization,
             float content_scale, float ui_scale, float font_size);
    void draw(bool& running);
    void set_scale_diagnostics(float content_scale, float ui_scale, float font_size);

    private:
    void draw_main_menu(bool& running);
    void draw_scene_graph();
    void draw_scene_node(ObjectId id);
    void draw_viewport();
    void draw_object_inspector();
    void draw_console();
    std::string window_title(std::string_view key, std::string_view stable_id) const;

    EditorState& state_;
    ViewportView& viewport_view_;
    ViewportRenderer viewport_renderer_;
    Localization& localization_;
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
};
} // namespace ai3
