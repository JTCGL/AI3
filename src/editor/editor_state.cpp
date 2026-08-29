#include "editor/editor_state.h"

#include <utility>

namespace ai3
{
namespace
{
std::size_t panel_index(EditorPanel panel) { return static_cast<std::size_t>(panel); }
} // namespace

EditorState::EditorState()
    : objects_({{1, no_object, "Scene", "Scene", true, true, {}, RenderableKind::none},
                {2, 1, "Camera", "Camera", true, true, {}, RenderableKind::none},
                {3, 1, "Light", "Light", true, true, {}, RenderableKind::none},
                {4, 1, "Cube", "Mesh", true, true, {}, RenderableKind::cube},
                {5, 1, "Group", "Group", true, true, {}, RenderableKind::none},
                {6, 5, "Child A", "Mesh", true, true, {}, RenderableKind::none},
                {7, 5, "Child B", "Mesh", true, true, {}, RenderableKind::none}}),
      console_messages_({{"console.initialized", {}}, {"console.ready", {}}})
{
    objects_[1].transform.position = {0.0F, 2.0F, 5.0F};
    objects_[2].transform.position = {2.0F, 4.0F, 1.0F};
}

const std::vector<SceneObject>& EditorState::objects() const { return objects_; }

SceneObject* EditorState::find_object(ObjectId id)
{
    for (SceneObject& object : objects_)
        if (object.id == id)
            return &object;
    return nullptr;
}

const SceneObject* EditorState::find_object(ObjectId id) const
{
    for (const SceneObject& object : objects_)
        if (object.id == id)
            return &object;
    return nullptr;
}

std::vector<ObjectId> EditorState::children_of(ObjectId parent) const
{
    std::vector<ObjectId> children;
    for (const SceneObject& object : objects_)
        if (object.parent == parent)
            children.push_back(object.id);
    return children;
}

ObjectId EditorState::selection() const { return selection_; }

bool EditorState::select(ObjectId id)
{
    const SceneObject* object = find_object(id);
    if (object == nullptr || selection_ == id)
        return false;
    selection_ = id;
    add_console_message("console.selected", object->name);
    return true;
}

void EditorState::clear_selection() { selection_ = no_object; }

bool EditorState::panel_visible(EditorPanel panel) const
{
    return panel_visibility_.at(panel_index(panel));
}

void EditorState::set_panel_visible(EditorPanel panel, bool visible)
{
    panel_visibility_.at(panel_index(panel)) = visible;
}

const std::vector<ConsoleMessage>& EditorState::console_messages() const
{
    return console_messages_;
}

void EditorState::add_console_message(std::string key, std::string argument)
{
    console_messages_.push_back({std::move(key), std::move(argument)});
}

void EditorState::clear_console() { console_messages_.clear(); }

void EditorState::request_layout_reset()
{
    panel_visibility_.fill(true);
    layout_reset_requested_ = true;
}

bool EditorState::consume_layout_reset_request()
{
    const bool requested = layout_reset_requested_;
    layout_reset_requested_ = false;
    return requested;
}
} // namespace ai3
