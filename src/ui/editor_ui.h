#pragma once
#include "editor/editor_state.h"

namespace ai3
{
class EditorUi
{
    public:
    void draw(bool& running);

    private:
    void draw_main_menu(bool& running);
    void draw_scene_graph();
    void draw_scene_node(ObjectId id);
    void draw_viewport();
    void draw_object_inspector();
    void draw_console();

    EditorState state_;
    bool show_demo_ = false;
    bool show_metrics_ = false;
    bool show_debug_log_ = false;
    bool show_id_stack_ = false;
    bool show_about_ = false;
};
} // namespace ai3
