#pragma once

#include "core/scene.h"
#include "core/workspace.h"
#include "scene/resolved_view.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace ai3
{
struct ColoredLine
{
    glm::vec3 start{0.0F};
    glm::vec3 end{0.0F};
    glm::vec3 color{1.0F};
};
struct ColoredTriangle
{
    glm::vec3 first{0.0F};
    glm::vec3 second{0.0F};
    glm::vec3 third{0.0F};
    glm::vec3 color{1.0F};
};
struct HelperGeometry
{
    std::vector<ColoredLine> lines;
    std::vector<ColoredTriangle> triangles;
};

void append_object_bounds(HelperGeometry& result, const Scene& scene, const SceneObject& object,
                          const BoundsDisplayState& display, glm::vec3 color);
HelperGeometry resolve_bounds_helper_geometry(const Scene& scene, const Workspace& workspace,
                                              ObjectId selected_id, ObjectId hovered_id);
HelperGeometry resolve_translation_helper_geometry(ObjectId selected_id, glm::vec3 gizmo_pivot,
                                                   const glm::mat3& gizmo_basis,
                                                   const ResolvedViewportView& view,
                                                   glm::vec2 viewport_size,
                                                   float gizmo_pixel_length,
                                                   int highlighted_axis = -1);
} // namespace ai3
