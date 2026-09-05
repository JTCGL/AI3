#include "scene/box_mesh.h"
#include "editor/editor_state.h"

#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <limits>
#include <stdexcept>

namespace ai3
{
TriangleMesh make_box_mesh(const BoxPrimitive& b)
{
    const auto valid = [](float v) { return std::isfinite(v) && v >= 0.001F && v <= 9999.0F; };
    if (!valid(b.width_meters) || !valid(b.length_meters) || !valid(b.height_meters) ||
        b.width_segments < 1 || b.width_segments > 999 || b.length_segments < 1 ||
        b.length_segments > 999 || b.height_segments < 1 || b.height_segments > 999)
        throw std::invalid_argument("Box parameters are invalid");
    TriangleMesh mesh;
    struct Face
    {
        glm::vec3 n, u, v;
        float extent_u, extent_v;
        int su, sv;
    };
    const float hx = b.width_meters * 0.5F, hy = b.length_meters * 0.5F,
                hz = b.height_meters * 0.5F;
    const std::array<Face, 6> faces{{{{1, 0, 0},
                                      {0, 1, 0},
                                      {0, 0, 1},
                                      b.length_meters,
                                      b.height_meters,
                                      b.length_segments,
                                      b.height_segments},
                                     {{-1, 0, 0},
                                      {0, -1, 0},
                                      {0, 0, 1},
                                      b.length_meters,
                                      b.height_meters,
                                      b.length_segments,
                                      b.height_segments},
                                     {{0, 1, 0},
                                      {-1, 0, 0},
                                      {0, 0, 1},
                                      b.width_meters,
                                      b.height_meters,
                                      b.width_segments,
                                      b.height_segments},
                                     {{0, -1, 0},
                                      {1, 0, 0},
                                      {0, 0, 1},
                                      b.width_meters,
                                      b.height_meters,
                                      b.width_segments,
                                      b.height_segments},
                                     {{0, 0, 1},
                                      {1, 0, 0},
                                      {0, 1, 0},
                                      b.width_meters,
                                      b.length_meters,
                                      b.width_segments,
                                      b.length_segments},
                                     {{0, 0, -1},
                                      {1, 0, 0},
                                      {0, -1, 0},
                                      b.width_meters,
                                      b.length_meters,
                                      b.width_segments,
                                      b.length_segments}}};
    for (const Face& f : faces)
    {
        const std::size_t rows = static_cast<std::size_t>(f.sv) + 1,
                          cols = static_cast<std::size_t>(f.su) + 1;
        if (rows > std::numeric_limits<std::size_t>::max() / cols ||
            mesh.vertices.size() > std::numeric_limits<std::size_t>::max() - rows * cols)
            throw std::length_error("Box mesh is too large");
        const std::size_t base = mesh.vertices.size();
        for (int j = 0; j <= f.sv; ++j)
            for (int i = 0; i <= f.su; ++i)
            {
                const float u = static_cast<float>(i) / f.su, v = static_cast<float>(j) / f.sv;
                const glm::vec3 p = f.n * (f.n.x   ? hx
                                           : f.n.y ? hy
                                                   : hz) +
                                    f.u * ((u - 0.5F) * f.extent_u) +
                                    f.v * ((v - 0.5F) * f.extent_v);
                mesh.vertices.push_back({p, f.n, {u, v}});
            }
        for (int j = 0; j < f.sv; ++j)
            for (int i = 0; i < f.su; ++i)
            {
                const std::uint32_t a =
                    static_cast<std::uint32_t>(base + static_cast<std::size_t>(j) * cols + i);
                const std::uint32_t b0 = a + 1,
                                    c = static_cast<std::uint32_t>(
                                        base + static_cast<std::size_t>(j + 1) * cols + i + 1),
                                    d = c - 1;
                mesh.indices.insert(mesh.indices.end(), {a, b0, c, a, c, d});
            }
    }
    return mesh;
}
} // namespace ai3
