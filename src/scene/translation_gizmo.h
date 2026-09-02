#pragma once

#include "editor/editor_state.h"
#include "scene/resolved_view.h"
#include "scene/viewport_picking.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <optional>

namespace ai3
{
enum class TranslationAxis
{
    x,
    y,
    z,
    none
};

enum class AxisConstraintMethod
{
    closest_points,
    view_fallback
};

struct AxisDragConstraint
{
    AxisConstraintMethod method = AxisConstraintMethod::closest_points;
    glm::vec3 pivot{0.0F};
    glm::vec3 axis{1.0F, 0.0F, 0.0F};
    glm::vec3 plane_normal{0.0F};
    float starting_parameter = 0.0F;
    bool valid = false;
};

struct AxisTranslationGesture
{
    ObjectId object_id = no_object;
    TranslationAxis selected_axis = TranslationAxis::none;
    glm::vec3 starting_world_pivot{0.0F};
    glm::vec3 world_axis{1.0F, 0.0F, 0.0F};
    glm::mat3 frozen_basis{1.0F};
    float frozen_world_axis_length = 1.0F;
    float frozen_hit_tolerance = 1.0F;
    glm::vec2 frozen_viewport_size{1.0F};
    ResolvedViewportView frozen_view;
    AxisDragConstraint constraint;
};

AxisDragConstraint begin_axis_drag_constraint(const WorldRay& pointer_ray, glm::vec3 pivot,
                                              glm::vec3 world_axis,
                                              const ResolvedViewportView& view);
std::optional<glm::vec3> constrained_axis_position(const AxisDragConstraint& constraint,
                                                   const WorldRay& pointer_ray);
std::optional<float> translation_gizmo_world_length(glm::vec3 pivot,
                                                    const ResolvedViewportView& view,
                                                    float desired_pixels,
                                                    float viewport_height_pixels);
std::optional<glm::vec2> project_world_to_viewport(glm::vec3 point,
                                                   const ResolvedViewportView& view,
                                                   glm::vec2 viewport_size);
TranslationAxis pick_translation_axis(glm::vec2 pointer, const std::array<glm::vec2, 3>& starts,
                                      const std::array<glm::vec2, 3>& ends, float tolerance);
} // namespace ai3
