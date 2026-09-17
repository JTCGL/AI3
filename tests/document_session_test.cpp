#include "core/document_session.h"
#include "core/edit_operations.h"
#include "core/workspace_document.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace
{
struct CoreFixture
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history{scene, workspace};
    ai3::EditOperations operations{scene, workspace, history};
    ai3::DocumentSession session{scene, workspace, history};
};

std::filesystem::path temporary_scene(const char* label)
{
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string{"ai3-m25-"} + label + '-' + std::to_string(nonce) + ".ai3scene");
}

template <typename Edit> void edit(CoreFixture& fixture, Edit&& operation)
{
    REQUIRE(fixture.history.begin_transaction());
    operation();
    fixture.history.commit_transaction();
}
} // namespace

TEST_CASE("Core document session observes authority without rebaselining it")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history{scene, workspace};
    ai3::EditOperations operations{scene, workspace, history};
    const ai3::ObjectId sphere = operations.create_sphere("Sphere");
    REQUIRE(history.can_undo());
    const ai3::HistoryStateId authored_state = history.current_state_id();

    ai3::DocumentSession session{scene, workspace, history};
    CHECK(session.history().current_state_id() == authored_state);
    CHECK(session.history().can_undo());
    CHECK_FALSE(session.dirty());
    REQUIRE(session.history().undo());
    CHECK(scene.find_object(sphere) == nullptr);
}

TEST_CASE("Core document checkpoints track undo redo branching and active changes")
{
    CoreFixture fixture;
    edit(fixture, [&] { fixture.operations.create_sphere("Sphere"); });
    fixture.session.mark_saved();
    CHECK_FALSE(fixture.session.dirty());

    ai3::ContinuousEdit continuous = fixture.operations.begin_continuous_edit();
    REQUIRE(continuous.active());
    REQUIRE(fixture.operations.rename_object(1, "Live"));
    CHECK(fixture.session.dirty());
    REQUIRE(continuous.cancel());
    CHECK_FALSE(fixture.session.dirty());

    edit(fixture, [&] { fixture.operations.rename_object(1, "Edited"); });
    CHECK(fixture.session.dirty());
    REQUIRE(fixture.history.undo());
    CHECK_FALSE(fixture.session.dirty());
    REQUIRE(fixture.history.redo());
    CHECK(fixture.session.dirty());
    REQUIRE(fixture.history.undo());
    edit(fixture, [&] { fixture.operations.set_object_visible(1, false); });
    CHECK(fixture.session.dirty());
    CHECK_FALSE(fixture.history.can_redo());
}

TEST_CASE("Core document pending transitions preserve save discard cancel semantics")
{
    CoreFixture fixture;
    for (const ai3::DocumentTransition transition :
         {ai3::DocumentTransition::new_document, ai3::DocumentTransition::open_document,
          ai3::DocumentTransition::quit})
        CHECK(fixture.session.request_transition(transition) ==
              ai3::TransitionRequestResult::proceed);

    edit(fixture, [&] { fixture.operations.create_object(ai3::CreateObject{"Dirty"}); });
    CHECK(fixture.session.request_transition(ai3::DocumentTransition::open_document) ==
          ai3::TransitionRequestResult::needs_unsaved_resolution);
    fixture.session.cancel_pending_transition();
    CHECK(fixture.session.pending_transition() == ai3::DocumentTransition::none);
    fixture.session.request_transition(ai3::DocumentTransition::new_document);
    CHECK(fixture.session.discard_and_take_pending_transition() ==
          ai3::DocumentTransition::new_document);
    CHECK(fixture.session.dirty());
    fixture.session.request_transition(ai3::DocumentTransition::quit);
    fixture.session.save_failed();
    CHECK(fixture.session.pending_transition() == ai3::DocumentTransition::none);
    fixture.session.request_transition(ai3::DocumentTransition::open_document);
    CHECK(fixture.session.saved_and_take_pending_transition() ==
          ai3::DocumentTransition::open_document);
    CHECK_FALSE(fixture.session.dirty());
}

TEST_CASE("New document applies the Workspace transition policy")
{
    CoreFixture fixture;
    const ai3::ObjectId camera = fixture.operations.create_perspective_camera("Camera");
    const ai3::ObjectId sphere = fixture.operations.create_sphere("Sphere");
    const ai3::MaterialId material = fixture.operations.create_material("Material");
    fixture.operations.select(camera);
    REQUIRE(fixture.operations.set_bounds_display(sphere, {true, true, true}));
    fixture.workspace.set_active_material(material);
    fixture.workspace.set_display_length_unit(ai3::LengthUnit::centimeter);
    auto& viewport = fixture.workspace.viewport();
    viewport.source = ai3::ViewSource::scene_camera;
    viewport.scene_camera_id = camera;
    viewport.editor_view = {{1.0F, 2.0F, 3.0F}, 12.0F, -8.0F, 17.0F};
    viewport.interaction_mode = ai3::ViewportInteractionMode::navigation;
    viewport.reference_space = ai3::CoordinateSpace::view;
    const ai3::EditorViewState editor_view = viewport.editor_view;
    const ai3::ViewportTransformTool transform_tool = viewport.transform_tool;

    fixture.session.new_document();
    CHECK(fixture.scene.objects().empty());
    CHECK(fixture.scene.materials().empty());
    CHECK(fixture.workspace.selection() == ai3::no_object);
    CHECK(fixture.workspace.bounds_display_states().empty());
    CHECK(fixture.workspace.active_material() == ai3::no_material);
    CHECK(viewport.source == ai3::ViewSource::editor_view);
    CHECK(viewport.scene_camera_id == ai3::no_object);
    CHECK(viewport.editor_view.target == editor_view.target);
    CHECK(viewport.editor_view.yaw_degrees == editor_view.yaw_degrees);
    CHECK(viewport.editor_view.pitch_degrees == editor_view.pitch_degrees);
    CHECK(viewport.editor_view.distance == editor_view.distance);
    CHECK(fixture.workspace.display_length_unit() == ai3::LengthUnit::centimeter);
    CHECK(viewport.interaction_mode == ai3::ViewportInteractionMode::navigation);
    CHECK(viewport.transform_tool == transform_tool);
    CHECK(viewport.reference_space == ai3::CoordinateSpace::view);
    CHECK_FALSE(fixture.session.dirty());
    CHECK_FALSE(fixture.history.can_undo());
}

TEST_CASE("Open is transactional and distinguishes missing Workspace sidecars")
{
    const auto valid = temporary_scene("open");
    const auto invalid = temporary_scene("invalid");
    const auto original = temporary_scene("original");
    CoreFixture source;
    source.operations.create_sphere("Loaded");
    REQUIRE(source.session.save_as(valid).scene_saved);
    std::filesystem::remove(ai3::workspace_path_for_scene(valid));

    CoreFixture fixture;
    const ai3::ObjectId existing = fixture.operations.create_sphere("Existing");
    fixture.operations.select(existing);
    fixture.operations.set_bounds_display(existing, {true, false, true});
    fixture.workspace.set_active_material(42);
    fixture.workspace.set_display_length_unit(ai3::LengthUnit::millimeter);
    fixture.workspace.viewport().editor_view = {{8.0F, 5.0F, 3.0F}, 19.0F, -4.0F, 11.0F};
    fixture.workspace.viewport().interaction_mode = ai3::ViewportInteractionMode::navigation;
    const ai3::EditorViewState editor_view = fixture.workspace.viewport().editor_view;
    REQUIRE(fixture.session.save_as(original).scene_saved);
    edit(fixture, [&] { fixture.operations.rename_object(existing, "Dirty existing"); });
    const auto path_before = fixture.session.document_path();
    const auto history_before = fixture.history.current_state_id();
    {
        std::ofstream stream(invalid);
        stream << "invalid";
    }

    const ai3::DocumentOpenResult failed = fixture.session.open(invalid);
    CHECK_FALSE(failed.scene_opened);
    CHECK_FALSE(failed.scene_diagnostic.empty());
    CHECK(failed.workspace.status == ai3::WorkspacePersistenceStatus::not_attempted);
    CHECK(fixture.scene.find_object(existing) != nullptr);
    CHECK(fixture.workspace.selection() == existing);
    CHECK(fixture.workspace.bounds_display(existing).show_bounding_box);
    CHECK(fixture.workspace.active_material() == 42);
    CHECK(fixture.session.document_path() == path_before);
    CHECK(fixture.history.current_state_id() == history_before);
    CHECK(fixture.session.dirty());

    const ai3::DocumentOpenResult opened = fixture.session.open(valid);
    CHECK(opened.scene_opened);
    CHECK(opened.workspace.status == ai3::WorkspacePersistenceStatus::missing);
    CHECK(fixture.scene.objects().size() == 1);
    CHECK(fixture.workspace.selection() == ai3::no_object);
    CHECK(fixture.workspace.bounds_display_states().empty());
    CHECK(fixture.workspace.active_material() == ai3::no_material);
    CHECK(fixture.workspace.display_length_unit() == ai3::LengthUnit::millimeter);
    CHECK(fixture.workspace.viewport().editor_view.target == editor_view.target);
    CHECK(fixture.workspace.viewport().editor_view.yaw_degrees == editor_view.yaw_degrees);
    CHECK(fixture.workspace.viewport().editor_view.pitch_degrees == editor_view.pitch_degrees);
    CHECK(fixture.workspace.viewport().editor_view.distance == editor_view.distance);
    CHECK(fixture.workspace.viewport().interaction_mode ==
          ai3::ViewportInteractionMode::navigation);
    CHECK_FALSE(fixture.session.dirty());
    CHECK_FALSE(fixture.history.can_undo());
    std::filesystem::remove(valid);
    std::filesystem::remove(invalid);
    std::filesystem::remove(original);
    std::filesystem::remove(ai3::workspace_path_for_scene(original));
}

TEST_CASE("Open overlays valid Workspace v1 object state and drops stale identities")
{
    const auto scene_path = temporary_scene("stale");
    CoreFixture source;
    const ai3::ObjectId object = source.operations.create_sphere("Sphere");
    REQUIRE(source.session.save_as(scene_path).scene_saved);
    const ai3::WorkspaceDocument sidecar{
        {{object, {true, false, true}}, {object + 99, {false, true, true}}}};
    REQUIRE(ai3::save_workspace_file(sidecar, ai3::workspace_path_for_scene(scene_path)));

    CoreFixture fixture;
    fixture.workspace.set_bounds_display(700, {true, true, true});
    const ai3::DocumentOpenResult result = fixture.session.open(scene_path);
    REQUIRE(result.scene_opened);
    CHECK(result.workspace.status == ai3::WorkspacePersistenceStatus::succeeded);
    CHECK(fixture.workspace.bounds_display_states().size() == 1);
    CHECK(fixture.workspace.bounds_display(object).show_bounding_box);
    CHECK(fixture.workspace.bounds_display(object).hover_feedback);
    CHECK_FALSE(fixture.workspace.bounds_display(object + 99).show_bounding_sphere);
    std::filesystem::remove(scene_path);
    std::filesystem::remove(ai3::workspace_path_for_scene(scene_path));
}

TEST_CASE("Workspace failures are ancillary to authoritative Scene persistence")
{
    auto root = temporary_scene("partial-root");
    root.replace_extension();
    std::filesystem::create_directory(root);
    const auto scene_path = root / "partial.ai3scene";
    std::filesystem::create_directory(ai3::workspace_path_for_scene(scene_path));
    CoreFixture fixture;
    edit(fixture, [&] { fixture.operations.create_sphere("Sphere"); });
    const ai3::DocumentSaveResult result = fixture.session.save_as(scene_path);
    CHECK(result.scene_saved);
    CHECK(result.workspace.status == ai3::WorkspacePersistenceStatus::failed);
    CHECK_FALSE(result.workspace.diagnostic.empty());
    CHECK(std::filesystem::is_regular_file(scene_path));
    CHECK_FALSE(fixture.session.dirty());

    edit(fixture, [&] { fixture.operations.rename_object(1, "Dirty again"); });
    const auto adopted_path = fixture.session.document_path();
    const ai3::DocumentSaveResult failed_as = fixture.session.save_as(root);
    CHECK_FALSE(failed_as.scene_saved);
    CHECK(failed_as.workspace.status == ai3::WorkspacePersistenceStatus::not_attempted);
    CHECK(fixture.session.document_path() == adopted_path);
    CHECK(fixture.session.dirty());
    std::filesystem::remove_all(root);
}

TEST_CASE("Failed Workspace read remains ancillary to successful Open")
{
    const auto scene_path = temporary_scene("bad-sidecar");
    CoreFixture source;
    source.operations.create_sphere("Sphere");
    REQUIRE(source.session.save_as(scene_path).scene_saved);
    {
        std::ofstream stream(ai3::workspace_path_for_scene(scene_path), std::ios::trunc);
        stream << "invalid";
    }
    CoreFixture fixture;
    const ai3::DocumentOpenResult result = fixture.session.open(scene_path);
    CHECK(result.scene_opened);
    CHECK(result.workspace.status == ai3::WorkspacePersistenceStatus::failed);
    CHECK_FALSE(result.workspace.diagnostic.empty());
    CHECK(fixture.scene.objects().size() == 1);
    CHECK_FALSE(fixture.session.dirty());
    std::filesystem::remove(scene_path);
    std::filesystem::remove(ai3::workspace_path_for_scene(scene_path));
}
