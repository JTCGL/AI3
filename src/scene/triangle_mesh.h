#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace ai3
{
struct MeshVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};
struct TriangleMesh
{
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};
} // namespace ai3
