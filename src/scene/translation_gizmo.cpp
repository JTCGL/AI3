#include "scene/translation_gizmo.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>

namespace ai3
{
namespace
{
constexpr float epsilon = 0.000001F;
constexpr float projected_direction_epsilon = 0.01F;

bool finite(glm::vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(glm::vec2 value) { return std::isfinite(value.x) && std::isfinite(value.y); }

std::optional<float> line_parameter(const WorldRay& ray, glm::vec3 pivot, glm::vec3 axis)
{
    const glm::vec3 direction = glm::normalize(ray.direction);
    const glm::vec3 offset = pivot - ray.origin;
    const float parallel = glm::dot(axis, direction);
    const float denominator = 1.0F - parallel * parallel;
    if (!std::isfinite(denominator) || denominator <= 0.01F)
        return std::nullopt;
    const float parameter =
        (parallel * glm::dot(direction, offset) - glm::dot(axis, offset)) / denominator;
    return std::isfinite(parameter) ? std::optional<float>{parameter} : std::nullopt;
}

std::optional<glm::vec3> plane_intersection(const WorldRay& ray, glm::vec3 point, glm::vec3 normal)
{
    const float denominator = glm::dot(ray.direction, normal);
    if (!std::isfinite(denominator) || std::abs(denominator) <= epsilon)
        return std::nullopt;
    const float distance = glm::dot(point - ray.origin, normal) / denominator;
    const glm::vec3 intersection = ray.origin + ray.direction * distance;
    return finite(intersection) ? std::optional<glm::vec3>{intersection} : std::nullopt;
}
} // namespace

AxisDragConstraint begin_axis_drag_constraint(const WorldRay& pointer_ray, glm::vec3 pivot,
                                              glm::vec3 world_axis,
                                              const ResolvedViewportView& view)
{
    AxisDragConstraint result;
    const float axis_length = glm::length(world_axis);
    const float ray_length = glm::length(pointer_ray.direction);
    if (!finite(pivot) || !finite(world_axis) || !finite(pointer_ray.origin) ||
        !finite(pointer_ray.direction) || !std::isfinite(axis_length) || axis_length <= epsilon ||
        !std::isfinite(ray_length) || ray_length <= epsilon)
        return result;
    result.pivot = pivot;
    result.axis = world_axis / axis_length;
    if (const std::optional<float> parameter =
            line_parameter(pointer_ray, result.pivot, result.axis))
    {
        result.method = AxisConstraintMethod::closest_points;
        result.starting_parameter = *parameter;
        result.valid = true;
        return result;
    }

    result.method = AxisConstraintMethod::view_fallback;
    const glm::mat3 view_basis = glm::transpose(glm::mat3{view.view});
    const glm::vec3 view_right = view_basis[0];
    const glm::vec3 view_up = view_basis[1];
    const glm::vec3 first = glm::cross(result.axis, view_right);
    const glm::vec3 second = glm::cross(result.axis, view_up);
    const glm::vec3 first_normal =
        glm::length(first) > epsilon ? glm::normalize(first) : glm::vec3{};
    const glm::vec3 second_normal =
        glm::length(second) > epsilon ? glm::normalize(second) : glm::vec3{};
    result.plane_normal = std::abs(glm::dot(pointer_ray.direction, first_normal)) >=
                                  std::abs(glm::dot(pointer_ray.direction, second_normal))
                              ? first_normal
                              : second_normal;
    const std::optional<glm::vec3> intersection =
        plane_intersection(pointer_ray, result.pivot, result.plane_normal);
    if (!intersection.has_value())
        return result;
    result.starting_parameter = glm::dot(*intersection - result.pivot, result.axis);
    result.valid = std::isfinite(result.starting_parameter);
    return result;
}

std::optional<glm::vec3> constrained_axis_position(const AxisDragConstraint& constraint,
                                                   const WorldRay& pointer_ray)
{
    if (!constraint.valid)
        return std::nullopt;
    std::optional<float> parameter;
    if (constraint.method == AxisConstraintMethod::closest_points)
        parameter = line_parameter(pointer_ray, constraint.pivot, constraint.axis);
    else if (const std::optional<glm::vec3> intersection =
                 plane_intersection(pointer_ray, constraint.pivot, constraint.plane_normal))
        parameter = glm::dot(*intersection - constraint.pivot, constraint.axis);
    if (!parameter.has_value())
        return std::nullopt;
    const glm::vec3 position =
        constraint.pivot + constraint.axis * (*parameter - constraint.starting_parameter);
    return finite(position) ? std::optional<glm::vec3>{position} : std::nullopt;
}

std::optional<glm::vec2> project_world_to_viewport(glm::vec3 point,
                                                   const ResolvedViewportView& view,
                                                   glm::vec2 viewport_size)
{
    const glm::vec4 clip = view.projection * view.view * glm::vec4{point, 1.0F};
    if (!finite(point) || !std::isfinite(clip.x) || !std::isfinite(clip.y) ||
        !std::isfinite(clip.w) || clip.w <= epsilon || !finite(viewport_size) ||
        viewport_size.x <= 0.0F || viewport_size.y <= 0.0F)
        return std::nullopt;
    const glm::vec2 normalized_device{clip.x / clip.w, clip.y / clip.w};
    return glm::vec2{(normalized_device.x * 0.5F + 0.5F) * viewport_size.x,
                     (0.5F - normalized_device.y * 0.5F) * viewport_size.y};
}

std::optional<ProjectedTranslationGizmo>
project_translation_gizmo(glm::vec3 pivot, const glm::mat3& basis, const ResolvedViewportView& view,
                          glm::vec2 viewport_size, float screen_axis_length)
{
    if (!std::isfinite(screen_axis_length) || screen_axis_length <= 0.0F)
        return std::nullopt;
    const std::optional<glm::vec2> projected_pivot =
        project_world_to_viewport(pivot, view, viewport_size);
    if (!projected_pivot.has_value())
        return std::nullopt;
    ProjectedTranslationGizmo result;
    result.pivot = *projected_pivot;
    for (std::size_t index = 0; index < result.endpoints.size(); ++index)
    {
        const std::optional<glm::vec2> projected_axis =
            project_world_to_viewport(pivot + basis[index], view, viewport_size);
        if (!projected_axis.has_value())
            continue;
        const glm::vec2 direction = *projected_axis - result.pivot;
        const float length = glm::length(direction);
        if (!std::isfinite(length) || length <= projected_direction_epsilon)
            continue;
        result.endpoints[index] = result.pivot + direction * (screen_axis_length / length);
    }
    return result;
}

TranslationAxis pick_translation_axis(glm::vec2 pointer, const ProjectedTranslationGizmo& gizmo,
                                      float tolerance)
{
    if (!std::isfinite(tolerance) || tolerance < 0.0F)
        return TranslationAxis::none;
    float best = tolerance;
    TranslationAxis result = TranslationAxis::none;
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (!gizmo.endpoints[index].has_value())
            continue;
        const glm::vec2 segment = *gizmo.endpoints[index] - gizmo.pivot;
        const float squared_length = glm::dot(segment, segment);
        const float amount =
            squared_length > epsilon
                ? std::clamp(glm::dot(pointer - gizmo.pivot, segment) / squared_length, 0.0F, 1.0F)
                : 0.0F;
        const float distance = glm::length(pointer - (gizmo.pivot + segment * amount));
        if (distance <= best)
        {
            best = distance;
            result = static_cast<TranslationAxis>(index);
        }
    }
    return result;
}

bool viewport_geometry_matches(glm::vec2 frozen_origin, glm::vec2 frozen_size,
                               glm::vec2 current_origin, glm::vec2 current_size)
{
    if (!std::isfinite(frozen_origin.x) || !std::isfinite(frozen_origin.y) ||
        !std::isfinite(frozen_size.x) || !std::isfinite(frozen_size.y) ||
        !std::isfinite(current_origin.x) || !std::isfinite(current_origin.y) ||
        !std::isfinite(current_size.x) || !std::isfinite(current_size.y))
        return false;
    constexpr float geometry_tolerance = 0.5F;
    return glm::all(glm::lessThanEqual(glm::abs(frozen_origin - current_origin),
                                       glm::vec2{geometry_tolerance})) &&
           glm::all(glm::lessThanEqual(glm::abs(frozen_size - current_size),
                                       glm::vec2{geometry_tolerance}));
}
} // namespace ai3
