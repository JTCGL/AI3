#include "scene/scene_math.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

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

glm::mat4 compose_transform(const Transform& transform)
{
    glm::mat4 result = glm::translate(glm::mat4{1.0F}, transform.position);
    result *= glm::mat4_cast(glm::normalize(transform.orientation));
    return glm::scale(result, transform.scale);
}
} // namespace ai3
