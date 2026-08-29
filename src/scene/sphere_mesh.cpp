#include "scene/sphere_mesh.h"

#include <cmath>
#include <stdexcept>

namespace ai3
{
SphereMesh make_sphere_mesh(float radius_meters)
{
    if (radius_meters <= 0.0F)
        throw std::invalid_argument("Sphere radius must be positive");

    constexpr std::uint32_t latitude_segments = 16;
    constexpr std::uint32_t longitude_segments = 24;
    constexpr float pi = 3.14159265358979323846F;
    SphereMesh mesh;
    mesh.vertices.reserve((latitude_segments + 1) * (longitude_segments + 1));
    mesh.indices.reserve(latitude_segments * longitude_segments * 6);

    for (std::uint32_t latitude = 0; latitude <= latitude_segments; ++latitude)
    {
        const float polar =
            pi * static_cast<float>(latitude) / static_cast<float>(latitude_segments);
        const float ring = std::sin(polar);
        const float z = std::cos(polar);
        for (std::uint32_t longitude = 0; longitude <= longitude_segments; ++longitude)
        {
            const float azimuth =
                2.0F * pi * static_cast<float>(longitude) / static_cast<float>(longitude_segments);
            const glm::vec3 normal{ring * std::cos(azimuth), ring * std::sin(azimuth), z};
            mesh.vertices.push_back({normal * radius_meters, normal});
        }
    }

    const std::uint32_t stride = longitude_segments + 1;
    for (std::uint32_t latitude = 0; latitude < latitude_segments; ++latitude)
        for (std::uint32_t longitude = 0; longitude < longitude_segments; ++longitude)
        {
            const std::uint32_t first = latitude * stride + longitude;
            const std::uint32_t second = first + stride;
            mesh.indices.insert(mesh.indices.end(),
                                {first, second, first + 1, first + 1, second, second + 1});
        }
    return mesh;
}
} // namespace ai3
