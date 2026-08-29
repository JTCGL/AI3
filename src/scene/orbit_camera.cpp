#include "scene/orbit_camera.h"

#include <algorithm>
#include <cmath>

namespace ai3
{
namespace
{
constexpr float pi = 3.14159265358979323846F;
float radians(float degrees) { return degrees * pi / 180.0F; }
} // namespace

Vec3 OrbitCamera::position() const
{
    const float yaw = radians(yaw_degrees_);
    const float pitch = radians(pitch_degrees_);
    const float horizontal_distance = distance_ * std::cos(pitch);
    return {target_.x + horizontal_distance * std::sin(yaw),
            target_.y + distance_ * std::sin(pitch),
            target_.z + horizontal_distance * std::cos(yaw)};
}

Mat4 OrbitCamera::view_matrix() const { return look_at(position(), target_, {0.0F, 1.0F, 0.0F}); }

Mat4 OrbitCamera::projection_matrix(float aspect_ratio) const
{
    return perspective(radians(50.0F), aspect_ratio, 0.1F, 100.0F);
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
} // namespace ai3
