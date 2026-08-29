#include "scene/scene_math.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace ai3
{
glm::mat4 compose_transform(const Transform& transform)
{
    glm::mat4 result = glm::translate(glm::mat4{1.0F}, transform.position);
    result = glm::rotate(result, glm::radians(transform.rotation.z), {0.0F, 0.0F, 1.0F});
    result = glm::rotate(result, glm::radians(transform.rotation.y), {0.0F, 1.0F, 0.0F});
    result = glm::rotate(result, glm::radians(transform.rotation.x), {1.0F, 0.0F, 0.0F});
    return glm::scale(result, transform.scale);
}
} // namespace ai3
