#include "editor/editor_state.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace ai3
{
namespace
{
std::size_t panel_index(EditorPanel panel) { return static_cast<std::size_t>(panel); }
} // namespace

EditorState::EditorState() : console_messages_({{"console.initialized", {}}, {"console.ready", {}}})
{
}

ObjectId EditorState::create_object(CreateObject object)
{
    if (object.parent != no_object && find_object(object.parent) == nullptr)
        throw std::invalid_argument("Scene object parent does not exist");
    if (object.primitive == PrimitiveKind::sphere && object.sphere.radius_meters <= 0.0F)
        throw std::invalid_argument("Sphere radius must be positive");

    const ObjectId id = next_object_id_++;
    objects_.push_back({id, object.parent, std::move(object.name), object.enabled, object.visible,
                        object.transform, object.primitive, object.sphere});
    return id;
}

ObjectId EditorState::create_sphere(std::string localized_base_name, SpherePrimitive sphere)
{
    const std::uint64_t sphere_number = spheres_created_ + 1;
    const ObjectId id =
        create_object({std::move(localized_base_name) + " " + std::to_string(sphere_number),
                       no_object,
                       {},
                       true,
                       true,
                       PrimitiveKind::sphere,
                       sphere});
    spheres_created_ = sphere_number;
    return id;
}

bool EditorState::delete_object(ObjectId id)
{
    if (find_object(id) == nullptr)
        return false;

    std::unordered_set<ObjectId> deleted{id};
    for (std::size_t index = 0; index < objects_.size(); ++index)
        if (deleted.count(objects_[index].parent) != 0)
            deleted.insert(objects_[index].id);

    if (deleted.count(selection_) != 0)
        clear_selection();
    objects_.erase(std::remove_if(objects_.begin(), objects_.end(), [&](const SceneObject& object)
                                  { return deleted.count(object.id) != 0; }),
                   objects_.end());
    return true;
}

void EditorState::reset_scene()
{
    objects_.clear();
    selection_ = no_object;
    next_object_id_ = 1;
    spheres_created_ = 0;
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

std::vector<const SceneObject*> EditorState::visible_spheres() const
{
    std::vector<const SceneObject*> spheres;
    for (const SceneObject& object : objects_)
        if (object.enabled && object.visible && object.primitive == PrimitiveKind::sphere)
            spheres.push_back(&object);
    return spheres;
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
