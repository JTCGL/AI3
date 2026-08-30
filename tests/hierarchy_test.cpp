#include <doctest/doctest.h>

#include "editor/editor_state.h"
#include "scene/orbit_camera.h"
#include "scene/scene_math.h"
#include "scene/sphere_mesh.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <string>

namespace
{
constexpr float epsilon = 0.0002F;

ai3::Transform transform(glm::vec3 position = {}, glm::vec3 euler = {},
                         glm::vec3 scale = glm::vec3{1.0F})
{
    return {position, ai3::orientation_from_euler_degrees(euler), scale};
}

ai3::CreateObject object(std::string name, ai3::ObjectCategory category,
                         ai3::ObjectId parent = ai3::no_object, ai3::Transform local = {})
{
    ai3::CreateObject created{std::move(name), parent, local};
    created.category = category;
    if (category == ai3::ObjectCategory::primitive)
        created.primitive_kind = ai3::PrimitiveKind::sphere;
    else if (category == ai3::ObjectCategory::camera)
        created.camera_kind = ai3::CameraKind::perspective;
    else if (category == ai3::ObjectCategory::light)
        created.light_kind = ai3::LightKind::directional;
    return created;
}

void check_vec3(const glm::vec3& actual, const glm::vec3& expected)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(epsilon));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(epsilon));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(epsilon));
}

void check_matrix(const glm::mat4& actual, const glm::mat4& expected)
{
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]).epsilon(epsilon));
}

void check_transform(const ai3::Transform& actual, const ai3::Transform& expected)
{
    check_vec3(actual.position, expected.position);
    check_vec3(actual.scale, expected.scale);
    CHECK(std::abs(
              glm::dot(glm::normalize(actual.orientation), glm::normalize(expected.orientation))) ==
          doctest::Approx(1.0F).epsilon(epsilon));
}
} // namespace

TEST_CASE("world transforms recursively inherit translation rotation and scale")
{
    ai3::EditorState scene;
    const ai3::ObjectId root = scene.create_object(
        object("Root", ai3::ObjectCategory::general, ai3::no_object,
               transform({2.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 90.0F}, {2.0F, 2.0F, 2.0F})));
    const ai3::ObjectId child = scene.create_object(
        object("Child", ai3::ObjectCategory::primitive, root, transform({1.0F, 0.0F, 0.0F})));
    const ai3::ObjectId grandchild = scene.create_object(
        object("Grandchild", ai3::ObjectCategory::light, child, transform({0.0F, 1.0F, 0.0F})));

    check_matrix(scene.world_transform_matrix(root),
                 ai3::compose_transform(scene.find_object(root)->transform));
    check_vec3(scene.world_position(child), {2.0F, 2.0F, 0.0F});
    check_vec3(scene.world_position(grandchild), {0.0F, 2.0F, 0.0F});
    check_vec3(glm::vec3{scene.world_transform_matrix(child) * glm::vec4{1.0F, 0.0F, 0.0F, 0.0F}},
               {0.0F, 2.0F, 0.0F});
    check_vec3(scene.world_orientation(child) * glm::vec3{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
}

TEST_CASE("mixed object categories can parent each other and inherit directions")
{
    ai3::EditorState scene;
    const ai3::ObjectId primitive =
        scene.create_object(object("Primitive", ai3::ObjectCategory::primitive, ai3::no_object,
                                   transform({}, {90.0F, 0.0F, 0.0F})));
    const ai3::ObjectId camera =
        scene.create_object(object("Camera", ai3::ObjectCategory::camera, primitive));
    const ai3::ObjectId light =
        scene.create_object(object("Light", ai3::ObjectCategory::light, camera));
    const ai3::ObjectId second_primitive =
        scene.create_object(object("Primitive 2", ai3::ObjectCategory::primitive, light));
    const ai3::ObjectId second_light =
        scene.create_object(object("Light 2", ai3::ObjectCategory::light, second_primitive));

    check_vec3(ai3::camera_forward_direction(scene, camera), {0.0F, 1.0F, 0.0F});
    check_vec3(ai3::directional_light_direction(scene, light), {0.0F, 1.0F, 0.0F});
    CHECK(scene.find_object(second_light)->parent_id() == second_primitive);
    CHECK(scene.children_of(camera) == std::vector<ai3::ObjectId>{light});
}

TEST_CASE("hierarchy legality is independent of every object category pairing")
{
    for (ai3::ObjectCategory parent_category :
         {ai3::ObjectCategory::primitive, ai3::ObjectCategory::camera, ai3::ObjectCategory::light})
        for (ai3::ObjectCategory child_category :
             {ai3::ObjectCategory::primitive, ai3::ObjectCategory::camera,
              ai3::ObjectCategory::light})
        {
            ai3::EditorState scene;
            const ai3::ObjectId parent = scene.create_object(object("Parent", parent_category));
            const ai3::ObjectId child = scene.create_object(object("Child", child_category));
            REQUIRE(scene.reparent_object(child, parent));
            CHECK(scene.find_object(child)->parent_id() == parent);
        }
}

TEST_CASE("parenting unparenting and reparenting preserve complete world pose")
{
    ai3::EditorState scene;
    const ai3::ObjectId parent_a = scene.create_object(
        object("A", ai3::ObjectCategory::camera, ai3::no_object,
               transform({5.0F, -2.0F, 1.0F}, {0.0F, 0.0F, 30.0F}, {2.0F, 2.0F, 2.0F})));
    const ai3::ObjectId parent_b = scene.create_object(
        object("B", ai3::ObjectCategory::light, ai3::no_object,
               transform({-4.0F, 3.0F, 2.0F}, {20.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F})));
    const ai3::ObjectId child = scene.create_object(
        object("Child", ai3::ObjectCategory::primitive, ai3::no_object,
               transform({10.0F, 1.0F, -2.0F}, {10.0F, 20.0F, 30.0F}, {1.5F, 2.0F, 0.75F})));
    const glm::mat4 original_world = scene.world_transform_matrix(child);

    REQUIRE(scene.reparent_object(child, parent_a));
    CHECK(scene.find_object(child)->parent_id() == parent_a);
    check_matrix(scene.world_transform_matrix(child), original_world);
    REQUIRE(scene.reparent_object(child, ai3::no_object));
    CHECK(scene.find_object(child)->parent_id() == ai3::no_object);
    check_matrix(scene.world_transform_matrix(child), original_world);
    REQUIRE(scene.reparent_object(child, parent_a));
    REQUIRE(scene.reparent_object(child, parent_b));
    CHECK(scene.find_object(child)->parent_id() == parent_b);
    check_matrix(scene.world_transform_matrix(child), original_world);

    scene.find_object(parent_b)->transform.position.x += 5.0F;
    check_vec3(scene.world_position(child),
               glm::vec3{original_world[3]} + glm::vec3{5.0F, 0.0F, 0.0F});
}

TEST_CASE("invalid hierarchy changes are rejected transactionally")
{
    ai3::EditorState scene;
    const ai3::ObjectId root = scene.create_object(object("Root", ai3::ObjectCategory::primitive));
    const ai3::ObjectId child = scene.create_object(
        object("Child", ai3::ObjectCategory::camera, root, transform({1, 2, 3})));
    const ai3::ObjectId leaf =
        scene.create_object(object("Leaf", ai3::ObjectCategory::light, child));
    const ai3::Transform before = scene.find_object(root)->transform;
    const glm::mat4 world_before = scene.world_transform_matrix(root);

    CHECK_FALSE(scene.reparent_object(root, root));
    CHECK_FALSE(scene.reparent_object(root, child));
    CHECK_FALSE(scene.reparent_object(root, leaf));
    CHECK_FALSE(scene.reparent_object(root, 999999));
    CHECK_FALSE(scene.reparent_object(999999, child));
    CHECK(scene.find_object(root)->parent_id() == ai3::no_object);
    check_transform(scene.find_object(root)->transform, before);
    check_matrix(scene.world_transform_matrix(root), world_before);
}

TEST_CASE("deep hierarchy resolution is deterministic and reset removes hierarchy state")
{
    ai3::EditorState scene;
    ai3::ObjectId parent = ai3::no_object;
    for (int index = 0; index < 128; ++index)
        parent = scene.create_object(object(std::to_string(index), ai3::ObjectCategory::general,
                                            parent, transform({1.0F, 0.0F, 0.0F})));
    const glm::mat4 first = scene.world_transform_matrix(parent);
    check_vec3(scene.world_position(parent), {128.0F, 0.0F, 0.0F});
    check_matrix(scene.world_transform_matrix(parent), first);
    scene.reset_scene();
    CHECK(scene.objects().empty());
    CHECK(scene.children_of(ai3::no_object).empty());
    CHECK(scene.create_object(object("New", ai3::ObjectCategory::general)) == 1);
}

TEST_CASE("recursive deletion handles mixed category hierarchy")
{
    ai3::EditorState scene;
    const ai3::ObjectId light = scene.create_object(object("Light", ai3::ObjectCategory::light));
    const ai3::ObjectId camera = scene.create_object(object("Camera", ai3::ObjectCategory::camera));
    const ai3::ObjectId primitive =
        scene.create_object(object("Primitive", ai3::ObjectCategory::primitive));
    REQUIRE(scene.reparent_object(light, camera));
    REQUIRE(scene.reparent_object(camera, primitive));
    REQUIRE(scene.delete_object(primitive));
    CHECK(scene.find_object(primitive) == nullptr);
    CHECK(scene.find_object(camera) == nullptr);
    CHECK(scene.find_object(light) == nullptr);
}

TEST_CASE("render-facing world matrix does not affect sphere geometry semantics")
{
    ai3::EditorState scene;
    const ai3::ObjectId parent = scene.create_object(
        object("Parent", ai3::ObjectCategory::camera, ai3::no_object, transform({4, 5, 6})));
    ai3::CreateObject sphere =
        object("Sphere", ai3::ObjectCategory::primitive, parent, transform({1.0F, 0.0F, 0.0F}));
    sphere.sphere.radius_meters = 2.5F;
    const ai3::ObjectId id = scene.create_object(sphere);
    const ai3::SphereMesh before =
        ai3::make_sphere_mesh(scene.find_object(id)->sphere.radius_meters);
    check_vec3(glm::vec3{scene.world_transform_matrix(id)[3]}, {5.0F, 5.0F, 6.0F});
    scene.find_object(parent)->transform.position = {-1.0F, -2.0F, -3.0F};
    const ai3::SphereMesh after =
        ai3::make_sphere_mesh(scene.find_object(id)->sphere.radius_meters);
    CHECK(before.indices == after.indices);
    CHECK(before.vertices.size() == after.vertices.size());
}

TEST_CASE("coordinate-space bases distinguish local parent world and editor view")
{
    ai3::EditorState scene;
    const ai3::ObjectId parent =
        scene.create_object(object("Parent", ai3::ObjectCategory::general, ai3::no_object,
                                   transform({}, {0.0F, 0.0F, 90.0F})));
    const ai3::ObjectId child = scene.create_object(object(
        "Child", ai3::ObjectCategory::primitive, parent, transform({}, {90.0F, 0.0F, 0.0F})));
    const glm::mat3 local = ai3::coordinate_space_basis(scene, child, ai3::CoordinateSpace::local);
    const glm::mat3 parent_basis =
        ai3::coordinate_space_basis(scene, child, ai3::CoordinateSpace::parent);
    const glm::mat3 world = ai3::coordinate_space_basis(scene, child, ai3::CoordinateSpace::world);
    ai3::OrbitCamera editor_camera;
    const glm::mat3 view = ai3::coordinate_space_basis(scene, child, ai3::CoordinateSpace::view,
                                                       editor_camera.view_matrix());

    check_vec3(parent_basis * glm::vec3{1, 0, 0}, {0, 1, 0});
    check_vec3(local * glm::vec3{0, 1, 0}, {0, 0, 1});
    check_vec3(world * glm::vec3{1, 0, 0}, {1, 0, 0});
    check_vec3(view * glm::vec3{0, 0, 1}, glm::normalize(editor_camera.position()));
}

TEST_CASE("reparent rejects shear that cannot be represented by local TRS")
{
    ai3::EditorState scene;
    const ai3::ObjectId parent =
        scene.create_object(object("Parent", ai3::ObjectCategory::general, ai3::no_object,
                                   transform({}, {0.0F, 0.0F, 45.0F}, {2.0F, 1.0F, 1.0F})));
    const ai3::ObjectId child =
        scene.create_object(object("Child", ai3::ObjectCategory::primitive, ai3::no_object,
                                   transform({3.0F, 4.0F, 0.0F}, {0.0F, 0.0F, -20.0F})));
    const ai3::Transform local_before = scene.find_object(child)->transform;
    const glm::mat4 world_before = scene.world_transform_matrix(child);

    CHECK_FALSE(scene.reparent_object(child, parent));
    CHECK(scene.find_object(child)->parent_id() == ai3::no_object);
    check_transform(scene.find_object(child)->transform, local_before);
    check_matrix(scene.world_transform_matrix(child), world_before);
}

TEST_CASE("verified reflected TRS reparenting preserves world pose")
{
    ai3::EditorState scene;
    const ai3::ObjectId reflected_parent =
        scene.create_object(object("Reflected", ai3::ObjectCategory::general, ai3::no_object,
                                   transform({2.0F, 0.0F, 0.0F}, {}, {-1.0F, 1.0F, 1.0F})));
    const ai3::ObjectId child =
        scene.create_object(object("Child", ai3::ObjectCategory::primitive, ai3::no_object,
                                   transform({4.0F, 1.0F, 0.0F}, {}, {2.0F, 3.0F, 4.0F})));
    const glm::mat4 before = scene.world_transform_matrix(child);
    REQUIRE(scene.reparent_object(child, reflected_parent));
    check_matrix(scene.world_transform_matrix(child), before);
}
