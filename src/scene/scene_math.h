#pragma once

#include "editor/editor_state.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ai3
{
// Euler values are human-facing degrees using intrinsic XYZ: rotate about the local X axis,
// then the resulting local Y axis, then the resulting local Z axis. Positive angles follow the
// right-hand rule. Quaternion-to-Euler results are one equivalent representation, not a unique one.
glm::quat orientation_from_euler_degrees(glm::vec3 euler_degrees);
glm::vec3 euler_degrees_from_orientation(const glm::quat& orientation);
glm::mat4 compose_transform(const Transform& transform);
} // namespace ai3
