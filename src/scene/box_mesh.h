#pragma once

#include "scene/triangle_mesh.h"

namespace ai3
{
struct BoxPrimitive;

TriangleMesh make_box_mesh(const BoxPrimitive& box);
} // namespace ai3
