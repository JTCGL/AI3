#include <doctest/doctest.h>

#include "scene/scene_math.h"
#include "scene/viewport_picking.h"
#include "scene/viewport_view.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

namespace
{
ai3::ObjectId create_sphere(ai3::EditorState& scene, ai3::Transform transform = {},
                            ai3::ObjectId parent = ai3::no_object, float radius = 1.0F,
                            bool enabled = true, bool visible = true)
{
    ai3::CreateObject object{"Sphere", parent, transform, enabled, visible};
    object.category = ai3::ObjectCategory::primitive;
    object.primitive_kind = ai3::PrimitiveKind::sphere;
    object.sphere.radius_meters = radius;
    return scene.create_object(object);
}

ai3::ObjectId create_camera(ai3::EditorState& scene, ai3::Transform transform = {})
{
    ai3::CreateObject object{"Camera", ai3::no_object, transform};
    object.category = ai3::ObjectCategory::camera;
    object.camera_kind = ai3::CameraKind::perspective;
    return scene.create_object(object);
}

void check_direction(const glm::vec3& actual, const glm::vec3& expected)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.0001F));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.0001F));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.0001F));
}
} // namespace

TEST_CASE("viewport coordinates construct center and off-center world rays")
{
    ai3::EditorState scene;
    ai3::ViewportView viewport;
    const ai3::ResolvedViewportView view = viewport.resolve(scene, 1.0F);
    const ai3::WorldRay center = ai3::viewport_world_ray({0.5F, 0.5F}, view);
    check_direction(center.direction,
                    glm::normalize(viewport.orbit().target() - viewport.orbit().position()));
    const ai3::WorldRay upper_right = ai3::viewport_world_ray({1.0F, 0.0F}, view);
    CHECK(glm::dot(center.direction, upper_right.direction) < 0.95F);
    CHECK(glm::length(upper_right.direction) == doctest::Approx(1.0F));
}

TEST_CASE("picking works through orbit and scene-camera resolved views")
{
    ai3::EditorState orbit_scene;
    const ai3::ObjectId orbit_sphere = create_sphere(orbit_scene);
    ai3::ViewportView orbit_viewport;
    const ai3::WorldRay orbit_ray =
        ai3::viewport_world_ray({0.5F, 0.5F}, orbit_viewport.resolve(orbit_scene, 1.0F));
    CHECK(ai3::pick_sphere(orbit_scene, orbit_ray) == orbit_sphere);

    ai3::EditorState camera_scene;
    const ai3::ObjectId camera = create_camera(camera_scene);
    ai3::Transform sphere_transform;
    sphere_transform.position = {0.0F, 0.0F, -5.0F};
    const ai3::ObjectId camera_sphere = create_sphere(camera_scene, sphere_transform);
    ai3::ViewportView camera_viewport;
    REQUIRE(camera_viewport.use_scene_camera(camera_scene, camera));
    const ai3::WorldRay camera_ray =
        ai3::viewport_world_ray({0.5F, 0.5F}, camera_viewport.resolve(camera_scene, 1.0F));
    CHECK(ai3::pick_sphere(camera_scene, camera_ray) == camera_sphere);
}

TEST_CASE("picking uses authoritative translated and hierarchical sphere transforms")
{
    ai3::EditorState scene;
    ai3::Transform parent_transform;
    parent_transform.position = {1.0F, 0.0F, 0.0F};
    parent_transform.orientation = ai3::orientation_from_euler_degrees({0.0F, 0.0F, 90.0F});
    parent_transform.scale = {2.0F, 1.0F, 1.0F};
    const ai3::ObjectId parent =
        scene.create_object(ai3::CreateObject{"Parent", ai3::no_object, parent_transform});
    ai3::Transform child_transform;
    child_transform.position = {0.0F, -1.0F, -5.0F};
    child_transform.orientation = ai3::orientation_from_euler_degrees({30.0F, 20.0F, 10.0F});
    child_transform.scale = {0.5F, 2.0F, 1.5F};
    const ai3::ObjectId sphere = create_sphere(scene, child_transform, parent);
    const glm::vec3 center = scene.world_position(sphere);
    const ai3::WorldRay ray{{center.x, center.y, center.z + 10.0F}, {0.0F, 0.0F, -1.0F}};
    CHECK(ai3::pick_sphere(scene, ray) == sphere);
}

TEST_CASE("inverse-local intersection handles non-uniform and reflected scale")
{
    ai3::EditorState scene;
    ai3::Transform uniform;
    uniform.position = {-6.0F, 0.0F, -6.0F};
    uniform.scale = {2.0F, 2.0F, 2.0F};
    const ai3::ObjectId uniform_id = create_sphere(scene, uniform);
    CHECK(ai3::pick_sphere(scene, {{-6.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == uniform_id);

    ai3::Transform stretched;
    stretched.position = {0.0F, 0.0F, -6.0F};
    stretched.scale = {4.0F, 0.25F, 1.0F};
    const ai3::ObjectId non_uniform = create_sphere(scene, stretched);
    CHECK(ai3::pick_sphere(scene, {{0.0F, 0.3F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == ai3::no_object);
    CHECK(ai3::pick_sphere(scene, {{3.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == non_uniform);

    ai3::Transform reflected;
    reflected.position = {7.0F, 0.0F, -6.0F};
    reflected.scale = {-2.0F, 1.0F, 1.0F};
    const ai3::ObjectId reflected_id = create_sphere(scene, reflected);
    CHECK(ai3::pick_sphere(scene, {{7.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == reflected_id);
}

TEST_CASE("nearest visible enabled sphere wins and misses clear to no object")
{
    ai3::EditorState scene;
    ai3::Transform hidden_transform;
    hidden_transform.position = {0.0F, 0.0F, -2.0F};
    create_sphere(scene, hidden_transform, ai3::no_object, 1.0F, true, false);
    ai3::Transform disabled_transform;
    disabled_transform.position = {0.0F, 0.0F, -3.0F};
    create_sphere(scene, disabled_transform, ai3::no_object, 1.0F, false, true);
    ai3::Transform near_transform;
    near_transform.position = {0.0F, 0.0F, -5.0F};
    const ai3::ObjectId near = create_sphere(scene, near_transform);
    ai3::Transform far_transform;
    far_transform.position = {0.0F, 0.0F, -9.0F};
    create_sphere(scene, far_transform);
    CHECK(ai3::pick_sphere(scene, {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == near);
    CHECK(ai3::pick_sphere(scene, {{20.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == ai3::no_object);
}

TEST_CASE("nearest hit comparison retains the shared world-ray parameter across scales")
{
    ai3::EditorState scene;
    ai3::Transform farther_stretched;
    farther_stretched.position = {0.0F, 0.0F, -10.0F};
    farther_stretched.scale = {0.5F, 2.0F, 5.0F};
    create_sphere(scene, farther_stretched);

    ai3::Transform nearer_compressed;
    nearer_compressed.position = {0.0F, 0.0F, -4.0F};
    nearer_compressed.scale = {3.0F, 0.25F, 0.5F};
    const ai3::ObjectId true_nearest = create_sphere(scene, nearer_compressed);

    // World-space front hits are 5.0 m and 3.5 m respectively. Normalizing each inverse-
    // transformed local direction would instead compare incompatible parameters 1.0 and 7.0.
    CHECK(ai3::pick_sphere(scene, {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == true_nearest);
}

TEST_CASE("non-invertible sphere transforms fail safely")
{
    ai3::EditorState scene;
    ai3::Transform singular;
    singular.position = {0.0F, 0.0F, -2.0F};
    singular.scale = {1.0F, 0.0F, 1.0F};
    create_sphere(scene, singular);
    CHECK(ai3::pick_sphere(scene, {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}}) == ai3::no_object);
}

TEST_CASE("resolved ray excludes spheres beyond the projection far plane")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    ai3::Transform beyond_far;
    beyond_far.position = {0.0F, 0.0F, -150.0F};
    create_sphere(scene, beyond_far);
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));
    const ai3::WorldRay ray = ai3::viewport_world_ray({0.5F, 0.5F}, viewport.resolve(scene, 1.0F));
    CHECK(ai3::pick_sphere(scene, ray) == ai3::no_object);
}
