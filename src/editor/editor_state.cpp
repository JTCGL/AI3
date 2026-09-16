#include "editor/editor_state.h"

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
Scene& EditorState::scene() { return scene_; }
const Scene& EditorState::scene() const { return scene_; }
Workspace& EditorState::workspace() { return workspace_; }
const Workspace& EditorState::workspace() const { return workspace_; }

ObjectId EditorState::create_object(CreateObject object)
{
    return scene_.create_object(std::move(object));
}
ObjectId EditorState::create_sphere(std::string name, SpherePrimitive value)
{
    return scene_.create_sphere(std::move(name), value);
}
ObjectId EditorState::create_box(std::string name, BoxPrimitive value)
{
    return scene_.create_box(std::move(name), value);
}
ObjectId EditorState::create_perspective_camera(std::string name, PerspectiveCamera value)
{
    return scene_.create_perspective_camera(std::move(name), value);
}
ObjectId EditorState::create_directional_light(std::string name, DirectionalLight value)
{
    return scene_.create_directional_light(std::move(name), value);
}
bool EditorState::set_sphere(ObjectId id, SpherePrimitive value)
{
    return scene_.set_sphere(id, value);
}
bool EditorState::set_box(ObjectId id, BoxPrimitive value) { return scene_.set_box(id, value); }
bool EditorState::set_perspective_camera(ObjectId id, PerspectiveCamera value)
{
    return scene_.set_perspective_camera(id, value);
}
bool EditorState::set_directional_light(ObjectId id, DirectionalLight value)
{
    return scene_.set_directional_light(id, value);
}
MaterialId EditorState::create_material(std::string name, Material value)
{
    return scene_.create_material(std::move(name), std::move(value));
}
bool EditorState::rename_material(MaterialId id, std::string name)
{
    return scene_.rename_material(id, std::move(name));
}
bool EditorState::set_material(MaterialId id, Material value)
{
    return scene_.set_material(id, std::move(value));
}
bool EditorState::assign_material(ObjectId id, MaterialId material)
{
    return scene_.assign_material(id, material);
}
bool EditorState::set_sphere_fallback_color(ObjectId id, glm::vec3 color)
{
    return scene_.set_sphere_fallback_color(id, color);
}
bool EditorState::rename_object(ObjectId id, std::string name)
{
    return scene_.rename_object(id, std::move(name));
}
bool EditorState::set_object_enabled(ObjectId id, bool value)
{
    return scene_.set_object_enabled(id, value);
}
bool EditorState::set_object_visible(ObjectId id, bool value)
{
    return scene_.set_object_visible(id, value);
}
bool EditorState::set_local_transform(ObjectId id, Transform value)
{
    return scene_.set_local_transform(id, std::move(value));
}
bool EditorState::set_world_position(ObjectId id, glm::vec3 value)
{
    return scene_.set_world_position(id, value);
}
bool EditorState::reparent_object(ObjectId id, ObjectId parent)
{
    return scene_.reparent_object(id, parent);
}
bool EditorState::delete_object(ObjectId id)
{
    if (!scene_.delete_object(id))
        return false;
    workspace_.remove_bounds_display(id);
    if (workspace_.selection() == id)
        clear_selection();
    return true;
}
bool EditorState::reset_scene()
{
    const bool changed = scene_.reset_scene();
    workspace_.clear_selection();
    workspace_.replace_bounds_display({});
    return changed;
}

const BoundsDisplayState& EditorState::bounds_display(ObjectId id) const
{
    return workspace_.bounds_display(id);
}
bool EditorState::set_bounds_display(ObjectId id, BoundsDisplayState display)
{
    const SceneObject* object = find_object(id);
    if (object == nullptr || !object->bounds.box.has_value() || !object->bounds.sphere.has_value())
        return false;
    workspace_.set_bounds_display(id, display);
    return true;
}
void EditorState::replace_bounds_workspace(std::map<ObjectId, BoundsDisplayState> workspace)
{
    std::map<ObjectId, BoundsDisplayState> validated;
    for (const auto& [id, display] : workspace)
    {
        const SceneObject* object = find_object(id);
        if (object != nullptr && object->bounds.box.has_value() &&
            object->bounds.sphere.has_value() &&
            (display.show_bounding_box || display.show_bounding_sphere || display.hover_feedback))
            validated.emplace(id, display);
    }
    workspace_.replace_bounds_display(std::move(validated));
}
const std::map<ObjectId, BoundsDisplayState>& EditorState::bounds_workspace() const
{
    return workspace_.bounds_display_states();
}

DocumentRevision EditorState::document_revision() const { return scene_.document_revision(); }
const std::vector<SceneObject>& EditorState::objects() const { return scene_.objects(); }
const SceneObject* EditorState::find_object(ObjectId id) const { return scene_.find_object(id); }
const std::vector<Material>& EditorState::materials() const { return scene_.materials(); }
const Material* EditorState::find_material(MaterialId id) const { return scene_.find_material(id); }
std::vector<ObjectId> EditorState::children_of(ObjectId parent) const
{
    return scene_.children_of(parent);
}
std::vector<const SceneObject*> EditorState::objects_by_category(ObjectCategory value,
                                                                 ObjectQueryFilter filter) const
{
    return scene_.objects_by_category(value, filter);
}
std::vector<const SceneObject*> EditorState::primitives(PrimitiveKind value,
                                                        ObjectQueryFilter filter) const
{
    return scene_.primitives(value, filter);
}
std::vector<const SceneObject*> EditorState::cameras(CameraKind value,
                                                     ObjectQueryFilter filter) const
{
    return scene_.cameras(value, filter);
}
std::vector<const SceneObject*> EditorState::lights(LightKind value, ObjectQueryFilter filter) const
{
    return scene_.lights(value, filter);
}
ResolvedTransform EditorState::world_transform(ObjectId id) const
{
    return scene_.world_transform(id);
}
glm::mat4 EditorState::world_transform_matrix(ObjectId id) const
{
    return scene_.world_transform_matrix(id);
}
glm::vec3 EditorState::world_position(ObjectId id) const { return scene_.world_position(id); }
glm::quat EditorState::world_orientation(ObjectId id) const { return scene_.world_orientation(id); }

ObjectId EditorState::selection() const { return workspace_.selection(); }
bool EditorState::select(ObjectId id)
{
    const SceneObject* object = find_object(id);
    if (object == nullptr || workspace_.selection() == id)
        return false;
    workspace_.set_selection(id);
    add_console_message("console.selected", object->name);
    return true;
}
void EditorState::clear_selection() { workspace_.clear_selection(); }
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
