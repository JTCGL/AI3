#pragma once

#include <glm/mat4x4.hpp>

namespace ai3
{
struct ResolvedViewportView
{
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
};
} // namespace ai3
