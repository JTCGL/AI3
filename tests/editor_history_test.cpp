#include <doctest/doctest.h>

#include "editor/editor_history.h"
#include "scene/scene_math.h"

namespace
{
template <typename Edit> bool transact(ai3::EditorHistory& history, Edit&& edit)
{
    REQUIRE(history.begin_transaction());
    edit();
    return history.commit_transaction();
}
} // namespace

TEST_CASE("transactions group live mutations and support undo redo cancel and no-ops")
{
    ai3::EditorState state;
    ai3::EditorHistory history(state);
    REQUIRE(transact(history, [&] { state.create_sphere("Sphere"); }));
    CHECK(history.can_undo());
    CHECK(state.objects().size() == 1);
    REQUIRE(history.undo());
    CHECK(state.objects().empty());
    REQUIRE(history.redo());
    CHECK(state.objects().size() == 1);

    REQUIRE(history.begin_transaction());
    state.rename_object(1, "Intermediate");
    state.rename_object(1, "Final");
    REQUIRE(history.commit_transaction());
    REQUIRE(history.undo());
    CHECK(state.find_object(1)->name == "Sphere 1");
    REQUIRE(history.redo());
    CHECK(state.find_object(1)->name == "Final");

    REQUIRE(history.begin_transaction());
    state.rename_object(1, "Final");
    CHECK_FALSE(history.commit_transaction());
    CHECK_FALSE(history.can_redo());

    REQUIRE(history.begin_transaction());
    state.set_sphere(1, {4.0F});
    REQUIRE(history.cancel_transaction());
    CHECK(state.find_object(1)->sphere.radius_meters == doctest::Approx(1.0F));
}

TEST_CASE("history restores exact authoritative scene state and allocator metadata")
{
    ai3::EditorState state;
    ai3::EditorHistory history(state);
    ai3::ObjectId root = ai3::no_object;
    ai3::ObjectId sphere = ai3::no_object;
    ai3::ObjectId camera = ai3::no_object;
    REQUIRE(transact(history,
                     [&]
                     {
                         root = state.create_object(ai3::CreateObject{"Root"});
                         sphere = state.create_sphere("Sphere", {2.5F});
                         camera = state.create_perspective_camera("Camera");
                         ai3::Transform transform;
                         transform.position = {1.0F, 2.0F, 3.0F};
                         transform.orientation =
                             ai3::orientation_from_euler_degrees({10.0F, 20.0F, 30.0F});
                         transform.scale = {2.0F, 3.0F, 4.0F};
                         state.set_local_transform(sphere, transform);
                         state.reparent_object(sphere, root);
                         state.set_perspective_camera(camera, {65.0F, 0.25F, 500.0F});
                     }));
    const ai3::Transform expected_transform = state.find_object(sphere)->transform;

    REQUIRE(transact(history,
                     [&]
                     {
                         state.delete_object(root);
                         state.set_sphere(sphere, {9.0F});
                         state.create_directional_light("Light");
                     }));
    REQUIRE(history.undo());
    REQUIRE(state.objects().size() == 3);
    CHECK(state.objects()[0].id == root);
    CHECK(state.objects()[1].id == sphere);
    CHECK(state.objects()[2].id == camera);
    CHECK(state.find_object(sphere)->parent_id() == root);
    CHECK(state.find_object(sphere)->transform.position == expected_transform.position);
    CHECK(state.find_object(sphere)->transform.scale == expected_transform.scale);
    CHECK(state.find_object(sphere)->sphere.radius_meters == doctest::Approx(2.5F));
    CHECK(state.find_object(camera)->perspective_camera.far_plane_meters ==
          doctest::Approx(500.0F));

    REQUIRE(transact(history,
                     [&]
                     {
                         CHECK(state.create_sphere("Sphere") == 4);
                         CHECK(state.find_object(4)->name == "Sphere 2");
                         CHECK(state.create_directional_light("Light") == 5);
                         CHECK(state.find_object(5)->name == "Light 1");
                     }));
    CHECK_FALSE(history.can_redo());
}

TEST_CASE("delete reparent reset invalid edits and revisions obey transaction semantics")
{
    ai3::EditorState state;
    ai3::EditorHistory history(state);
    ai3::ObjectId parent = ai3::no_object;
    ai3::ObjectId child = ai3::no_object;
    transact(history,
             [&]
             {
                 parent = state.create_object(ai3::CreateObject{"Parent"});
                 child = state.create_sphere("Sphere");
             });
    const ai3::DocumentRevision before = state.document_revision();
    REQUIRE(transact(history, [&] { state.reparent_object(child, parent); }));
    REQUIRE(history.undo());
    CHECK(state.find_object(child)->parent_id() == ai3::no_object);
    REQUIRE(history.redo());
    CHECK(state.find_object(child)->parent_id() == parent);
    CHECK(state.document_revision() > before);

    REQUIRE(transact(history, [&] { state.delete_object(parent); }));
    REQUIRE(history.undo());
    CHECK(state.find_object(parent) != nullptr);
    REQUIRE(transact(history, [&] { state.reset_scene(); }));
    CHECK(state.objects().empty());
    REQUIRE(history.undo());
    CHECK(state.find_object(parent) != nullptr);

    REQUIRE(history.begin_transaction());
    CHECK_FALSE(state.reparent_object(child, child));
    CHECK_FALSE(history.commit_transaction());
}
