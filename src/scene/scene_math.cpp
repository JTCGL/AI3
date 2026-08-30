#include "scene/scene_math.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ai3
{
glm::quat orientation_from_euler_degrees(glm::vec3 euler_degrees)
{
    const glm::vec3 angles = glm::radians(euler_degrees);
    const glm::quat rotate_x = glm::angleAxis(angles.x, glm::vec3{1.0F, 0.0F, 0.0F});
    const glm::quat rotate_y = glm::angleAxis(angles.y, glm::vec3{0.0F, 1.0F, 0.0F});
    const glm::quat rotate_z = glm::angleAxis(angles.z, glm::vec3{0.0F, 0.0F, 1.0F});
    return glm::normalize(rotate_x * rotate_y * rotate_z);
}

glm::vec3 euler_degrees_from_orientation(const glm::quat& orientation)
{
    const glm::mat3 rotation = glm::mat3_cast(glm::normalize(orientation));
    const float y = std::asin(std::clamp(rotation[2][0], -1.0F, 1.0F));
    const float cosine_y = std::cos(y);
    float x;
    float z;
    if (std::abs(cosine_y) > 0.000001F)
    {
        x = std::atan2(-rotation[2][1], rotation[2][2]);
        z = std::atan2(-rotation[1][0], rotation[0][0]);
    }
    else
    {
        // At gimbal lock choose the valid representative with Z = 0. Editor continuity is deferred.
        x = std::atan2(rotation[1][2], rotation[1][1]);
        z = 0.0F;
    }
    return glm::degrees(glm::vec3{x, y, z});
}

glm::vec3 local_forward_from_orientation(const glm::quat& orientation)
{
    return glm::normalize(orientation) * glm::vec3{0.0F, 0.0F, -1.0F};
}

glm::mat3 coordinate_space_basis(const EditorState& scene, ObjectId id, CoordinateSpace space,
                                 const glm::mat4& view_matrix)
{
    const SceneObject* object = scene.find_object(id);
    if (object == nullptr)
        throw std::invalid_argument("Scene object does not exist");
    switch (space)
    {
    case CoordinateSpace::local:
        return glm::mat3_cast(scene.world_orientation(id));
    case CoordinateSpace::parent:
        return object->parent_id() == no_object
                   ? glm::mat3{1.0F}
                   : glm::mat3_cast(scene.world_orientation(object->parent_id()));
    case CoordinateSpace::world:
        return glm::mat3{1.0F};
    case CoordinateSpace::view:
        return glm::transpose(glm::mat3{view_matrix});
    }
    throw std::invalid_argument("Unknown coordinate space");
}

glm::vec3 camera_forward_direction(const EditorState& scene, ObjectId camera_id)
{
    const SceneObject* camera = scene.find_object(camera_id);
    if (camera == nullptr || camera->category != ObjectCategory::camera ||
        camera->camera_kind != CameraKind::perspective)
        throw std::invalid_argument("Scene object is not a perspective camera");
    return local_forward_from_orientation(scene.world_orientation(camera_id));
}

glm::vec3 directional_light_direction(const EditorState& scene, ObjectId light_id)
{
    const SceneObject* light = scene.find_object(light_id);
    if (light == nullptr || light->category != ObjectCategory::light ||
        light->light_kind != LightKind::directional)
        throw std::invalid_argument("Scene object is not a directional light");
    return local_forward_from_orientation(scene.world_orientation(light_id));
}
} // namespace ai3
