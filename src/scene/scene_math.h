#pragma once

#include "editor/editor_state.h"

#include <array>

namespace ai3
{
struct Vec3
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct Mat4
{
    std::array<float, 16> values{};
    const float* data() const { return values.data(); }
};

Mat4 identity_matrix();
Mat4 multiply(const Mat4& left, const Mat4& right);
Mat4 compose_transform(const Transform& transform);
Mat4 perspective(float vertical_fov_radians, float aspect_ratio, float near_plane, float far_plane);
Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
Vec3 transform_point(const Mat4& matrix, Vec3 point);
} // namespace ai3
