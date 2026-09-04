#include <doctest/doctest.h>

#include "editor/document_session.h"
#include "scene/scene_math.h"
#include "scene/viewport_view.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <limits>

namespace
{
void check_matrix(const glm::mat4& actual, const glm::mat4& expected, float epsilon = 0.0001F)
{
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]).epsilon(epsilon));
}

void check_vec3(const glm::vec3& actual, const glm::vec3& expected, float epsilon = 0.0001F)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(epsilon));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(epsilon));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(epsilon));
}

ai3::ObjectId create_camera(ai3::EditorState& scene, const ai3::Transform& transform = {},
                            ai3::PerspectiveCamera camera = {},
                            ai3::ObjectId parent = ai3::no_object)
{
    ai3::CreateObject object{"Camera", parent, transform};
    object.category = ai3::ObjectCategory::camera;
    object.camera_kind = ai3::CameraKind::perspective;
    object.perspective_camera = camera;
    return scene.create_object(object);
}
} // namespace

TEST_CASE("editor viewport resolves its view and aspect-dependent projection")
{
    ai3::EditorState scene;
    ai3::ViewportView viewport;
    const ai3::ResolvedViewportView wide = viewport.resolve(scene, 16.0F / 9.0F);
    const ai3::ResolvedViewportView square = viewport.resolve(scene, 1.0F);

    check_matrix(wide.view, viewport.editor_view().view_matrix());
    check_vec3(wide.eye_position, viewport.editor_view().position());
    check_matrix(wide.projection, viewport.editor_view().projection_matrix(16.0F / 9.0F));
    CHECK(wide.projection[0][0] < square.projection[0][0]);
    CHECK(wide.projection[1][1] == doctest::Approx(square.projection[1][1]));
}

TEST_CASE("viewport interaction mode is workspace state")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    REQUIRE(scene.select(camera));
    ai3::DocumentSession session(scene);
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));
    const ai3::DocumentRevision revision = scene.document_revision();
    const ai3::HistoryStateId history_state = session.history().current_state_id();

    CHECK(viewport.interaction_mode() == ai3::ViewportInteractionMode::selection);
    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK(viewport.interaction_mode() == ai3::ViewportInteractionMode::navigation);
    CHECK(scene.document_revision() == revision);
    CHECK(session.history().current_state_id() == history_state);
    CHECK_FALSE(session.history().can_undo());
    CHECK_FALSE(session.dirty());
    CHECK(scene.selection() == camera);
    CHECK(viewport.source() == ai3::ViewSource::scene_camera);
    CHECK(viewport.scene_camera_id() == camera);

    viewport.reset();
    CHECK(viewport.interaction_mode() == ai3::ViewportInteractionMode::navigation);
}

TEST_CASE("retained orbit requires Navigation while wheel zoom works in either mode")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    ai3::ViewportView viewport;
    const float initial_yaw = viewport.editor_view().yaw_degrees();
    const float initial_pitch = viewport.editor_view().pitch_degrees();
    const float initial_distance = viewport.editor_view().distance();

    CHECK_FALSE(viewport.navigate(4.0F, -2.0F));
    REQUIRE(viewport.zoom(1.0F));
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(initial_yaw));
    CHECK(viewport.editor_view().pitch_degrees() == doctest::Approx(initial_pitch));
    CHECK(viewport.editor_view().distance() != doctest::Approx(initial_distance));

    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    REQUIRE(viewport.navigate(4.0F, -2.0F));
    REQUIRE(viewport.zoom(-1.0F));
    CHECK(viewport.editor_view().yaw_degrees() != doctest::Approx(initial_yaw));
    CHECK(viewport.editor_view().distance() == doctest::Approx(initial_distance));
    const float orbit_yaw = viewport.editor_view().yaw_degrees();
    const float orbit_pitch = viewport.editor_view().pitch_degrees();
    const float orbit_distance = viewport.editor_view().distance();
    const ai3::Transform camera_before = scene.find_object(camera)->transform;
    const ai3::DocumentRevision revision = scene.document_revision();

    viewport.set_interaction_mode(ai3::ViewportInteractionMode::selection);
    REQUIRE(viewport.use_scene_camera(scene, camera));
    CHECK_FALSE(viewport.navigate(10.0F, 10.0F));
    CHECK_FALSE(viewport.zoom(3.0F));
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(orbit_yaw));
    CHECK(viewport.editor_view().pitch_degrees() == doctest::Approx(orbit_pitch));
    CHECK(viewport.editor_view().distance() == doctest::Approx(orbit_distance));
    CHECK(scene.find_object(camera)->transform.position == camera_before.position);
    CHECK(scene.find_object(camera)->transform.orientation == camera_before.orientation);
    CHECK(scene.document_revision() == revision);

    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK_FALSE(viewport.navigate(10.0F, 10.0F));
    CHECK_FALSE(viewport.zoom(3.0F));
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(orbit_yaw));
    CHECK(viewport.editor_view().pitch_degrees() == doctest::Approx(orbit_pitch));
    CHECK(viewport.editor_view().distance() == doctest::Approx(orbit_distance));
    CHECK(scene.find_object(camera)->transform.position == camera_before.position);
    CHECK(scene.find_object(camera)->transform.orientation == camera_before.orientation);
    CHECK(scene.document_revision() == revision);
}

TEST_CASE("transient pan and orbit preserve the retained interaction mode")
{
    ai3::EditorState scene;
    ai3::DocumentSession session(scene);
    ai3::ViewportView viewport;
    const ai3::DocumentRevision revision = scene.document_revision();
    const ai3::HistoryStateId history = session.history().current_state_id();

    const auto pan = viewport.begin_transient_navigation(false, {800.0F, 600.0F});
    REQUIRE(pan.has_value());
    CHECK(pan->kind == ai3::TransientNavigationKind::pan);
    const glm::vec3 target = viewport.editor_view().target();
    REQUIRE(viewport.update_transient_navigation(*pan, {40.0F, -20.0F}));
    CHECK(glm::length(viewport.editor_view().target() - target) > 0.0F);
    CHECK(viewport.interaction_mode() == ai3::ViewportInteractionMode::selection);

    const auto orbit = viewport.begin_transient_navigation(true, {800.0F, 600.0F});
    REQUIRE(orbit.has_value());
    CHECK(orbit->kind == ai3::TransientNavigationKind::orbit);
    const float yaw = viewport.editor_view().yaw_degrees();
    REQUIRE(viewport.update_transient_navigation(*orbit, {12.0F, 0.0F}));
    CHECK(viewport.editor_view().yaw_degrees() != doctest::Approx(yaw));
    CHECK(viewport.interaction_mode() == ai3::ViewportInteractionMode::selection);
    CHECK(scene.document_revision() == revision);
    CHECK(session.history().current_state_id() == history);
    CHECK_FALSE(session.dirty());
}

TEST_CASE("transient navigation freezes its operation and validates acquisition")
{
    ai3::EditorState scene;
    ai3::ViewportView viewport;
    CHECK_FALSE(viewport.begin_transient_navigation(false, {}).has_value());

    const auto acquired_pan = viewport.begin_transient_navigation(false, {800.0F, 600.0F});
    REQUIRE(acquired_pan.has_value());
    CHECK(acquired_pan->kind == ai3::TransientNavigationKind::pan);
    REQUIRE(viewport.update_transient_navigation(*acquired_pan, {10.0F, 0.0F}));
    CHECK(acquired_pan->kind == ai3::TransientNavigationKind::pan);

    const auto acquired_orbit = viewport.begin_transient_navigation(true, {800.0F, 600.0F});
    REQUIRE(acquired_orbit.has_value());
    CHECK(acquired_orbit->kind == ai3::TransientNavigationKind::orbit);
    CHECK_FALSE(viewport.update_transient_navigation(*acquired_orbit, {}));
    CHECK_FALSE(viewport.update_transient_navigation(
        {ai3::TransientNavigationKind::orbit, {}}, {1.0F, 1.0F}));
    CHECK_FALSE(viewport.update_transient_navigation(
        *acquired_orbit, {std::numeric_limits<float>::quiet_NaN(), 1.0F}));

    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK_FALSE(viewport.navigate(std::numeric_limits<float>::quiet_NaN(), 1.0F));
}

TEST_CASE("all editor navigation paths are inert for a Scene Camera")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    ai3::ViewportView viewport;
    const glm::vec3 target = viewport.editor_view().target();
    const float yaw = viewport.editor_view().yaw_degrees();
    const float distance = viewport.editor_view().distance();
    const ai3::Transform camera_before = scene.find_object(camera)->transform;
    const ai3::DocumentRevision revision = scene.document_revision();

    REQUIRE(viewport.use_scene_camera(scene, camera));
    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK_FALSE(viewport.navigate(10.0F, 10.0F));
    CHECK_FALSE(viewport.zoom(2.0F));
    CHECK_FALSE(viewport.begin_transient_navigation(false, {800.0F, 600.0F}).has_value());
    check_vec3(viewport.editor_view().target(), target);
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(yaw));
    CHECK(viewport.editor_view().distance() == doctest::Approx(distance));
    CHECK(scene.find_object(camera)->transform.position == camera_before.position);
    CHECK(scene.find_object(camera)->transform.orientation == camera_before.orientation);
    CHECK(scene.document_revision() == revision);
}

TEST_CASE("editor view state survives scene-camera selection")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    ai3::ViewportView viewport;
    viewport.editor_view().orbit(17.0F, -9.0F);
    viewport.editor_view().zoom(2.0F);
    const float yaw = viewport.editor_view().yaw_degrees();
    const float pitch = viewport.editor_view().pitch_degrees();
    const float distance = viewport.editor_view().distance();

    REQUIRE(viewport.use_scene_camera(scene, camera));
    CHECK(viewport.source() == ai3::ViewSource::scene_camera);
    viewport.resolve(scene, 1.0F);
    viewport.use_editor_view();
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(yaw));
    CHECK(viewport.editor_view().pitch_degrees() == doctest::Approx(pitch));
    CHECK(viewport.editor_view().distance() == doctest::Approx(distance));
}

TEST_CASE("scene-camera view uses current authoritative world transform")
{
    ai3::EditorState scene;
    ai3::Transform camera_transform;
    camera_transform.position = {3.0F, 4.0F, 5.0F};
    camera_transform.orientation = ai3::orientation_from_euler_degrees({20.0F, -15.0F, 35.0F});
    const ai3::ObjectId camera = create_camera(scene, camera_transform);
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));

    ai3::ResolvedViewportView resolved = viewport.resolve(scene, 1.0F);
    check_vec3(resolved.eye_position, scene.world_position(camera));
    check_vec3(glm::vec3{resolved.view * glm::vec4{scene.world_position(camera), 1.0F}},
               glm::vec3{0.0F});
    const glm::vec3 forward = scene.world_orientation(camera) * glm::vec3{0.0F, 0.0F, -1.0F};
    check_vec3(glm::normalize(glm::vec3{resolved.view * glm::vec4{forward, 0.0F}}),
               {0.0F, 0.0F, -1.0F});

    ai3::Transform moved_camera = scene.find_object(camera)->transform;
    moved_camera.position = {-2.0F, 7.0F, 1.0F};
    REQUIRE(scene.set_local_transform(camera, moved_camera));
    resolved = viewport.resolve(scene, 1.0F);
    check_vec3(glm::vec3{resolved.view * glm::vec4{scene.world_position(camera), 1.0F}},
               glm::vec3{0.0F});
}

TEST_CASE("parented scene-camera view uses resolved parent transform")
{
    ai3::EditorState scene;
    ai3::Transform parent_transform;
    parent_transform.position = {10.0F, -3.0F, 2.0F};
    parent_transform.orientation = ai3::orientation_from_euler_degrees({0.0F, 0.0F, 90.0F});
    const ai3::ObjectId parent =
        scene.create_object(ai3::CreateObject{"Parent", ai3::no_object, parent_transform});
    ai3::Transform local;
    local.position = {2.0F, 0.0F, 1.0F};
    local.orientation = ai3::orientation_from_euler_degrees({25.0F, 0.0F, 0.0F});
    const ai3::ObjectId camera = create_camera(scene, local, {}, parent);
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));

    const ai3::ResolvedViewportView first = viewport.resolve(scene, 1.0F);
    check_vec3(first.eye_position, scene.world_position(camera));
    const glm::vec3 first_world_position = scene.world_position(camera);
    check_vec3(glm::vec3{first.view * glm::vec4{scene.world_position(camera), 1.0F}},
               glm::vec3{0.0F});
    ai3::Transform moved_parent = scene.find_object(parent)->transform;
    moved_parent.position.x += 4.0F;
    REQUIRE(scene.set_local_transform(parent, moved_parent));
    const ai3::ResolvedViewportView moved = viewport.resolve(scene, 1.0F);
    CHECK(glm::length(glm::vec3{moved.view * glm::vec4{first_world_position, 1.0F}}) > 1.0F);
    check_vec3(glm::vec3{moved.view * glm::vec4{scene.world_position(camera), 1.0F}},
               glm::vec3{0.0F});
}

TEST_CASE("generic scene-camera source dispatches the current perspective subtype")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene, {}, {60.0F, 0.25F, 250.0F});
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));
    CHECK(viewport.source() == ai3::ViewSource::scene_camera);
    const glm::mat4 square = viewport.resolve(scene, 1.0F).projection;
    const glm::mat4 wide = viewport.resolve(scene, 2.0F).projection;
    CHECK(wide[0][0] == doctest::Approx(square[0][0] * 0.5F));
    CHECK(wide[1][1] == doctest::Approx(square[1][1]));

    REQUIRE(scene.set_perspective_camera(camera, {35.0F, 1.0F, 40.0F}));
    const glm::mat4 changed = viewport.resolve(scene, 1.0F).projection;
    CHECK(changed[1][1] != doctest::Approx(square[1][1]));
    CHECK(changed[2][2] != doctest::Approx(square[2][2]));
    CHECK(changed[3][2] != doctest::Approx(square[3][2]));
}

TEST_CASE("invalid view sources are rejected and deleted camera falls back to Editor View")
{
    ai3::EditorState scene;
    const ai3::ObjectId object = scene.create_object(ai3::CreateObject{"Object"});
    const ai3::ObjectId sphere = scene.create_sphere("Sphere");
    const ai3::ObjectId camera = create_camera(scene);
    ai3::ViewportView viewport;
    CHECK_FALSE(viewport.use_scene_camera(scene, ai3::no_object));
    CHECK_FALSE(viewport.use_scene_camera(scene, object));
    CHECK_FALSE(viewport.use_scene_camera(scene, sphere));
    CHECK(viewport.source() == ai3::ViewSource::editor_view);

    REQUIRE(viewport.use_scene_camera(scene, camera));
    CHECK(viewport.source() == ai3::ViewSource::scene_camera);
    REQUIRE(scene.delete_object(camera));
    const ai3::ResolvedViewportView resolved = viewport.resolve(scene, 1.0F);
    CHECK(viewport.source() == ai3::ViewSource::editor_view);
    CHECK(viewport.scene_camera_id() == ai3::no_object);
    check_matrix(resolved.view, viewport.editor_view().view_matrix());
}

TEST_CASE("scene and viewport reset produce deterministic default Editor View state")
{
    ai3::EditorState scene;
    const ai3::ObjectId camera = create_camera(scene);
    ai3::ViewportView viewport;
    viewport.editor_view().orbit(10.0F, 20.0F);
    viewport.editor_view().zoom(3.0F);
    REQUIRE(viewport.use_scene_camera(scene, camera));

    scene.reset_scene();
    viewport.reset();
    CHECK(scene.objects().empty());
    CHECK(viewport.source() == ai3::ViewSource::editor_view);
    CHECK(viewport.scene_camera_id() == ai3::no_object);
    CHECK(viewport.editor_view().yaw_degrees() == doctest::Approx(35.0F));
    CHECK(viewport.editor_view().pitch_degrees() == doctest::Approx(20.0F));
    CHECK(viewport.editor_view().distance() == doctest::Approx(6.0F));
}
