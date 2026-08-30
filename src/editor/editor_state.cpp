#include "editor/editor_state.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace ai3
{
namespace
{
std::size_t panel_index(EditorPanel panel) { return static_cast<std::size_t>(panel); }
bool matches_filter(const SceneObject& object, ObjectQueryFilter filter)
{
    return (!filter.enabled_only || object.enabled) && (!filter.visible_only || object.visible);
}

void validate(const CreateObject& object)
{
    const bool primitive_valid = object.category == ObjectCategory::primitive
                                     ? object.primitive_kind != PrimitiveKind::none
                                     : object.primitive_kind == PrimitiveKind::none;
    const bool camera_valid = object.category == ObjectCategory::camera
                                  ? object.camera_kind != CameraKind::none
                                  : object.camera_kind == CameraKind::none;
    const bool light_valid = object.category == ObjectCategory::light
                                 ? object.light_kind != LightKind::none
                                 : object.light_kind == LightKind::none;
    if (!primitive_valid || !camera_valid || !light_valid)
        throw std::invalid_argument("Scene object category and subtype are inconsistent");
    if (object.primitive_kind == PrimitiveKind::sphere &&
        (!std::isfinite(object.sphere.radius_meters) || object.sphere.radius_meters <= 0.0F))
        throw std::invalid_argument("Sphere radius must be positive");
    if (object.camera_kind == CameraKind::perspective)
    {
        const PerspectiveCamera& camera = object.perspective_camera;
        if (!std::isfinite(camera.vertical_fov_degrees) || camera.vertical_fov_degrees <= 0.0F ||
            camera.vertical_fov_degrees >= 180.0F)
            throw std::invalid_argument("Perspective camera FOV must be between 0 and 180 degrees");
        if (!std::isfinite(camera.near_plane_meters) || camera.near_plane_meters <= 0.0F)
            throw std::invalid_argument("Perspective camera near plane must be positive");
        if (!std::isfinite(camera.far_plane_meters) ||
            camera.far_plane_meters <= camera.near_plane_meters)
            throw std::invalid_argument("Perspective camera far plane must exceed its near plane");
    }
    if (object.light_kind == LightKind::directional &&
        (!std::isfinite(object.directional_light.intensity) ||
         object.directional_light.intensity < 0.0F))
        throw std::invalid_argument("Directional light intensity must be non-negative");
}
} // namespace

EditorState::EditorState() : console_messages_({{"console.initialized", {}}, {"console.ready", {}}})
{
}

ObjectId EditorState::create_object(CreateObject object)
{
    if (object.parent != no_object && find_object(object.parent) == nullptr)
        throw std::invalid_argument("Scene object parent does not exist");
    validate(object);
    const ObjectId id = next_object_id_++;
    objects_.push_back({id, object.parent, std::move(object.name), object.enabled, object.visible,
                        object.transform, object.category, object.primitive_kind,
                        object.camera_kind, object.light_kind, object.sphere,
                        object.perspective_camera, object.directional_light});
    return id;
}

ObjectId EditorState::create_named_object(std::string localized_base_name, CreateObject object,
                                          SubtypeKey subtype)
{
    const std::uint64_t number = default_name_counts_[subtype] + 1;
    object.name = std::move(localized_base_name) + " " + std::to_string(number);
    const ObjectId id = create_object(std::move(object));
    default_name_counts_[subtype] = number;
    return id;
}

ObjectId EditorState::create_sphere(std::string localized_base_name, SpherePrimitive sphere)
{
    CreateObject object{""};
    object.category = ObjectCategory::primitive;
    object.primitive_kind = PrimitiveKind::sphere;
    object.sphere = sphere;
    return create_named_object(
        std::move(localized_base_name), std::move(object),
        {ObjectCategory::primitive, static_cast<int>(PrimitiveKind::sphere)});
}

ObjectId EditorState::create_perspective_camera(std::string localized_base_name,
                                                PerspectiveCamera camera)
{
    CreateObject object{""};
    object.category = ObjectCategory::camera;
    object.camera_kind = CameraKind::perspective;
    object.perspective_camera = camera;
    return create_named_object(std::move(localized_base_name), std::move(object),
                               {ObjectCategory::camera, static_cast<int>(CameraKind::perspective)});
}

ObjectId EditorState::create_directional_light(std::string localized_base_name,
                                               DirectionalLight light)
{
    CreateObject object{""};
    object.category = ObjectCategory::light;
    object.light_kind = LightKind::directional;
    object.directional_light = light;
    return create_named_object(std::move(localized_base_name), std::move(object),
                               {ObjectCategory::light, static_cast<int>(LightKind::directional)});
}

bool EditorState::set_sphere(ObjectId id, SpherePrimitive sphere)
{
    SceneObject* object = find_object(id);
    if (object == nullptr || object->primitive_kind != PrimitiveKind::sphere)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::primitive;
    candidate.primitive_kind = PrimitiveKind::sphere;
    candidate.sphere = sphere;
    validate(candidate);
    object->sphere = sphere;
    return true;
}

bool EditorState::set_perspective_camera(ObjectId id, PerspectiveCamera camera)
{
    SceneObject* object = find_object(id);
    if (object == nullptr || object->camera_kind != CameraKind::perspective)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::camera;
    candidate.camera_kind = CameraKind::perspective;
    candidate.perspective_camera = camera;
    validate(candidate);
    object->perspective_camera = camera;
    return true;
}

bool EditorState::set_directional_light(ObjectId id, DirectionalLight light)
{
    SceneObject* object = find_object(id);
    if (object == nullptr || object->light_kind != LightKind::directional)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::light;
    candidate.light_kind = LightKind::directional;
    candidate.directional_light = light;
    validate(candidate);
    object->directional_light = light;
    return true;
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
    default_name_counts_.clear();
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

std::vector<const SceneObject*> EditorState::objects_by_category(ObjectCategory category,
                                                                 ObjectQueryFilter filter) const
{
    std::vector<const SceneObject*> matches;
    for (const SceneObject& object : objects_)
        if (object.category == category && matches_filter(object, filter))
            matches.push_back(&object);
    return matches;
}
std::vector<const SceneObject*> EditorState::primitives(PrimitiveKind kind,
                                                        ObjectQueryFilter filter) const
{
    std::vector<const SceneObject*> matches;
    for (const SceneObject* object : objects_by_category(ObjectCategory::primitive, filter))
        if (object->primitive_kind == kind)
            matches.push_back(object);
    return matches;
}
std::vector<const SceneObject*> EditorState::cameras(CameraKind kind,
                                                     ObjectQueryFilter filter) const
{
    std::vector<const SceneObject*> matches;
    for (const SceneObject* object : objects_by_category(ObjectCategory::camera, filter))
        if (object->camera_kind == kind)
            matches.push_back(object);
    return matches;
}
std::vector<const SceneObject*> EditorState::lights(LightKind kind, ObjectQueryFilter filter) const
{
    std::vector<const SceneObject*> matches;
    for (const SceneObject* object : objects_by_category(ObjectCategory::light, filter))
        if (object->light_kind == kind)
            matches.push_back(object);
    return matches;
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
