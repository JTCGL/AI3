#pragma once

#include "editor/editor_state.h"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ai3
{
// Euler values are human-facing degrees using intrinsic XYZ: rotate about the local X axis,
// then the resulting local Y axis, then the resulting local Z axis. Positive angles follow the
// right-hand rule. Quaternion-to-Euler results are one equivalent representation, not a unique one.
glm::quat orientation_from_euler_degrees(glm::vec3 euler_degrees);
glm::vec3 euler_degrees_from_orientation(const glm::quat& orientation);
glm::vec3 local_forward_from_orientation(const glm::quat& orientation);

enum class CoordinateSpace
{
    local,
    parent,
    world,
    view
};

// Returns an orthonormal reference-space basis expressed in world coordinates. Reference space
// selects manipulation axes and is intentionally separate from local-to-parent transform storage.
glm::mat3 coordinate_space_basis(const EditorState& scene, ObjectId id, CoordinateSpace space,
                                 const glm::mat4& view_matrix = glm::mat4{1.0F});
glm::vec3 camera_forward_direction(const EditorState& scene, ObjectId camera_id);
glm::vec3 directional_light_direction(const EditorState& scene, ObjectId light_id);
} // namespace ai3
