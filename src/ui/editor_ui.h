#pragma once
namespace ai3
{
class EditorUi
{
    public:
    void draw(bool& running);

    private:
    bool show_hello_ = true;
    bool show_demo_ = false;
    bool show_metrics_ = false;
    bool show_debug_log_ = false;
    bool show_id_stack_ = false;
    bool show_about_ = false;
};
} // namespace ai3
