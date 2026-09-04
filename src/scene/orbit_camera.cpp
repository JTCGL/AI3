#include "scene/orbit_camera.h"

#include "scene/world_coordinates.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
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
    return glm::perspective(glm::radians(vertical_fov_degrees_), aspect_ratio, 0.1F, 100.0F);
}

void OrbitCamera::orbit(float yaw_delta_degrees, float pitch_delta_degrees)
{
    yaw_degrees_ += yaw_delta_degrees;
    pitch_degrees_ = std::clamp(pitch_degrees_ + pitch_delta_degrees, -85.0F, 85.0F);
}

bool OrbitCamera::pan(glm::vec2 pointer_delta_pixels, glm::vec2 viewport_size)
{
    if (!std::isfinite(pointer_delta_pixels.x) || !std::isfinite(pointer_delta_pixels.y) ||
        !std::isfinite(viewport_size.x) || !std::isfinite(viewport_size.y) ||
        viewport_size.x <= 0.0F || viewport_size.y <= 0.0F ||
        (pointer_delta_pixels.x == 0.0F && pointer_delta_pixels.y == 0.0F))
        return false;

    const glm::vec3 forward = glm::normalize(target_ - position());
    const glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float world_units_per_pixel =
        2.0F * distance_ * std::tan(glm::radians(vertical_fov_degrees_) * 0.5F) / viewport_size.y;
    const glm::vec3 candidate =
        target_ + (-pointer_delta_pixels.x * right + pointer_delta_pixels.y * up) *
                      world_units_per_pixel;
    if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z))
        return false;
    target_ = candidate;
    return true;
}

void OrbitCamera::zoom(float wheel_delta)
{
    distance_ = std::clamp(distance_ * std::exp(-wheel_delta * 0.12F), 1.5F, 30.0F);
}

void OrbitCamera::reset() { *this = OrbitCamera{}; }
} // namespace ai3
