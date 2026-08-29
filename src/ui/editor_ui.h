#pragma once
#include "editor/editor_state.h"
#include "localization/localization.h"

#include <string>
#include <string_view>

namespace ai3
{
class EditorUi
{
    public:
    EditorUi(Localization& localization, float content_scale, float ui_scale, float font_size);
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

    EditorState state_;
    Localization& localization_;
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
