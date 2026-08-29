#include <doctest/doctest.h>

#include "scene/length_units.h"
#include "scene/orbit_camera.h"
#include "scene/render_target_size.h"
#include "scene/scene_math.h"
#include "scene/sphere_mesh.h"
#include "scene/world_coordinates.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace
{
void check_vec3(const glm::vec3& actual, const glm::vec3& expected, float epsilon = 0.0001F)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(epsilon));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(epsilon));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(epsilon));
}

void check_same_orientation(const glm::quat& actual, const glm::quat& expected)
{
    CHECK(std::abs(glm::dot(glm::normalize(actual), glm::normalize(expected))) ==
          doctest::Approx(1.0F).epsilon(0.0001));
}
} // namespace

TEST_CASE("world axes define a right-handed Z-up basis")
{
    check_vec3(glm::cross(ai3::world_right, ai3::world_forward), ai3::world_up);
    CHECK(ai3::world_right.x == doctest::Approx(1.0F));
    CHECK(ai3::world_forward.y == doctest::Approx(1.0F));
    CHECK(ai3::world_up.z == doctest::Approx(1.0F));
}

TEST_CASE("metric display units convert to and from canonical meters")
{
    CHECK(ai3::default_display_length_unit == ai3::LengthUnit::meter);
    CHECK(ai3::length_from_meters(1.0F, ai3::LengthUnit::millimeter) == doctest::Approx(1000.0F));
    CHECK(ai3::length_from_meters(1.0F, ai3::LengthUnit::centimeter) == doctest::Approx(100.0F));
    CHECK(ai3::length_from_meters(1.0F, ai3::LengthUnit::meter) == doctest::Approx(1.0F));
    CHECK(ai3::length_from_meters(1000.0F, ai3::LengthUnit::kilometer) == doctest::Approx(1.0F));
    for (ai3::LengthUnit unit : {ai3::LengthUnit::millimeter, ai3::LengthUnit::centimeter,
                                 ai3::LengthUnit::meter, ai3::LengthUnit::kilometer})
    {
        const float canonical_meters = 12.345F;
        const float displayed = ai3::length_from_meters(canonical_meters, unit);
        CHECK(ai3::length_to_meters(displayed, unit) == doctest::Approx(canonical_meters));
        CHECK_FALSE(ai3::length_unit_symbol(unit).empty());
    }

    ai3::Transform transform;
    transform.position = {1.25F, -2.5F, 3.75F};
    const glm::vec3 stored_meters = transform.position;
    CHECK(ai3::length_from_meters(transform.position.x, ai3::LengthUnit::millimeter) ==
          doctest::Approx(1250.0F));
    CHECK(ai3::length_from_meters(transform.position.x, ai3::LengthUnit::kilometer) ==
          doctest::Approx(0.00125F));
    check_vec3(transform.position, stored_meters);
}

TEST_CASE("Euler degree conversions use right-handed intrinsic XYZ")
{
    const glm::quat around_x = ai3::orientation_from_euler_degrees({90.0F, 0.0F, 0.0F});
    const glm::quat around_y = ai3::orientation_from_euler_degrees({0.0F, 90.0F, 0.0F});
    const glm::quat around_z = ai3::orientation_from_euler_degrees({0.0F, 0.0F, 90.0F});
    check_vec3(around_x * ai3::world_forward, ai3::world_up);
    check_vec3(around_y * ai3::world_up, ai3::world_right);
    check_vec3(around_z * ai3::world_right, ai3::world_forward);

    const glm::vec3 angles{23.0F, -31.0F, 47.0F};
    const glm::quat orientation = ai3::orientation_from_euler_degrees(angles);
    const glm::quat composed = glm::angleAxis(glm::radians(angles.x), ai3::world_right) *
                               glm::angleAxis(glm::radians(angles.y), ai3::world_forward) *
                               glm::angleAxis(glm::radians(angles.z), ai3::world_up);
    check_same_orientation(orientation, composed);
    check_same_orientation(
        ai3::orientation_from_euler_degrees(ai3::euler_degrees_from_orientation(orientation)),
        orientation);
    check_vec3(ai3::euler_degrees_from_orientation(orientation), angles, 0.001F);
}

TEST_CASE("transform composition applies scale rotation and translation")
{
    ai3::Transform transform;
    transform.position = {3.0F, 4.0F, 5.0F};
    transform.orientation = ai3::orientation_from_euler_degrees({0.0F, 0.0F, 90.0F});
    transform.scale = {2.0F, 1.0F, 1.0F};
    const glm::vec3 point{ai3::compose_transform(transform) * glm::vec4{1.0F, 0.0F, 0.0F, 1.0F}};
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
    const glm::vec3 camera_in_view_space{camera.view_matrix() * glm::vec4{camera.position(), 1.0F}};
    CHECK(camera_in_view_space.x == doctest::Approx(0.0F).epsilon(0.0001));
    CHECK(camera_in_view_space.y == doctest::Approx(0.0F).epsilon(0.0001));
    CHECK(camera_in_view_space.z == doctest::Approx(0.0F).epsilon(0.0001));
    const glm::vec3 target_in_view_space{camera.view_matrix() * glm::vec4{0.0F, 0.0F, 0.0F, 1.0F}};
    CHECK(target_in_view_space.z == doctest::Approx(-30.0F).epsilon(0.0001));
    const glm::mat4 projection = camera.projection_matrix(16.0F / 9.0F);
    CHECK(projection[0][0] > 0.0F);
    CHECK(projection[1][1] > projection[0][0]);
}

TEST_CASE("orbit camera uses Z as its stable up axis")
{
    ai3::OrbitCamera camera;
    CHECK(camera.position().z > 0.0F);
    camera.orbit(0.0F, 1000.0F);
    const glm::mat4 view = camera.view_matrix();
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            CHECK(std::isfinite(view[column][row]));
}

TEST_CASE("procedural sphere is deterministic and has valid indexed geometry")
{
    const ai3::SphereMesh mesh = ai3::make_sphere_mesh(1.0F);
    const ai3::SphereMesh repeated = ai3::make_sphere_mesh(1.0F);
    CHECK(mesh.vertices.size() == 425);
    CHECK(mesh.indices.size() == 2304);
    CHECK(repeated.indices == mesh.indices);
    for (std::uint32_t index : mesh.indices)
        CHECK(index < mesh.vertices.size());
    for (std::size_t index = 0; index < mesh.vertices.size(); ++index)
    {
        CHECK(glm::length(mesh.vertices[index].position) == doctest::Approx(1.0F));
        CHECK(mesh.vertices[index].position.x ==
              doctest::Approx(repeated.vertices[index].position.x));
        CHECK(mesh.vertices[index].position.y ==
              doctest::Approx(repeated.vertices[index].position.y));
        CHECK(mesh.vertices[index].position.z ==
              doctest::Approx(repeated.vertices[index].position.z));
    }
}

TEST_CASE("sphere radius changes generated geometry bounds")
{
    const ai3::SphereMesh small = ai3::make_sphere_mesh(1.0F);
    const ai3::SphereMesh large = ai3::make_sphere_mesh(2.5F);
    REQUIRE(small.vertices.size() == large.vertices.size());
    for (std::size_t index = 0; index < small.vertices.size(); ++index)
        CHECK(glm::length(large.vertices[index].position) ==
              doctest::Approx(glm::length(small.vertices[index].position) * 2.5F));
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
