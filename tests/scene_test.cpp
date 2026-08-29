#include <doctest/doctest.h>

#include "scene/cube_mesh.h"
#include "scene/orbit_camera.h"
#include "scene/render_target_size.h"
#include "scene/scene_math.h"

TEST_CASE("transform composition applies scale rotation and translation")
{
    ai3::Transform transform;
    transform.position = {3.0F, 4.0F, 5.0F};
    transform.rotation = {0.0F, 0.0F, 90.0F};
    transform.scale = {2.0F, 1.0F, 1.0F};
    const ai3::Vec3 point =
        ai3::transform_point(ai3::compose_transform(transform), {1.0F, 0.0F, 0.0F});
    CHECK(point.x == doctest::Approx(3.0F));
    CHECK(point.y == doctest::Approx(6.0F));
    CHECK(point.z == doctest::Approx(5.0F));
}

TEST_CASE("orbit camera clamps interaction and produces finite matrices")
{
    ai3::OrbitCamera camera;
    camera.orbit(15.0F, 200.0F);
    CHECK(camera.yaw_degrees() == doctest::Approx(50.0F));
    CHECK(camera.pitch_degrees() == doctest::Approx(85.0F));
    camera.zoom(100.0F);
    CHECK(camera.distance() == doctest::Approx(1.5F));
    camera.zoom(-100.0F);
    CHECK(camera.distance() == doctest::Approx(30.0F));
    const ai3::Mat4 projection = camera.projection_matrix(16.0F / 9.0F);
    CHECK(projection.values[0] > 0.0F);
    CHECK(projection.values[5] > projection.values[0]);
}

TEST_CASE("procedural cube has complete indexed faces")
{
    const ai3::CubeMesh mesh = ai3::make_cube_mesh();
    CHECK(mesh.vertices.size() == 24);
    CHECK(mesh.indices.size() == 36);
    for (std::uint16_t index : mesh.indices)
        CHECK(index < mesh.vertices.size());
}

TEST_CASE("render target policy converts logical content to framebuffer pixels")
{
    const ai3::RenderTargetSize requested = ai3::render_target_size(320.0F, 180.0F, 2.0F, 1.5F);
    CHECK(requested.width == 640);
    CHECK(requested.height == 270);
    CHECK(ai3::requires_render_target_resize({}, requested));
    CHECK_FALSE(ai3::requires_render_target_resize(requested, requested));
    CHECK(ai3::render_target_size(0.0F, 100.0F, 1.0F, 1.0F).width == 0);
}
