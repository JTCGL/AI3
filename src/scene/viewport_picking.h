#pragma once

#include "editor/editor_state.h"
#include "scene/resolved_view.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <limits>

namespace ai3
{
struct WorldRay
{
    glm::vec3 origin{0.0F};
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
    float maximum_distance = std::numeric_limits<float>::infinity();
};

// Coordinates are normalized across the rendered viewport, with (0, 0) at its top-left.
WorldRay viewport_world_ray(glm::vec2 viewport_coordinates, const ResolvedViewportView& view);

// Returns the closest enabled, visible sphere hit in front of the ray origin.
ObjectId pick_sphere(const EditorState& scene, const WorldRay& ray);
} // namespace ai3
