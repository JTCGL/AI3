#include "scene/viewport_picking.h"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ai3
{
namespace
{
bool finite(const glm::mat4& matrix)
{
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!std::isfinite(matrix[column][row]))
                return false;
    return true;
}

bool finite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
} // namespace

WorldRay viewport_world_ray(glm::vec2 viewport_coordinates, const ResolvedViewportView& view)
{
    if (!std::isfinite(viewport_coordinates.x) || !std::isfinite(viewport_coordinates.y))
        throw std::invalid_argument("Viewport coordinates must be finite");
    const glm::mat4 view_projection = view.projection * view.view;
    const float determinant = glm::determinant(view_projection);
    if (!finite(view_projection) || !std::isfinite(determinant) || std::abs(determinant) <= 1e-8F)
        throw std::invalid_argument("Resolved viewport matrices are not invertible");

    const glm::mat4 inverse = glm::inverse(view_projection);
    const float clip_x = viewport_coordinates.x * 2.0F - 1.0F;
    const float clip_y = 1.0F - viewport_coordinates.y * 2.0F;
    glm::vec4 near_point = inverse * glm::vec4{clip_x, clip_y, -1.0F, 1.0F};
    glm::vec4 far_point = inverse * glm::vec4{clip_x, clip_y, 1.0F, 1.0F};
    if (!std::isfinite(near_point.w) || !std::isfinite(far_point.w) ||
        std::abs(near_point.w) <= 1e-8F || std::abs(far_point.w) <= 1e-8F)
        throw std::invalid_argument("Resolved viewport ray is invalid");
    near_point /= near_point.w;
    far_point /= far_point.w;
    const glm::vec3 origin{near_point};
    const glm::vec3 difference = glm::vec3{far_point} - origin;
    const float length = glm::length(difference);
    if (!finite(origin) || !finite(difference) || !std::isfinite(length) || length <= 1e-8F)
        throw std::invalid_argument("Resolved viewport ray is invalid");
    return {origin, difference / length, length};
}

ObjectId pick_sphere(const EditorState& scene, const WorldRay& ray)
{
    const float direction_length = glm::length(ray.direction);
    if (!finite(ray.origin) || !finite(ray.direction) || !std::isfinite(direction_length) ||
        direction_length <= 1e-8F)
        return no_object;
    const glm::vec3 world_direction = ray.direction / direction_length;
    float closest_distance = std::numeric_limits<float>::infinity();
    ObjectId closest = no_object;
    for (const SceneObject* object : scene.primitives(PrimitiveKind::sphere, {true, true}))
    {
        const glm::mat4 world = scene.world_transform_matrix(object->id);
        const float determinant = glm::determinant(world);
        if (!finite(world) || !std::isfinite(determinant) || std::abs(determinant) <= 1e-8F)
            continue;
        const glm::mat4 inverse = glm::inverse(world);
        const glm::vec3 local_origin = glm::vec3{inverse * glm::vec4{ray.origin, 1.0F}};
        const glm::vec3 local_direction = glm::vec3{inverse * glm::vec4{world_direction, 0.0F}};
        if (!finite(local_origin) || !finite(local_direction))
            continue;
        const float a = glm::dot(local_direction, local_direction);
        const float b = 2.0F * glm::dot(local_origin, local_direction);
        const float radius = object->sphere.radius_meters;
        const float c = glm::dot(local_origin, local_origin) - radius * radius;
        const float discriminant = b * b - 4.0F * a * c;
        if (!std::isfinite(a) || a <= 1e-12F || !std::isfinite(discriminant) || discriminant < 0.0F)
            continue;
        const float root = std::sqrt(std::max(0.0F, discriminant));
        const float first = (-b - root) / (2.0F * a);
        const float second = (-b + root) / (2.0F * a);
        const float distance = first >= 0.0F ? first : second;
        if (std::isfinite(distance) && distance >= 0.0F && distance <= ray.maximum_distance &&
            distance < closest_distance)
        {
            closest_distance = distance;
            closest = object->id;
        }
    }
    return closest;
}

ObjectId pick_box(const EditorState& scene, const WorldRay& ray)
{
    if (!finite(ray.origin) || !finite(ray.direction) || glm::length(ray.direction) <= 1e-8F)
        return no_object;
    const glm::vec3 direction = glm::normalize(ray.direction);
    float closest = std::numeric_limits<float>::infinity();
    ObjectId result = no_object;
    for (const SceneObject* object : scene.primitives(PrimitiveKind::box, {true, true}))
    {
        const glm::mat4 world = scene.world_transform_matrix(object->id);
        const float det = glm::determinant(world);
        if (!finite(world) || !std::isfinite(det) || std::abs(det) <= 1e-8F)
            continue;
        const glm::mat4 inv = glm::inverse(world);
        const glm::vec3 o = glm::vec3(inv * glm::vec4(ray.origin, 1));
        const glm::vec3 d = glm::vec3(inv * glm::vec4(direction, 0));
        const glm::vec3 h{object->box.width_meters * 0.5F, object->box.length_meters * 0.5F,
                          object->box.height_meters * 0.5F};
        float tmin = 0.0F, tmax = ray.maximum_distance;
        bool hit = true;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(d[axis]) <= 1e-8F)
            {
                if (o[axis] < -h[axis] || o[axis] > h[axis])
                {
                    hit = false;
                    break;
                }
            }
            else
            {
                float a = (-h[axis] - o[axis]) / d[axis], b = (h[axis] - o[axis]) / d[axis];
                if (a > b)
                    std::swap(a, b);
                tmin = std::max(tmin, a);
                tmax = std::min(tmax, b);
                if (tmin > tmax)
                {
                    hit = false;
                    break;
                }
            }
        }
        if (hit && tmin < closest)
        {
            closest = tmin;
            result = object->id;
        }
    }
    return result;
}
} // namespace ai3
