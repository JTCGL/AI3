#pragma once

#include "scene/triangle_mesh.h"

namespace ai3
{
using SphereMesh = TriangleMesh;

SphereMesh make_sphere_mesh(float radius_meters);
} // namespace ai3
