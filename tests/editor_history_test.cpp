#include <doctest/doctest.h>

#include "core/edit_operations.h"

#include <glm/gtc/quaternion.hpp>

namespace
{
template <typename Edit> bool transact(ai3::EditHistory& history, Edit&& edit)
{
    REQUIRE(history.begin_transaction());
    edit();
    return history.commit_transaction();
}
} // namespace

TEST_CASE("transactions group live mutations and support undo redo cancel and no-ops")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    REQUIRE(transact(history, [&] { operations.create_sphere("Sphere"); }));
    CHECK(history.can_undo());
    CHECK(scene.objects().size() == 1);
    REQUIRE(history.undo());
    CHECK(scene.objects().empty());
    REQUIRE(history.redo());
    CHECK(scene.objects().size() == 1);

    REQUIRE(history.begin_transaction());
    operations.rename_object(1, "Intermediate");
    operations.rename_object(1, "Final");
    REQUIRE(history.commit_transaction());
    REQUIRE(history.undo());
    CHECK(scene.find_object(1)->name == "Sphere 1");
    REQUIRE(history.redo());
    CHECK(scene.find_object(1)->name == "Final");

    REQUIRE(history.begin_transaction());
    operations.rename_object(1, "Final");
    CHECK_FALSE(history.commit_transaction());
    CHECK_FALSE(history.can_redo());

    REQUIRE(history.begin_transaction());
    operations.set_sphere(1, {4.0F});
    REQUIRE(history.cancel_transaction());
    CHECK(scene.find_object(1)->sphere.radius_meters == doctest::Approx(1.0F));
}

TEST_CASE("history restores exact authoritative scene state and allocator metadata")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    ai3::ObjectId root = ai3::no_object;
    ai3::ObjectId sphere = ai3::no_object;
    ai3::ObjectId camera = ai3::no_object;
    REQUIRE(transact(history,
                     [&]
                     {
                         root = operations.create_object(ai3::CreateObject{"Root"});
                         sphere = operations.create_sphere("Sphere", {2.5F});
                         camera = operations.create_perspective_camera("Camera");
                         ai3::Transform transform;
                         transform.position = {1.0F, 2.0F, 3.0F};
                         transform.orientation = glm::angleAxis(
                             glm::radians(37.0F), glm::normalize(glm::vec3{1.0F, 2.0F, 3.0F}));
                         transform.scale = {2.0F, 3.0F, 4.0F};
                         operations.set_local_transform(sphere, transform);
                         operations.reparent_object(sphere, root);
                         operations.set_perspective_camera(camera, {65.0F, 0.25F, 500.0F});
                     }));
    const ai3::Transform expected_transform = scene.find_object(sphere)->transform;

    REQUIRE(transact(history,
                     [&]
                     {
                         operations.delete_object(root);
                         operations.set_sphere(sphere, {9.0F});
                         operations.create_directional_light("Light");
                     }));
    REQUIRE(history.undo());
    REQUIRE(scene.objects().size() == 3);
    CHECK(scene.objects()[0].id == root);
    CHECK(scene.objects()[1].id == sphere);
    CHECK(scene.objects()[2].id == camera);
    CHECK(scene.find_object(sphere)->parent_id() == root);
    CHECK(scene.find_object(sphere)->transform.position == expected_transform.position);
    CHECK(scene.find_object(sphere)->transform.orientation == expected_transform.orientation);
    CHECK(scene.find_object(sphere)->transform.scale == expected_transform.scale);
    CHECK(scene.find_object(sphere)->sphere.radius_meters == doctest::Approx(2.5F));
    CHECK(scene.find_object(camera)->perspective_camera.far_plane_meters ==
          doctest::Approx(500.0F));

    REQUIRE(transact(history,
                     [&]
                     {
                         CHECK(operations.create_sphere("Sphere") == 4);
                         CHECK(scene.find_object(4)->name == "Sphere 2");
                         CHECK(operations.create_directional_light("Light") == 5);
                         CHECK(scene.find_object(5)->name == "Light 1");
                     }));
    CHECK_FALSE(history.can_redo());
}

TEST_CASE("delete reparent reset invalid edits and revisions obey transaction semantics")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    ai3::ObjectId parent = ai3::no_object;
    ai3::ObjectId child = ai3::no_object;
    transact(history,
             [&]
             {
                 parent = operations.create_object(ai3::CreateObject{"Parent"});
                 child = operations.create_sphere("Sphere");
             });
    const ai3::DocumentRevision before = scene.document_revision();
    REQUIRE(transact(history, [&] { operations.reparent_object(child, parent); }));
    REQUIRE(history.undo());
    CHECK(scene.find_object(child)->parent_id() == ai3::no_object);
    REQUIRE(history.redo());
    CHECK(scene.find_object(child)->parent_id() == parent);
    CHECK(scene.document_revision() > before);

    REQUIRE(transact(history, [&] { operations.delete_object(parent); }));
    REQUIRE(history.undo());
    CHECK(scene.find_object(parent) != nullptr);
    REQUIRE(transact(history, [&] { operations.reset_scene(); }));
    CHECK(scene.objects().empty());
    REQUIRE(history.undo());
    CHECK(scene.find_object(parent) != nullptr);

    REQUIRE(history.begin_transaction());
    CHECK_FALSE(operations.reparent_object(child, child));
    CHECK_FALSE(history.commit_transaction());
}

TEST_CASE("history does not snapshot the whole Core Workspace")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    const ai3::ObjectId sphere = operations.create_sphere("Sphere");
    history.rebaseline();

    REQUIRE(transact(history, [&] { operations.rename_object(sphere, "Renamed"); }));
    REQUIRE(operations.select(sphere));
    REQUIRE(operations.set_bounds_display(sphere, {true, true, true}));
    workspace.set_active_material(77);
    workspace.set_display_length_unit(ai3::LengthUnit::centimeter);

    REQUIRE(history.undo());
    CHECK(scene.find_object(sphere)->name == "Sphere 1");
    CHECK(workspace.selection() == sphere);
    CHECK(workspace.bounds_display(sphere).show_bounding_box);
    CHECK(workspace.bounds_display(sphere).show_bounding_sphere);
    CHECK(workspace.bounds_display(sphere).hover_feedback);
    CHECK(workspace.active_material() == 77);
    CHECK(workspace.display_length_unit() == ai3::LengthUnit::centimeter);

    REQUIRE(history.redo());
    CHECK(scene.find_object(sphere)->name == "Renamed");
    CHECK(workspace.selection() == sphere);
    CHECK(workspace.bounds_display(sphere).hover_feedback);
    CHECK(workspace.active_material() == 77);
    CHECK(workspace.display_length_unit() == ai3::LengthUnit::centimeter);
}
