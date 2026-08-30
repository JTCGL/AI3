#include "scene/orbit_camera.h"

#include "scene/world_coordinates.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace ai3
{
glm::vec3 OrbitCamera::position() const
{
    const float yaw = glm::radians(yaw_degrees_);
    const float pitch = glm::radians(pitch_degrees_);
    const float horizontal_distance = distance_ * std::cos(pitch);
    return target_ + glm::vec3{horizontal_distance * std::sin(yaw),
                               -horizontal_distance * std::cos(yaw), distance_ * std::sin(pitch)};
}

glm::mat4 OrbitCamera::view_matrix() const { return glm::lookAt(position(), target_, world_up); }

glm::mat4 OrbitCamera::projection_matrix(float aspect_ratio) const
{
    return glm::perspective(glm::radians(50.0F), aspect_ratio, 0.1F, 100.0F);
}

void OrbitCamera::orbit(float yaw_delta_degrees, float pitch_delta_degrees)
{
    yaw_degrees_ += yaw_delta_degrees;
    pitch_degrees_ = std::clamp(pitch_degrees_ + pitch_delta_degrees, -85.0F, 85.0F);
}

void OrbitCamera::zoom(float wheel_delta)
{
    distance_ = std::clamp(distance_ * std::exp(-wheel_delta * 0.12F), 1.5F, 30.0F);
}

void OrbitCamera::reset() { *this = OrbitCamera{}; }
} // namespace ai3
