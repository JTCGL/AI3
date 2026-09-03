#include "editor/editor_state.h"
#include "scene/color_space.h"

#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
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

bool equal(const glm::vec3& left, const glm::vec3& right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool equal(const glm::quat& left, const glm::quat& right)
{
    return left.w == right.w && left.x == right.x && left.y == right.y && left.z == right.z;
}

bool equal(const Transform& left, const Transform& right)
{
    return equal(left.position, right.position) && equal(left.orientation, right.orientation) &&
           equal(left.scale, right.scale);
}
bool equal(const Material& left, const Material& right)
{
    return left.id == right.id && left.name == right.name && left.shading == right.shading &&
           equal(left.ambient_color, right.ambient_color) &&
           equal(left.diffuse_color, right.diffuse_color) &&
           equal(left.specular_color, right.specular_color) &&
           left.specular_power == right.specular_power;
}

void validate_material(const Material& material)
{
    if ((material.shading != MaterialShading::lambert &&
         material.shading != MaterialShading::phong) ||
        !valid_linear_color(material.ambient_color) ||
        !valid_linear_color(material.diffuse_color) ||
        !valid_linear_color(material.specular_color) || !std::isfinite(material.specular_power) ||
        material.specular_power <= 0.0F)
        throw std::invalid_argument("Material parameters are invalid");
}

bool finite(const glm::mat4& matrix)
{
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!std::isfinite(matrix[column][row]))
                return false;
    return true;
}

bool approximately_equal(float desired, float reconstructed)
{
    constexpr float absolute_tolerance = 0.00001F;
    constexpr float relative_tolerance = 0.0001F;
    return std::abs(desired - reconstructed) <=
           absolute_tolerance +
               relative_tolerance * std::max(std::abs(desired), std::abs(reconstructed));
}

bool decompose_verified_trs(const glm::mat4& matrix, Transform& result)
{
    if (!finite(matrix))
        return false;
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(matrix, result.scale, result.orientation, result.position, skew,
                        perspective))
        return false;
    if (!std::isfinite(glm::length(result.orientation)) || glm::length(result.orientation) == 0.0F)
        return false;
    result.orientation = glm::normalize(result.orientation);
    const glm::mat4 reconstructed = compose_transform(result);
    if (!finite(reconstructed))
        return false;
    for (int column = 0; column < 3; ++column)
        for (int row = 0; row < 3; ++row)
            if (!approximately_equal(matrix[column][row], reconstructed[column][row]))
                return false;
    for (int row = 0; row < 3; ++row)
        if (!approximately_equal(matrix[3][row], reconstructed[3][row]))
            return false;
    for (int column = 0; column < 4; ++column)
        if (!approximately_equal(matrix[column][3], reconstructed[column][3]))
            return false;
    return true;
}

void validate(const CreateObject& object)
{
    if (!valid_transform(object.transform))
        throw std::invalid_argument("Scene object transform is invalid");
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
    if (object.primitive_kind == PrimitiveKind::sphere &&
        !valid_linear_color(object.sphere.fallback_color))
        throw std::invalid_argument("Sphere fallback color is invalid");
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
    if (object.light_kind == LightKind::directional)
    {
        const DirectionalLight& light = object.directional_light;
        if (!valid_linear_color(light.color))
            throw std::invalid_argument("Directional light color is invalid");
        if (!std::isfinite(light.intensity) || light.intensity < 0.0F)
            throw std::invalid_argument("Directional light intensity must be non-negative");
    }
}
} // namespace

bool valid_transform(const Transform& transform)
{
    for (float value : {transform.position.x, transform.position.y, transform.position.z,
                        transform.scale.x, transform.scale.y, transform.scale.z})
        if (!std::isfinite(value))
            return false;
    const float orientation_length = glm::length(transform.orientation);
    return std::isfinite(orientation_length) && orientation_length > 0.0F &&
           std::abs(orientation_length - 1.0F) <= 0.0001F;
}

glm::mat4 compose_transform(const Transform& transform)
{
    glm::mat4 result = glm::translate(glm::mat4{1.0F}, transform.position);
    result *= glm::mat4_cast(glm::normalize(transform.orientation));
    return glm::scale(result, transform.scale);
}

EditorState::EditorState() : console_messages_({{"console.initialized", {}}, {"console.ready", {}}})
{
}

ObjectId EditorState::create_object(CreateObject object)
{
    if (object.parent != no_object && find_object(object.parent) == nullptr)
        throw std::invalid_argument("Scene object parent does not exist");
    validate(object);
    if (object.sphere.material_id != no_material &&
        find_material(object.sphere.material_id) == nullptr)
        throw std::invalid_argument("Sphere material does not exist");
    const ObjectId id = next_object_id_++;
    SceneObject created;
    created.id = id;
    created.parent_id_ = object.parent;
    created.name = std::move(object.name);
    created.enabled = object.enabled;
    created.visible = object.visible;
    created.transform = object.transform;
    created.category = object.category;
    created.primitive_kind = object.primitive_kind;
    created.camera_kind = object.camera_kind;
    created.light_kind = object.light_kind;
    created.sphere = object.sphere;
    created.perspective_camera = object.perspective_camera;
    created.directional_light = object.directional_light;
    rebuild_bounds(created);
    objects_.push_back(std::move(created));
    advance_document_revision();
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
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || object->primitive_kind != PrimitiveKind::sphere)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::primitive;
    candidate.primitive_kind = PrimitiveKind::sphere;
    candidate.sphere = sphere;
    validate(candidate);
    if (sphere.material_id != no_material && find_material(sphere.material_id) == nullptr)
        return false;
    if (object->sphere.radius_meters == sphere.radius_meters &&
        object->sphere.material_id == sphere.material_id &&
        equal(object->sphere.fallback_color, sphere.fallback_color))
        return true;
    const bool shape_changed = object->sphere.radius_meters != sphere.radius_meters;
    object->sphere = sphere;
    if (shape_changed)
        rebuild_bounds(*object);
    advance_document_revision();
    return true;
}

void EditorState::rebuild_bounds(SceneObject& object)
{
    object.bounds = {};
    if (object.primitive_kind == PrimitiveKind::sphere)
    {
        const float radius = object.sphere.radius_meters;
        if (std::isfinite(radius) && radius > 0.0F)
        {
            object.bounds.box = AxisAlignedBounds{glm::vec3{-radius}, glm::vec3{radius}};
            object.bounds.sphere = BoundingSphere{{0.0F, 0.0F, 0.0F}, radius};
        }
    }
}

const BoundsDisplayState& EditorState::bounds_display(ObjectId id) const
{
    static const BoundsDisplayState defaults;
    const auto found = bounds_workspace_.find(id);
    return found == bounds_workspace_.end() ? defaults : found->second;
}

bool EditorState::set_bounds_display(ObjectId id, BoundsDisplayState display)
{
    const SceneObject* object = find_object(id);
    if (object == nullptr || !object->bounds.box.has_value() || !object->bounds.sphere.has_value())
        return false;
    if (!display.show_bounding_box && !display.show_bounding_sphere && !display.hover_feedback)
        bounds_workspace_.erase(id);
    else
        bounds_workspace_[id] = display;
    return true;
}

void EditorState::replace_bounds_workspace(std::map<ObjectId, BoundsDisplayState> workspace)
{
    bounds_workspace_.clear();
    for (const auto& [id, display] : workspace)
        if (const SceneObject* object = find_object(id);
            object != nullptr && object->bounds.box.has_value())
            set_bounds_display(id, display);
}

const std::map<ObjectId, BoundsDisplayState>& EditorState::bounds_workspace() const
{
    return bounds_workspace_;
}

MaterialId EditorState::create_material(std::string localized_base_name, Material material)
{
    validate_material(material);
    const MaterialId id = next_material_id_++;
    material.id = id;
    material.name =
        std::move(localized_base_name) + " " + std::to_string(++default_material_name_count_);
    materials_.push_back(std::move(material));
    advance_document_revision();
    return id;
}

bool EditorState::rename_material(MaterialId id, std::string name)
{
    Material* material = find_material_mutable(id);
    if (material == nullptr)
        return false;
    if (material->name == name)
        return true;
    material->name = std::move(name);
    advance_document_revision();
    return true;
}

bool EditorState::set_material(MaterialId id, Material material)
{
    Material* current = find_material_mutable(id);
    if (current == nullptr)
        return false;
    material.id = id;
    validate_material(material);
    if (equal(*current, material))
        return true;
    *current = std::move(material);
    advance_document_revision();
    return true;
}

bool EditorState::assign_material(ObjectId id, MaterialId material_id)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || object->primitive_kind != PrimitiveKind::sphere ||
        (material_id != no_material && find_material(material_id) == nullptr))
        return false;
    if (object->sphere.material_id == material_id)
        return true;
    object->sphere.material_id = material_id;
    advance_document_revision();
    return true;
}

bool EditorState::set_sphere_fallback_color(ObjectId id, glm::vec3 linear_color)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || object->primitive_kind != PrimitiveKind::sphere ||
        !valid_linear_color(linear_color))
        return false;
    if (equal(object->sphere.fallback_color, linear_color))
        return true;
    object->sphere.fallback_color = linear_color;
    advance_document_revision();
    return true;
}

bool EditorState::set_perspective_camera(ObjectId id, PerspectiveCamera camera)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || object->camera_kind != CameraKind::perspective)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::camera;
    candidate.camera_kind = CameraKind::perspective;
    candidate.perspective_camera = camera;
    validate(candidate);
    if (object->perspective_camera.vertical_fov_degrees == camera.vertical_fov_degrees &&
        object->perspective_camera.near_plane_meters == camera.near_plane_meters &&
        object->perspective_camera.far_plane_meters == camera.far_plane_meters)
        return true;
    object->perspective_camera = camera;
    advance_document_revision();
    return true;
}

bool EditorState::set_directional_light(ObjectId id, DirectionalLight light)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || object->light_kind != LightKind::directional)
        return false;
    CreateObject candidate{object->name};
    candidate.category = ObjectCategory::light;
    candidate.light_kind = LightKind::directional;
    candidate.directional_light = light;
    validate(candidate);
    if (equal(object->directional_light.color, light.color) &&
        object->directional_light.intensity == light.intensity)
        return true;
    object->directional_light = light;
    advance_document_revision();
    return true;
}

bool EditorState::rename_object(ObjectId id, std::string name)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr)
        return false;
    if (object->name == name)
        return true;
    object->name = std::move(name);
    advance_document_revision();
    return true;
}

bool EditorState::set_object_enabled(ObjectId id, bool enabled)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr)
        return false;
    if (object->enabled == enabled)
        return true;
    object->enabled = enabled;
    advance_document_revision();
    return true;
}

bool EditorState::set_object_visible(ObjectId id, bool visible)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr)
        return false;
    if (object->visible == visible)
        return true;
    object->visible = visible;
    advance_document_revision();
    return true;
}

bool EditorState::set_local_transform(ObjectId id, Transform transform)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || !valid_transform(transform))
        return false;
    if (equal(object->transform, transform))
        return true;
    object->transform = std::move(transform);
    advance_document_revision();
    return true;
}

bool EditorState::set_world_position(ObjectId id, glm::vec3 world_position)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || !std::isfinite(world_position.x) || !std::isfinite(world_position.y) ||
        !std::isfinite(world_position.z))
        return false;
    glm::vec3 local_position = world_position;
    if (object->parent_id_ != no_object)
    {
        const glm::mat4 parent_world = world_transform_matrix(object->parent_id_);
        const float determinant = glm::determinant(parent_world);
        if (!std::isfinite(determinant) || std::abs(determinant) <= 0.000001F)
            return false;
        const glm::vec4 local = glm::inverse(parent_world) * glm::vec4{world_position, 1.0F};
        if (!std::isfinite(local.x) || !std::isfinite(local.y) || !std::isfinite(local.z) ||
            !std::isfinite(local.w) || std::abs(local.w) <= 0.000001F)
            return false;
        local_position = glm::vec3{local} / local.w;
    }
    Transform transform = object->transform;
    transform.position = local_position;
    return set_local_transform(id, transform);
}

bool EditorState::reparent_object(ObjectId id, ObjectId new_parent)
{
    SceneObject* object = find_object_mutable(id);
    if (object == nullptr || id == new_parent)
        return false;
    if (new_parent != no_object && find_object(new_parent) == nullptr)
        return false;
    for (ObjectId ancestor = new_parent; ancestor != no_object;)
    {
        if (ancestor == id)
            return false;
        const SceneObject* parent = find_object(ancestor);
        if (parent == nullptr)
            return false;
        ancestor = parent->parent_id_;
    }
    if (object->parent_id_ == new_parent)
        return true;

    const glm::mat4 old_world = world_transform_matrix(id);
    glm::mat4 desired_local = old_world;
    if (new_parent != no_object)
    {
        const glm::mat4 parent_world = world_transform_matrix(new_parent);
        const float determinant = glm::determinant(parent_world);
        if (!std::isfinite(determinant) || std::abs(determinant) <= 0.000001F)
            return false;
        desired_local = glm::inverse(parent_world) * old_world;
    }
    Transform new_local;
    if (!decompose_verified_trs(desired_local, new_local))
        return false;

    object->parent_id_ = new_parent;
    object->transform = new_local;
    advance_document_revision();
    return true;
}

bool EditorState::delete_object(ObjectId id)
{
    if (find_object(id) == nullptr)
        return false;

    EditorState candidate = *this;
    for (ObjectId child : children_of(id))
        if (!candidate.reparent_object(child, no_object))
            return false;
    candidate.objects_.erase(std::remove_if(candidate.objects_.begin(), candidate.objects_.end(),
                                            [id](const SceneObject& object)
                                            { return object.id == id; }),
                             candidate.objects_.end());

    objects_ = std::move(candidate.objects_);
    bounds_workspace_.erase(id);
    if (selection_ == id)
        clear_selection();
    advance_document_revision();
    return true;
}

bool EditorState::reset_scene()
{
    if (objects_.empty() && materials_.empty() && next_object_id_ == 1 && next_material_id_ == 1 &&
        default_name_counts_.empty() && default_material_name_count_ == 0)
    {
        bounds_workspace_.clear();
        return false;
    }
    objects_.clear();
    materials_.clear();
    selection_ = no_object;
    next_object_id_ = 1;
    next_material_id_ = 1;
    default_material_name_count_ = 0;
    default_name_counts_.clear();
    bounds_workspace_.clear();
    advance_document_revision();
    return true;
}
DocumentRevision EditorState::document_revision() const { return document_revision_; }
const std::vector<SceneObject>& EditorState::objects() const { return objects_; }
const std::vector<Material>& EditorState::materials() const { return materials_; }
Material* EditorState::find_material_mutable(MaterialId id)
{
    for (Material& material : materials_)
        if (material.id == id)
            return &material;
    return nullptr;
}
const Material* EditorState::find_material(MaterialId id) const
{
    for (const Material& material : materials_)
        if (material.id == id)
            return &material;
    return nullptr;
}
SceneObject* EditorState::find_object_mutable(ObjectId id)
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
        if (object.parent_id_ == parent)
            children.push_back(object.id);
    return children;
}

ResolvedTransform EditorState::world_transform(ObjectId id) const
{
    const SceneObject* object = find_object(id);
    if (object == nullptr)
        throw std::invalid_argument("Scene object does not exist");
    std::vector<const SceneObject*> chain;
    for (const SceneObject* current = object; current != nullptr;)
    {
        chain.push_back(current);
        current = current->parent_id_ == no_object ? nullptr : find_object(current->parent_id_);
        if (current == nullptr && chain.back()->parent_id_ != no_object)
            throw std::logic_error("Scene hierarchy contains a missing parent");
        if (chain.size() > objects_.size())
            throw std::logic_error("Scene hierarchy contains a cycle");
    }
    ResolvedTransform resolved;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    {
        resolved.matrix *= compose_transform((*it)->transform);
        resolved.orientation =
            glm::normalize(resolved.orientation * glm::normalize((*it)->transform.orientation));
    }
    resolved.position = glm::vec3{resolved.matrix[3]};
    return resolved;
}

glm::mat4 EditorState::world_transform_matrix(ObjectId id) const
{
    return world_transform(id).matrix;
}

glm::vec3 EditorState::world_position(ObjectId id) const { return world_transform(id).position; }

glm::quat EditorState::world_orientation(ObjectId id) const
{
    return world_transform(id).orientation;
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
void EditorState::advance_document_revision()
{
    if (document_revision_ == std::numeric_limits<DocumentRevision>::max())
        throw std::overflow_error("Scene Document revision exhausted");
    ++document_revision_;
}
} // namespace ai3
