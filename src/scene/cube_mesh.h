#pragma once

#include <array>
#include <cstdint>

namespace ai3
{
struct MeshVertex
{
    std::array<float, 3> position;
    std::array<float, 3> normal;
};

struct CubeMesh
{
    std::array<MeshVertex, 24> vertices;
    std::array<std::uint16_t, 36> indices;
};

CubeMesh make_cube_mesh();
} // namespace ai3
