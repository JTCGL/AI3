#pragma once

#include "editor/editor_state.h"
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

void append_object_bounds(HelperGeometry& result, const EditorState& scene,
                          const SceneObject& object, glm::vec3 color);
void append_helper_geometry(HelperGeometry& destination, const HelperGeometry& source);
HelperGeometry resolve_bounds_helper_geometry(const EditorState& scene, ObjectId selected_id,
                                              ObjectId hovered_id);
HelperGeometry resolve_translation_helper_geometry(ObjectId selected_id, glm::vec3 gizmo_pivot,
                                                   const glm::mat3& gizmo_basis,
                                                   const ResolvedViewportView& view,
                                                   glm::vec2 viewport_size,
                                                   float gizmo_pixel_length,
                                                   int highlighted_axis = -1);
HelperGeometry resolve_helper_geometry(const EditorState& scene, ObjectId selected_id,
                                       ObjectId hovered_id, glm::vec3 gizmo_pivot,
                                       const glm::mat3& gizmo_basis,
                                       const ResolvedViewportView& view, glm::vec2 viewport_size,
                                       float gizmo_pixel_length, int highlighted_axis = -1);
} // namespace ai3
