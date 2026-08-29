#pragma once

#include <glm/vec3.hpp>

namespace ai3
{
// AI3 world space is right-handed: +X right, +Y forward, and +Z up.
inline constexpr glm::vec3 world_right{1.0F, 0.0F, 0.0F};
inline constexpr glm::vec3 world_forward{0.0F, 1.0F, 0.0F};
inline constexpr glm::vec3 world_up{0.0F, 0.0F, 1.0F};
} // namespace ai3
