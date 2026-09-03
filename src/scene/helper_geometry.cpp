#include "scene/helper_geometry.h"
#include "scene/translation_gizmo.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cmath>

namespace ai3
{
namespace
{
glm::vec3 transform_point(const glm::mat4& matrix, glm::vec3 point)
{
    return glm::vec3{matrix * glm::vec4{point, 1.0F}};
}
} // namespace

void append_object_bounds(HelperGeometry& result, const EditorState& scene,
                          const SceneObject& object, glm::vec3 color)
{
    const BoundsDisplayState& display = scene.bounds_display(object.id);
    const glm::mat4 world = scene.world_transform_matrix(object.id);
    if (display.show_bounding_box && object.bounds.box)
    {
        const auto& box = *object.bounds.box;
        std::array<glm::vec3, 8> p;
        for (int i = 0; i < 8; ++i)
            p[i] = transform_point(world, {i & 1 ? box.maximum.x : box.minimum.x,
                                           i & 2 ? box.maximum.y : box.minimum.y,
                                           i & 4 ? box.maximum.z : box.minimum.z});
        constexpr std::array<std::array<int, 2>, 12> edges = {{{0, 1},
                                                               {0, 2},
                                                               {0, 4},
                                                               {1, 3},
                                                               {1, 5},
                                                               {2, 3},
                                                               {2, 6},
                                                               {3, 7},
                                                               {4, 5},
                                                               {4, 6},
                                                               {5, 7},
                                                               {6, 7}}};
        for (const auto& edge : edges)
            result.lines.push_back({p[edge[0]], p[edge[1]], color});
    }
    if (display.show_bounding_sphere && object.bounds.sphere)
    {
        constexpr int segments = 48;
        const BoundingSphere& sphere = *object.bounds.sphere;
        for (int plane = 0; plane < 3; ++plane)
            for (int segment = 0; segment < segments; ++segment)
            {
                const float a = glm::two_pi<float>() * segment / segments;
                const float b = glm::two_pi<float>() * (segment + 1) / segments;
                const auto point = [&](float angle)
                {
                    glm::vec3 p = sphere.center;
                    const float u = std::cos(angle) * sphere.radius;
                    const float v = std::sin(angle) * sphere.radius;
                    if (plane == 0)
                        p += glm::vec3{u, v, 0.0F};
                    if (plane == 1)
                        p += glm::vec3{u, 0.0F, v};
                    if (plane == 2)
                        p += glm::vec3{0.0F, u, v};
                    return transform_point(world, p);
                };
                result.lines.push_back({point(a), point(b), color});
            }
    }
}

HelperGeometry resolve_helper_geometry(const EditorState& scene, ObjectId selected_id,
                                       ObjectId hovered_id, glm::vec3 gizmo_pivot,
                                       const glm::mat3& gizmo_basis,
                                       const ResolvedViewportView& view, glm::vec2 viewport_size,
                                       float gizmo_pixel_length, int highlighted_axis)
{
    HelperGeometry result;
    for (const SceneObject& object : scene.objects())
    {
        const bool selected = object.id == selected_id;
        const bool hovered =
            object.id == hovered_id && scene.bounds_display(object.id).hover_feedback;
        if (selected || hovered)
            append_object_bounds(result, scene, object,
                                 selected ? glm::vec3{1.0F} : glm::vec3{1.0F, 1.0F, 0.0F});
    }
    if (selected_id == no_object)
        return result;
    const auto projected = project_translation_gizmo(gizmo_pivot, gizmo_basis, view, viewport_size,
                                                     gizmo_pixel_length);
    if (!projected)
        return result;
    constexpr std::array<glm::vec3, 3> inactive = {glm::vec3{0.588F, 0.216F, 0.216F},
                                                   glm::vec3{0.196F, 0.549F, 0.255F},
                                                   glm::vec3{0.216F, 0.333F, 0.608F}};
    constexpr std::array<glm::vec3, 3> active = {glm::vec3{1.0F, 0.353F, 0.353F},
                                                 glm::vec3{0.353F, 0.941F, 0.412F},
                                                 glm::vec3{0.353F, 0.588F, 1.0F}};
    const glm::vec3 camera_right = glm::transpose(glm::mat3{view.view})[0];
    for (int i = 0; i < 3; ++i)
    {
        if (!projected->endpoints[i])
            continue;
        const glm::vec3 axis = glm::normalize(gizmo_basis[i]);
        const auto projected_unit =
            project_world_to_viewport(gizmo_pivot + axis, view, viewport_size);
        if (!projected_unit)
            continue;
        const float pixels_per_unit = glm::length(*projected_unit - projected->pivot);
        if (!std::isfinite(pixels_per_unit) || pixels_per_unit <= 0.000001F)
            continue;
        float world_length = gizmo_pixel_length / pixels_per_unit;
        bool valid_length = false;
        for (int iteration = 0; iteration < 8; ++iteration)
        {
            const auto candidate =
                project_world_to_viewport(gizmo_pivot + axis * world_length, view, viewport_size);
            if (!candidate)
                break;
            const float current_length = glm::length(*candidate - projected->pivot);
            if (!std::isfinite(current_length) || current_length <= 0.000001F)
                break;
            const float correction = gizmo_pixel_length / current_length;
            world_length *= correction;
            if (!std::isfinite(world_length) || world_length <= 0.0F)
                break;
            if (std::abs(1.0F - correction) <= 0.0001F)
            {
                valid_length = true;
                break;
            }
        }
        if (!valid_length)
            continue;
        const glm::vec3 endpoint = gizmo_pivot + axis * world_length;
        const glm::vec3 color = highlighted_axis == i ? active[i] : inactive[i];
        result.lines.push_back({gizmo_pivot, endpoint, color});
        glm::vec3 side_direction = glm::cross(axis, glm::cross(camera_right, axis));
        if (glm::length(side_direction) <= 0.000001F)
        {
            const glm::vec3 camera_up = glm::transpose(glm::mat3{view.view})[1];
            side_direction = glm::cross(axis, glm::cross(camera_up, axis));
        }
        if (glm::length(side_direction) <= 0.000001F)
            continue;
        const float world_units_per_pixel = world_length / gizmo_pixel_length;
        const glm::vec3 side = glm::normalize(side_direction) * (5.0F * world_units_per_pixel);
        const glm::vec3 base = endpoint - axis * (10.0F * world_units_per_pixel);
        result.triangles.push_back({endpoint, base + side, base - side, color});
    }
    return result;
}
} // namespace ai3
