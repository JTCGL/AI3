#include "scene/color_space.h"

#include <cmath>
#include <stdexcept>

namespace ai3
{
float srgb_to_linear(float value)
{
    if (!std::isfinite(value) || value < 0.0F || value > 1.0F)
        throw std::invalid_argument("sRGB component must be finite and in [0, 1]");
    return value <= 0.04045F ? value / 12.92F : std::pow((value + 0.055F) / 1.055F, 2.4F);
}
float linear_to_srgb(float value)
{
    if (!std::isfinite(value) || value < 0.0F || value > 1.0F)
        throw std::invalid_argument("linear RGB component must be finite and in [0, 1]");
    return value <= 0.0031308F ? value * 12.92F : 1.055F * std::pow(value, 1.0F / 2.4F) - 0.055F;
}
glm::vec3 srgb_to_linear(glm::vec3 value)
{
    return {srgb_to_linear(value.x), srgb_to_linear(value.y), srgb_to_linear(value.z)};
}
glm::vec3 linear_to_srgb(glm::vec3 value)
{
    return {linear_to_srgb(value.x), linear_to_srgb(value.y), linear_to_srgb(value.z)};
}
bool valid_linear_color(glm::vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
           value.x >= 0.0F && value.x <= 1.0F && value.y >= 0.0F && value.y <= 1.0F &&
           value.z >= 0.0F && value.z <= 1.0F;
}
} // namespace ai3
