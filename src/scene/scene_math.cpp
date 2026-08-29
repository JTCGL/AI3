#include "scene/scene_math.h"

#include <cmath>
#include <stdexcept>

namespace ai3
{
namespace
{
constexpr float pi = 3.14159265358979323846F;

Vec3 subtract(Vec3 left, Vec3 right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

float dot(Vec3 left, Vec3 right) { return left.x * right.x + left.y * right.y + left.z * right.z; }

Vec3 cross(Vec3 left, Vec3 right)
{
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

Vec3 normalized(Vec3 value)
{
    const float length = std::sqrt(dot(value, value));
    if (length <= 0.000001F)
        throw std::invalid_argument("Cannot normalize a zero-length vector");
    return {value.x / length, value.y / length, value.z / length};
}

float radians(float degrees) { return degrees * pi / 180.0F; }
} // namespace

Mat4 identity_matrix()
{
    Mat4 result;
    result.values = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                     0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
    return result;
}

Mat4 multiply(const Mat4& left, const Mat4& right)
{
    Mat4 result;
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            for (int inner = 0; inner < 4; ++inner)
                result.values[column * 4 + row] +=
                    left.values[inner * 4 + row] * right.values[column * 4 + inner];
    return result;
}

Mat4 compose_transform(const Transform& transform)
{
    Mat4 translation = identity_matrix();
    translation.values[12] = transform.position[0];
    translation.values[13] = transform.position[1];
    translation.values[14] = transform.position[2];

    Mat4 scale = identity_matrix();
    scale.values[0] = transform.scale[0];
    scale.values[5] = transform.scale[1];
    scale.values[10] = transform.scale[2];

    const float x = radians(transform.rotation[0]);
    const float y = radians(transform.rotation[1]);
    const float z = radians(transform.rotation[2]);
    Mat4 rotation_x = identity_matrix();
    rotation_x.values[5] = std::cos(x);
    rotation_x.values[6] = std::sin(x);
    rotation_x.values[9] = -std::sin(x);
    rotation_x.values[10] = std::cos(x);
    Mat4 rotation_y = identity_matrix();
    rotation_y.values[0] = std::cos(y);
    rotation_y.values[2] = -std::sin(y);
    rotation_y.values[8] = std::sin(y);
    rotation_y.values[10] = std::cos(y);
    Mat4 rotation_z = identity_matrix();
    rotation_z.values[0] = std::cos(z);
    rotation_z.values[1] = std::sin(z);
    rotation_z.values[4] = -std::sin(z);
    rotation_z.values[5] = std::cos(z);
    return multiply(translation,
                    multiply(rotation_z, multiply(rotation_y, multiply(rotation_x, scale))));
}

Mat4 perspective(float vertical_fov_radians, float aspect_ratio, float near_plane, float far_plane)
{
    if (aspect_ratio <= 0.0F || near_plane <= 0.0F || far_plane <= near_plane)
        throw std::invalid_argument("Invalid perspective projection parameters");
    const float focal_length = 1.0F / std::tan(vertical_fov_radians * 0.5F);
    Mat4 result;
    result.values[0] = focal_length / aspect_ratio;
    result.values[5] = focal_length;
    result.values[10] = (far_plane + near_plane) / (near_plane - far_plane);
    result.values[11] = -1.0F;
    result.values[14] = (2.0F * far_plane * near_plane) / (near_plane - far_plane);
    return result;
}

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up)
{
    const Vec3 forward = normalized(subtract(target, eye));
    const Vec3 side = normalized(cross(forward, up));
    const Vec3 corrected_up = cross(side, forward);
    Mat4 result = identity_matrix();
    result.values[0] = side.x;
    result.values[4] = side.y;
    result.values[8] = side.z;
    result.values[1] = corrected_up.x;
    result.values[5] = corrected_up.y;
    result.values[9] = corrected_up.z;
    result.values[2] = -forward.x;
    result.values[6] = -forward.y;
    result.values[10] = -forward.z;
    result.values[12] = -dot(side, eye);
    result.values[13] = -dot(corrected_up, eye);
    result.values[14] = dot(forward, eye);
    return result;
}

Vec3 transform_point(const Mat4& matrix, Vec3 point)
{
    return {matrix.values[0] * point.x + matrix.values[4] * point.y + matrix.values[8] * point.z +
                matrix.values[12],
            matrix.values[1] * point.x + matrix.values[5] * point.y + matrix.values[9] * point.z +
                matrix.values[13],
            matrix.values[2] * point.x + matrix.values[6] * point.y + matrix.values[10] * point.z +
                matrix.values[14]};
}
} // namespace ai3
