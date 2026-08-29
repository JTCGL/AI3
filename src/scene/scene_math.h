#pragma once

#include "editor/editor_state.h"

#include <glm/mat4x4.hpp>

namespace ai3
{
glm::mat4 compose_transform(const Transform& transform);
} // namespace ai3
