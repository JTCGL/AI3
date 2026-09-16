#include "editor/editor_state.h"

#include <utility>

namespace ai3
{
namespace
{
std::size_t panel_index(EditorPanel panel) { return static_cast<std::size_t>(panel); }
} // namespace

EditorState::EditorState()
    : history_(scene_, workspace_), operations_(scene_, workspace_, history_),
      console_messages_({{"console.initialized", {}}, {"console.ready", {}}})
{
}
Scene& EditorState::scene() { return scene_; }
const Scene& EditorState::scene() const { return scene_; }
Workspace& EditorState::workspace() { return workspace_; }
const Workspace& EditorState::workspace() const { return workspace_; }
EditHistory& EditorState::history() { return history_; }
const EditHistory& EditorState::history() const { return history_; }
EditOperations& EditorState::operations() { return operations_; }
const EditOperations& EditorState::operations() const { return operations_; }

ObjectId EditorState::create_object(CreateObject object)
{
    return operations_.create_object(std::move(object));
}
ObjectId EditorState::create_sphere(std::string name, SpherePrimitive value)
{
    return operations_.create_sphere(std::move(name), value);
}
ObjectId EditorState::create_box(std::string name, BoxPrimitive value)
{
    return operations_.create_box(std::move(name), value);
}
ObjectId EditorState::create_perspective_camera(std::string name, PerspectiveCamera value)
{
    return operations_.create_perspective_camera(std::move(name), value);
}
ObjectId EditorState::create_directional_light(std::string name, DirectionalLight value)
{
    return operations_.create_directional_light(std::move(name), value);
}
bool EditorState::set_sphere(ObjectId id, SpherePrimitive value)
{
    return operations_.set_sphere(id, value);
}
bool EditorState::set_box(ObjectId id, BoxPrimitive value)
{
    return operations_.set_box(id, value);
}
bool EditorState::set_perspective_camera(ObjectId id, PerspectiveCamera value)
{
    return operations_.set_perspective_camera(id, value);
}
bool EditorState::set_directional_light(ObjectId id, DirectionalLight value)
{
    return operations_.set_directional_light(id, value);
}
MaterialId EditorState::create_material(std::string name, Material value)
{
    return operations_.create_material(std::move(name), std::move(value));
}
bool EditorState::rename_material(MaterialId id, std::string name)
{
    return operations_.rename_material(id, std::move(name));
}
bool EditorState::set_material(MaterialId id, Material value)
{
    return operations_.set_material(id, std::move(value));
}
bool EditorState::assign_material(ObjectId id, MaterialId material)
{
    return operations_.assign_material(id, material);
}
bool EditorState::set_sphere_fallback_color(ObjectId id, glm::vec3 color)
{
    return operations_.set_sphere_fallback_color(id, color);
}
bool EditorState::rename_object(ObjectId id, std::string name)
{
    return operations_.rename_object(id, std::move(name));
}
bool EditorState::set_object_enabled(ObjectId id, bool value)
{
    return operations_.set_object_enabled(id, value);
}
bool EditorState::set_object_visible(ObjectId id, bool value)
{
    return operations_.set_object_visible(id, value);
}
bool EditorState::set_local_transform(ObjectId id, Transform value)
{
    return operations_.set_local_transform(id, std::move(value));
}
bool EditorState::set_world_position(ObjectId id, glm::vec3 value)
{
    return operations_.set_world_position(id, value);
}
bool EditorState::reparent_object(ObjectId id, ObjectId parent)
{
    return operations_.reparent_object(id, parent);
}
bool EditorState::delete_object(ObjectId id) { return operations_.delete_object(id); }
bool EditorState::reset_scene() { return operations_.reset_scene(); }

const BoundsDisplayState& EditorState::bounds_display(ObjectId id) const
{
    return workspace_.bounds_display(id);
}
bool EditorState::set_bounds_display(ObjectId id, BoundsDisplayState display)
{
    return operations_.set_bounds_display(id, display);
}
void EditorState::replace_bounds_workspace(std::map<ObjectId, BoundsDisplayState> workspace)
{
    operations_.replace_bounds_display(std::move(workspace));
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
    if (!operations_.select(id))
        return false;
    add_console_message("console.selected", object->name);
    return true;
}
void EditorState::clear_selection() { operations_.clear_selection(); }
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
