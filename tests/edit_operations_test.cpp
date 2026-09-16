#include <doctest/doctest.h>

#include "core/edit_operations.h"

#include <stdexcept>

namespace
{
struct CoreEditingStack
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history{scene, workspace};
    ai3::EditOperations operations{scene, workspace, history};
};
} // namespace

TEST_CASE("discrete Core operations create exactly one history entry for real changes")
{
    CoreEditingStack core;
    const auto baseline = core.history.current_state_id();

    const ai3::ObjectId sphere = core.operations.create_sphere("Sphere", {2.0F});
    CHECK(core.history.current_state_id() != baseline);
    REQUIRE(core.history.undo());
    CHECK(core.scene.find_object(sphere) == nullptr);
    REQUIRE(core.history.redo());
    CHECK(core.scene.find_object(sphere)->sphere.radius_meters == doctest::Approx(2.0F));

    const auto sphere_state = core.history.current_state_id();
    CHECK(core.operations.rename_object(sphere, "Sphere 1"));
    CHECK(core.history.current_state_id() == sphere_state);
    CHECK_FALSE(core.operations.rename_object(9999, "Missing"));
    CHECK(core.history.current_state_id() == sphere_state);
    CHECK_THROWS_AS(core.operations.set_sphere(sphere, {-1.0F}), std::invalid_argument);
    CHECK(core.history.current_state_id() == sphere_state);
    CHECK_FALSE(core.history.transaction_active());
}

TEST_CASE("typed Core operations cover authored object material and hierarchy semantics")
{
    CoreEditingStack core;
    const ai3::ObjectId root = core.operations.create_object(ai3::CreateObject{"Root"});
    const ai3::ObjectId sphere = core.operations.create_sphere("Sphere");
    const ai3::ObjectId box = core.operations.create_box("Box");
    const ai3::ObjectId camera = core.operations.create_perspective_camera("Camera");
    const ai3::ObjectId light = core.operations.create_directional_light("Light");
    const ai3::MaterialId material = core.operations.create_material("Material");

    REQUIRE(core.operations.set_object_enabled(sphere, false));
    REQUIRE(core.operations.set_object_visible(sphere, false));
    REQUIRE(core.operations.set_sphere(sphere, {3.0F}));
    REQUIRE(core.operations.set_box(box, {2.0F, 3.0F, 4.0F, 2, 3, 4}));
    REQUIRE(core.operations.set_perspective_camera(camera, {65.0F, 0.25F, 500.0F}));
    REQUIRE(core.operations.set_directional_light(light, {{0.5F, 0.25F, 1.0F}, 2.0F}));
    REQUIRE(core.operations.rename_material(material, "Edited Material"));
    REQUIRE(core.operations.assign_material(sphere, material));

    ai3::Material edited = *core.scene.find_material(material);
    edited.specular_power = 64.0F;
    REQUIRE(core.operations.set_material(material, edited));
    ai3::Transform transform;
    transform.position = {1.0F, 2.0F, 3.0F};
    REQUIRE(core.operations.set_local_transform(root, transform));
    REQUIRE(core.operations.reparent_object(sphere, root));
    REQUIRE(core.operations.set_world_position(sphere, {4.0F, 5.0F, 6.0F}));
    CHECK_FALSE(core.operations.reparent_object(root, sphere));

    CHECK(core.scene.find_object(sphere)->sphere.material_id == material);
    CHECK(core.scene.world_position(sphere) == glm::vec3{4.0F, 5.0F, 6.0F});
    CHECK(core.scene.find_object(box)->box.height_segments == 4);
    CHECK(core.scene.find_object(camera)->perspective_camera.far_plane_meters ==
          doctest::Approx(500.0F));
    CHECK(core.scene.find_object(light)->directional_light.intensity == doctest::Approx(2.0F));
}

TEST_CASE("Core operation transactions group repeated edits and cancellation restores exact state")
{
    CoreEditingStack core;
    const ai3::ObjectId sphere = core.operations.create_sphere("Sphere");
    core.history.rebaseline();
    const ai3::DocumentRevision before = core.scene.document_revision();

    REQUIRE(core.history.begin_transaction());
    REQUIRE(core.operations.set_world_position(sphere, {1.0F, 0.0F, 0.0F}));
    REQUIRE(core.operations.set_world_position(sphere, {2.0F, 0.0F, 0.0F}));
    REQUIRE(core.history.commit_transaction());
    REQUIRE(core.history.undo());
    CHECK(core.scene.world_position(sphere) == glm::vec3{0.0F});
    CHECK(core.scene.document_revision() > before);
    REQUIRE(core.history.redo());
    CHECK(core.scene.world_position(sphere) == glm::vec3{2.0F, 0.0F, 0.0F});

    REQUIRE(core.history.begin_transaction());
    REQUIRE(core.operations.set_sphere(sphere, {5.0F}));
    REQUIRE(core.operations.rename_object(sphere, "Temporary"));
    REQUIRE(core.history.cancel_transaction());
    CHECK(core.scene.find_object(sphere)->sphere.radius_meters == doctest::Approx(1.0F));
    CHECK(core.scene.find_object(sphere)->name == "Sphere 1");
}

TEST_CASE("deletion coordinates Workspace lifecycle state without making Workspace undoable")
{
    CoreEditingStack core;
    const ai3::ObjectId parent = core.operations.create_object(ai3::CreateObject{"Parent"});
    const ai3::ObjectId child = core.operations.create_sphere("Sphere");
    ai3::Transform child_transform;
    child_transform.position = {1.0F, 2.0F, 3.0F};
    REQUIRE(core.operations.set_local_transform(child, child_transform));
    REQUIRE(core.operations.reparent_object(child, parent));
    const glm::vec3 child_world = core.scene.world_position(child);
    REQUIRE(core.operations.select(parent));
    REQUIRE(core.operations.set_bounds_display(child, {true, false, true}));
    core.workspace.set_active_material(77);
    core.workspace.set_display_length_unit(ai3::LengthUnit::centimeter);
    core.history.rebaseline();

    REQUIRE(core.operations.delete_object(parent));
    CHECK(core.workspace.selection() == ai3::no_object);
    CHECK(core.scene.find_object(child)->parent_id() == ai3::no_object);
    CHECK(core.scene.world_position(child) == child_world);
    REQUIRE(core.history.undo());
    CHECK(core.workspace.selection() == ai3::no_object);
    CHECK(core.workspace.bounds_display(child).show_bounding_box);
    CHECK(core.workspace.active_material() == 77);
    CHECK(core.workspace.display_length_unit() == ai3::LengthUnit::centimeter);

    REQUIRE(core.operations.select(child));
    REQUIRE(core.operations.delete_object(child));
    CHECK_FALSE(core.workspace.bounds_display(child).show_bounding_box);
    REQUIRE(core.history.undo());
    CHECK(core.workspace.bounds_display(child).show_bounding_box);
    REQUIRE(core.history.redo());
    CHECK_FALSE(core.workspace.bounds_display(child).show_bounding_box);

    REQUIRE(core.operations.reset_scene());
    CHECK(core.scene.objects().empty());
    REQUIRE(core.history.undo());
    CHECK(core.scene.find_object(child) == nullptr);
}
