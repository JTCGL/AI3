#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace ai3
{
struct MeshVertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct SphereMesh
{
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};

SphereMesh make_sphere_mesh(float radius_meters);
} // namespace ai3
