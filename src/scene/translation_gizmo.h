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
    float frozen_screen_axis_length = 1.0F;
    float frozen_hit_tolerance = 1.0F;
    glm::vec2 frozen_viewport_origin{0.0F};
    glm::vec2 frozen_viewport_size{1.0F};
    ResolvedViewportView frozen_view;
    AxisDragConstraint constraint;
};

struct ProjectedTranslationGizmo
{
    glm::vec2 pivot{0.0F};
    std::array<std::optional<glm::vec2>, 3> endpoints;
};

AxisDragConstraint begin_axis_drag_constraint(const WorldRay& pointer_ray, glm::vec3 pivot,
                                              glm::vec3 world_axis,
                                              const ResolvedViewportView& view);
std::optional<glm::vec3> constrained_axis_position(const AxisDragConstraint& constraint,
                                                   const WorldRay& pointer_ray);
std::optional<glm::vec2> project_world_to_viewport(glm::vec3 point,
                                                   const ResolvedViewportView& view,
                                                   glm::vec2 viewport_size);
std::optional<ProjectedTranslationGizmo>
project_translation_gizmo(glm::vec3 pivot, const glm::mat3& basis, const ResolvedViewportView& view,
                          glm::vec2 viewport_size, float screen_axis_length);
TranslationAxis pick_translation_axis(glm::vec2 pointer, const ProjectedTranslationGizmo& gizmo,
                                      float tolerance);
bool viewport_geometry_matches(glm::vec2 frozen_origin, glm::vec2 frozen_size,
                               glm::vec2 current_origin, glm::vec2 current_size);
} // namespace ai3
