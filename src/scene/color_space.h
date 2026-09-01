#pragma once

#include <glm/vec3.hpp>

namespace ai3
{
float srgb_to_linear(float value);
float linear_to_srgb(float value);
glm::vec3 srgb_to_linear(glm::vec3 value);
glm::vec3 linear_to_srgb(glm::vec3 value);
bool valid_linear_color(glm::vec3 value);
} // namespace ai3
