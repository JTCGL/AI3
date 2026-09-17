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
    const float yaw = glm::radians(state_.yaw_degrees);
    const float pitch = glm::radians(state_.pitch_degrees);
    const float horizontal_distance = state_.distance * std::cos(pitch);
    return state_.target + glm::vec3{horizontal_distance * std::sin(yaw),
                                     -horizontal_distance * std::cos(yaw),
                                     state_.distance * std::sin(pitch)};
}

glm::mat4 OrbitCamera::view_matrix() const
{
    return glm::lookAt(position(), state_.target, world_up);
}

glm::mat4 OrbitCamera::projection_matrix(float aspect_ratio) const
{
    return glm::perspective(glm::radians(vertical_fov_degrees_), aspect_ratio, 0.1F, 100.0F);
}

bool OrbitCamera::orbit(float yaw_delta_degrees, float pitch_delta_degrees)
{
    if (!std::isfinite(yaw_delta_degrees) || !std::isfinite(pitch_delta_degrees) ||
        (yaw_delta_degrees == 0.0F && pitch_delta_degrees == 0.0F))
        return false;
    state_.yaw_degrees += yaw_delta_degrees;
    state_.pitch_degrees = std::clamp(state_.pitch_degrees + pitch_delta_degrees, -85.0F, 85.0F);
    return true;
}

bool OrbitCamera::pan(glm::vec2 pointer_delta, float logical_viewport_height)
{
    if (!std::isfinite(pointer_delta.x) || !std::isfinite(pointer_delta.y) ||
        !std::isfinite(logical_viewport_height) || logical_viewport_height <= 0.0F ||
        (pointer_delta.x == 0.0F && pointer_delta.y == 0.0F) || !std::isfinite(state_.distance) ||
        state_.distance <= 0.0F)
        return false;

    const glm::vec3 forward = glm::normalize(state_.target - position());
    const glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float world_units_per_pixel =
        (2.0F * state_.distance * std::tan(glm::radians(vertical_fov_degrees_) * 0.5F)) /
        logical_viewport_height;
    const glm::vec3 translated =
        state_.target + (-right * pointer_delta.x + up * pointer_delta.y) * world_units_per_pixel;
    if (!std::isfinite(translated.x) || !std::isfinite(translated.y) ||
        !std::isfinite(translated.z))
        return false;
    state_.target = translated;
    return true;
}

bool OrbitCamera::zoom(float wheel_delta)
{
    if (!std::isfinite(wheel_delta) || wheel_delta == 0.0F)
        return false;
    state_.distance = std::clamp(state_.distance * std::exp(-wheel_delta * 0.12F), 1.5F, 30.0F);
    return true;
}

void OrbitCamera::reset() { state_ = {}; }
} // namespace ai3
