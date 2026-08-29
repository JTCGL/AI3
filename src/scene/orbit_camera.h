#pragma once

#include "scene/scene_math.h"

namespace ai3
{
class OrbitCamera
{
    public:
    Mat4 view_matrix() const;
    Mat4 projection_matrix(float aspect_ratio) const;
    Vec3 position() const;
    void orbit(float yaw_delta_degrees, float pitch_delta_degrees);
    void zoom(float wheel_delta);

    float yaw_degrees() const { return yaw_degrees_; }
    float pitch_degrees() const { return pitch_degrees_; }
    float distance() const { return distance_; }

    private:
    Vec3 target_{};
    float yaw_degrees_ = 35.0F;
    float pitch_degrees_ = 20.0F;
    float distance_ = 6.0F;
};
} // namespace ai3
