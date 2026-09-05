#include "editor/document_session.h"
#include "editor/scene_document.h"
#include "editor/workspace_document.h"
#include "render/viewport_renderer.h"
#include "scene/helper_geometry.h"
#include "scene/translation_gizmo.h"
#include "scene/viewport_view.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <glm/ext/matrix_transform.hpp>

namespace
{
std::filesystem::path temporary_path(const char* name)
{
    return std::filesystem::temp_directory_path() / name;
}

void check_gizmo_apparent_length(const ai3::ResolvedViewportView& view, glm::vec2 viewport_size,
                                 glm::vec3 pivot, const glm::mat3& basis, float requested_length)
{
    ai3::EditorState scene;
    const ai3::HelperGeometry geometry = ai3::resolve_translation_helper_geometry(
        99, pivot, basis, view, viewport_size, requested_length);
    REQUIRE_FALSE(geometry.lines.empty());
    for (const ai3::ColoredLine& line : geometry.lines)
    {
        const auto start = ai3::project_world_to_viewport(line.start, view, viewport_size);
        const auto end = ai3::project_world_to_viewport(line.end, view, viewport_size);
        REQUIRE(start);
        REQUIRE(end);
        CHECK(glm::length(*end - *start) == doctest::Approx(requested_length).epsilon(0.002));
    }
}
} // namespace

TEST_CASE("sphere bounds are cached locally and only radius changes their values")
{
    ai3::EditorState scene;
    const ai3::ObjectId parent = scene.create_directional_light("Light");
    const ai3::ObjectId id = scene.create_sphere("Sphere", {2.5F});
    const ai3::SceneObject* sphere = scene.find_object(id);
    REQUIRE(sphere->bounds.box);
    REQUIRE(sphere->bounds.sphere);
    CHECK(sphere->bounds.box->minimum == glm::vec3{-2.5F});
    CHECK(sphere->bounds.box->maximum == glm::vec3{2.5F});
    CHECK(sphere->bounds.sphere->center == glm::vec3{0.0F});
    CHECK(sphere->bounds.sphere->radius == doctest::Approx(2.5F));
    CHECK_FALSE(scene.find_object(parent)->bounds.box);
    const ai3::ObjectBounds before = sphere->bounds;
    const auto revision = scene.document_revision();
    REQUIRE(scene.set_sphere(id, sphere->sphere));
    CHECK(scene.document_revision() == revision);
    ai3::Transform transform;
    transform.position = {4.0F, 2.0F, -1.0F};
    transform.scale = {-2.0F, 3.0F, 0.5F};
    REQUIRE(scene.set_local_transform(id, transform));
    REQUIRE(scene.reparent_object(id, parent));
    REQUIRE(scene.rename_object(id, "Renamed"));
    REQUIRE(scene.set_object_enabled(id, false));
    REQUIRE(scene.set_object_visible(id, false));
    const auto material = scene.create_material("Material");
    REQUIRE(scene.assign_material(id, material));
    REQUIRE(scene.set_sphere_fallback_color(id, {0.1F, 0.2F, 0.3F}));
    REQUIRE(scene.select(id));
    CHECK(scene.find_object(id)->bounds.box->minimum == before.box->minimum);
    CHECK(scene.find_object(id)->bounds.sphere->radius == before.sphere->radius);
    ai3::SpherePrimitive changed = scene.find_object(id)->sphere;
    changed.radius_meters = 4.0F;
    REQUIRE(scene.set_sphere(id, changed));
    CHECK(scene.find_object(id)->bounds.box->minimum == glm::vec3{-4.0F});
    CHECK(scene.find_object(id)->bounds.sphere->radius == doctest::Approx(4.0F));
    CHECK_THROWS_AS(scene.create_sphere("Bad", {-1.0F}), std::invalid_argument);
}

TEST_CASE("bounds and workspace follow undo deletion and reset lifecycles")
{
    ai3::EditorState scene;
    ai3::DocumentSession session(scene);
    REQUIRE(session.history().begin_transaction());
    const auto id = scene.create_sphere("Sphere", {1.0F});
    REQUIRE(session.history().commit_transaction());
    REQUIRE(session.history().begin_transaction());
    ai3::SpherePrimitive changed = scene.find_object(id)->sphere;
    changed.radius_meters = 6.0F;
    REQUIRE(scene.set_sphere(id, changed));
    REQUIRE(session.history().commit_transaction());
    REQUIRE(session.history().undo());
    CHECK(scene.find_object(id)->bounds.sphere->radius == doctest::Approx(1.0F));
    REQUIRE(session.history().redo());
    CHECK(scene.find_object(id)->bounds.sphere->radius == doctest::Approx(6.0F));
    REQUIRE(scene.set_bounds_display(id, {true, true, true}));
    REQUIRE(session.history().begin_transaction());
    REQUIRE(scene.delete_object(id));
    REQUIRE(session.history().commit_transaction());
    CHECK(scene.bounds_workspace().empty());
    REQUIRE(session.history().undo());
    const ai3::SceneObject* restored = scene.find_object(id);
    REQUIRE(restored != nullptr);
    CHECK(restored->sphere.radius_meters == doctest::Approx(6.0F));
    REQUIRE(restored->bounds.box);
    REQUIRE(restored->bounds.sphere);
    CHECK(restored->bounds.box->minimum == glm::vec3{-6.0F});
    CHECK(restored->bounds.sphere->radius == doctest::Approx(6.0F));
    CHECK(scene.bounds_display(id).show_bounding_box);
    CHECK(scene.bounds_display(id).show_bounding_sphere);
    CHECK(scene.bounds_display(id).hover_feedback);
    REQUIRE(session.history().redo());
    CHECK(scene.find_object(id) == nullptr);
    CHECK(scene.bounds_workspace().empty());
    REQUIRE(session.history().undo());
    CHECK(scene.bounds_display(id).hover_feedback);
    REQUIRE(session.history().begin_transaction());
    REQUIRE(scene.rename_object(id, "Branched sphere"));
    REQUIRE(session.history().commit_transaction());
    CHECK_FALSE(session.history().can_redo());
    CHECK(scene.bounds_display(id).show_bounding_box);
    REQUIRE(session.history().begin_transaction());
    REQUIRE(scene.reset_scene());
    REQUIRE(session.history().commit_transaction());
    CHECK(scene.bounds_workspace().empty());
    REQUIRE(session.history().undo());
    REQUIRE(scene.find_object(id) != nullptr);
    CHECK(scene.bounds_workspace().empty());
    session.history().rebaseline();
    CHECK_FALSE(session.history().can_undo());
    CHECK_FALSE(session.history().can_redo());
}

TEST_CASE("bounds reconstruct on document load and never serialize")
{
    ai3::EditorState source;
    source.create_sphere("Sphere", {3.0F});
    std::string document;
    REQUIRE(ai3::serialize_scene_document(source, document));
    CHECK(document.find("bounds") == std::string::npos);
    CHECK(document.find("showBounding") == std::string::npos);
    for (int version : {1, 2})
    {
        auto json = nlohmann::json::parse(document);
        json["version"] = version;
        json["metadata"]["default_name_counters"].erase("box");
        if (version == 1)
        {
            json.erase("materials");
            json["metadata"].erase("next_material_id");
            json["metadata"].erase("default_material_name_count");
            json["objects"][0]["payload"].erase("material_id");
            json["objects"][0]["payload"].erase("fallback_color_linear");
        }
        ai3::EditorState loaded;
        REQUIRE(ai3::deserialize_scene_document(json.dump(), loaded));
        REQUIRE(loaded.objects()[0].bounds.sphere);
        CHECK(loaded.objects()[0].bounds.sphere->radius == doctest::Approx(3.0F));
    }
}

TEST_CASE("workspace v1 is transactional defaulting and exact by object ID")
{
    ai3::WorkspaceDocument source;
    source.objects = {{12, {true, false, true}}, {44, {false, true, false}}};
    std::string encoded;
    REQUIRE(ai3::serialize_workspace(source, encoded));
    CHECK(encoded.find("radius") == std::string::npos);
    CHECK(encoded.find("helperRenderingMode") == std::string::npos);
    ai3::WorkspaceDocument loaded;
    REQUIRE(ai3::deserialize_workspace(encoded, loaded));
    CHECK(loaded.objects.size() == 2);
    CHECK(loaded.objects.at(12).show_bounding_box);
    CHECK(loaded.objects.at(12).hover_feedback);
    REQUIRE(ai3::deserialize_workspace(
        R"({"format":"ai3-workspace","version":1,"objects":{"12":{"showBoundingBox":true}}})",
        loaded));
    CHECK(loaded.objects.at(12).show_bounding_box);
    CHECK_FALSE(loaded.objects.at(12).show_bounding_sphere);
    for (const char* legacy_mode : {"overlay", "depth-tested", "obsolete-value"})
    {
        const std::string legacy =
            std::string{R"({"format":"ai3-workspace","version":1,"helperRenderingMode":")"} +
            legacy_mode + R"(","objects":{"12":{"hoverFeedback":true}}})";
        REQUIRE(ai3::deserialize_workspace(legacy, loaded));
        CHECK(loaded.objects.at(12).hover_feedback);
    }
    const auto unchanged = loaded;
    CHECK_FALSE(ai3::deserialize_workspace("{bad", loaded));
    CHECK(loaded.objects.size() == unchanged.objects.size());
    CHECK_FALSE(ai3::deserialize_workspace(
        R"({"format":"ai3-workspace","version":1,"helperRenderingMode":4,"objects":{}})", loaded));
    CHECK_FALSE(
        ai3::deserialize_workspace(R"({"format":"wrong","version":1,"objects":{}})", loaded));
    CHECK_FALSE(ai3::deserialize_workspace(R"({"format":"ai3-workspace","version":2,"objects":{}})",
                                           loaded));
}

TEST_CASE("workspace sidecars follow document session lifecycle without dirtying scene")
{
    const auto first = temporary_path("ai3-m17-first.ai3scene");
    const auto second = temporary_path("ai3-m17-second.ai3scene");
    std::error_code ignored;
    std::filesystem::remove(first, ignored);
    std::filesystem::remove(second, ignored);
    std::filesystem::remove(ai3::workspace_path_for_scene(first), ignored);
    std::filesystem::remove(ai3::workspace_path_for_scene(second), ignored);
    CHECK(ai3::workspace_path_for_scene(first).filename() == "ai3-m17-first.ai3workspace");
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    const auto id = state.create_sphere("Sphere");
    session.history().rebaseline();
    session.mark_saved();
    const auto revision = state.document_revision();
    REQUIRE(session.set_bounds_display(id, {true, true, true}));
    CHECK(state.document_revision() == revision);
    CHECK_FALSE(session.dirty());
    REQUIRE(session.save_as(first).scene_saved);
    REQUIRE(std::filesystem::exists(ai3::workspace_path_for_scene(first)));
    REQUIRE(session.save_as(second).scene_saved);
    REQUIRE(std::filesystem::exists(ai3::workspace_path_for_scene(second)));
    state.replace_bounds_workspace({});
    REQUIRE(session.open(second));
    CHECK(state.bounds_display(id).show_bounding_sphere);
    session.new_document();
    CHECK(state.bounds_workspace().empty());
    std::filesystem::remove(first, ignored);
    std::filesystem::remove(second, ignored);
    std::filesystem::remove(ai3::workspace_path_for_scene(first), ignored);
    std::filesystem::remove(ai3::workspace_path_for_scene(second), ignored);
}

TEST_CASE("workspace mutation waits for Save and missing sidecars use object defaults")
{
    const auto scene_path = temporary_path("ai3-m17-explicit-save.ai3scene");
    const auto workspace_path = ai3::workspace_path_for_scene(scene_path);
    std::error_code ignored;
    std::filesystem::remove(scene_path, ignored);
    std::filesystem::remove(workspace_path, ignored);
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    const auto id = state.create_sphere("Sphere");
    REQUIRE(session.save_as(scene_path).scene_saved);
    std::filesystem::remove(workspace_path, ignored);
    REQUIRE(session.set_bounds_display(id, {true, false, true}));
    CHECK_FALSE(std::filesystem::exists(workspace_path));
    CHECK_FALSE(session.dirty());
    const ai3::DocumentSaveResult saved = session.save();
    CHECK(saved.scene_saved);
    CHECK(saved.workspace_saved);
    REQUIRE(session.open(scene_path));
    CHECK(state.bounds_display(id).show_bounding_box);
    std::filesystem::remove(workspace_path, ignored);
    REQUIRE(session.open(scene_path));
    CHECK_FALSE(state.bounds_display(id).show_bounding_box);
    CHECK_FALSE(state.bounds_display(id).show_bounding_sphere);
    CHECK_FALSE(state.bounds_display(id).hover_feedback);
    REQUIRE(session.reset_scene());
    session.new_document();
    std::filesystem::remove(scene_path, ignored);
    std::filesystem::remove(workspace_path, ignored);
}

TEST_CASE("scene save success remains distinct from workspace failure")
{
    const auto unique = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto root = std::filesystem::temp_directory_path() / ("ai3-m17-partial-" + unique);
    std::filesystem::create_directory(root);
    const auto scene_path = root / "partial.ai3scene";
    const auto workspace_path = ai3::workspace_path_for_scene(scene_path);
    std::filesystem::create_directory(workspace_path);
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    session.history().begin_transaction();
    state.create_sphere("Sphere");
    session.history().commit_transaction();
    REQUIRE(session.request_transition(ai3::DocumentTransition::quit) ==
            ai3::TransitionRequestResult::needs_unsaved_resolution);
    const std::size_t messages_before = state.console_messages().size();
    std::string scene_error;
    std::string workspace_error;
    const ai3::DocumentSaveResult partial =
        session.save_as(scene_path, &scene_error, &workspace_error);
    CHECK(partial.scene_saved);
    CHECK_FALSE(partial.workspace_saved);
    CHECK(scene_error.empty());
    CHECK_FALSE(workspace_error.empty());
    CHECK(std::filesystem::is_regular_file(scene_path));
    CHECK(session.document_path() == scene_path);
    CHECK_FALSE(session.dirty());
    CHECK(state.console_messages().size() == messages_before);
    CHECK(session.saved_and_take_pending_transition() == ai3::DocumentTransition::quit);

    session.history().begin_transaction();
    state.rename_object(1, "Dirty again");
    session.history().commit_transaction();
    const auto old_path = session.document_path();
    const ai3::DocumentSaveResult failed = session.save_as(root, &scene_error, &workspace_error);
    CHECK_FALSE(failed.scene_saved);
    CHECK_FALSE(failed.workspace_saved);
    CHECK(session.dirty());
    CHECK(session.document_path() == old_path);
    std::filesystem::remove_all(root);
}

TEST_CASE("world gizmo geometry projects to the requested apparent size")
{
    ai3::EditorState scene;
    ai3::ViewportView orbit;
    const glm::mat3 rotated = glm::mat3{glm::rotate(glm::mat4{1.0F}, glm::radians(31.0F),
                                                    glm::normalize(glm::vec3{1.0F, 2.0F, 3.0F}))};
    for (float zoom : {0.0F, 4.0F, -4.0F})
    {
        orbit.orbit().reset();
        orbit.orbit().zoom(zoom);
        for (const glm::vec2 viewport : {glm::vec2{800.0F, 600.0F}, glm::vec2{1440.0F, 600.0F}})
            for (float length : {72.0F, 108.0F, 144.0F})
                check_gizmo_apparent_length(orbit.resolve(scene, viewport.x / viewport.y), viewport,
                                            {}, rotated, length);
    }

    const auto camera = scene.create_perspective_camera("Camera", {60.0F, 0.1F, 200.0F});
    ai3::ViewportView scene_camera;
    REQUIRE(scene_camera.use_scene_camera(scene, camera));
    for (float depth : {-3.0F, -12.0F, -40.0F})
        check_gizmo_apparent_length(scene_camera.resolve(scene, 16.0F / 9.0F), {1280.0F, 720.0F},
                                    {0.0F, 0.0F, depth}, rotated, 108.0F);

    glm::mat3 collapsed{1.0F};
    collapsed[1] = glm::vec3{0.0F};
    const auto collapsed_geometry = ai3::resolve_translation_helper_geometry(
        99, {0.0F, 0.0F, -5.0F}, collapsed, scene_camera.resolve(scene, 1.0F), {800.0F, 800.0F},
        72.0F);
    CHECK(collapsed_geometry.lines.size() == 1);
}

TEST_CASE("helper bounds are deterministic and apply the complete world transform")
{
    ai3::EditorState scene;
    ai3::CreateObject parent{"Parent"};
    parent.transform.position = {2.0F, 3.0F, 4.0F};
    parent.transform.scale = {-2.0F, 1.5F, 0.5F};
    const auto parent_id = scene.create_object(parent);
    ai3::CreateObject sphere{"Sphere", parent_id};
    sphere.category = ai3::ObjectCategory::primitive;
    sphere.primitive_kind = ai3::PrimitiveKind::sphere;
    sphere.sphere.radius_meters = 1.0F;
    const auto id = scene.create_object(sphere);
    REQUIRE(scene.set_bounds_display(id, {true, true, true}));
    ai3::HelperGeometry geometry;
    ai3::append_object_bounds(geometry, scene, *scene.find_object(id), glm::vec3{1.0F});
    CHECK(geometry.lines.size() == 12 + 3 * 48);
    CHECK(geometry.lines[0].start == glm::vec3{4.0F, 1.5F, 3.5F});
    auto hovered = ai3::resolve_bounds_helper_geometry(scene, ai3::no_object, id);
    REQUIRE_FALSE(hovered.lines.empty());
    CHECK(hovered.lines[0].color == glm::vec3{1.0F, 1.0F, 0.0F});
    auto selected = ai3::resolve_bounds_helper_geometry(scene, id, id);
    REQUIRE_FALSE(selected.lines.empty());
    CHECK(selected.lines[0].color == glm::vec3{1.0F});
    ai3::ViewportView viewport;
    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK(viewport.helper_hover_object(id) == ai3::no_object);
    const auto navigation_bounds = ai3::resolve_bounds_helper_geometry(
        scene, ai3::no_object, viewport.helper_hover_object(id));
    CHECK(navigation_bounds.lines.empty());
    viewport.set_interaction_mode(ai3::ViewportInteractionMode::selection);
    CHECK(viewport.helper_hover_object(id) == id);
    const auto selection_bounds = ai3::resolve_bounds_helper_geometry(
        scene, ai3::no_object, viewport.helper_hover_object(id));
    CHECK_FALSE(selection_bounds.lines.empty());
}

TEST_CASE("GLES helper batches have explicit distinct depth policies")
{
    constexpr ai3::HelperDepthState scene = ai3::viewport_scene_depth_state;
    constexpr ai3::HelperDepthState bounds = ai3::helper_depth_state(ai3::HelperRenderRole::bounds);
    constexpr ai3::HelperDepthState gizmo = ai3::helper_depth_state(ai3::HelperRenderRole::gizmo);
    constexpr ai3::HelperDepthState restored = ai3::restored_helper_depth_state;
    CHECK(scene.depth_test_enabled);
    CHECK(scene.depth_write_enabled);
    CHECK(bounds.depth_test_enabled);
    CHECK_FALSE(bounds.depth_write_enabled);
    CHECK(bounds.depth_bias_enabled);
    CHECK_FALSE(gizmo.depth_test_enabled);
    CHECK_FALSE(gizmo.depth_write_enabled);
    CHECK_FALSE(gizmo.depth_bias_enabled);
    CHECK(restored.depth_test_enabled == scene.depth_test_enabled);
    CHECK(restored.depth_write_enabled == scene.depth_write_enabled);
    CHECK_FALSE(restored.depth_bias_enabled);

    ai3::HelperGeometry bounds_geometry;
    ai3::HelperGeometry gizmo_geometry;
    const ai3::ViewportHelperInputs submitted{&bounds_geometry, &gizmo_geometry, nullptr};
    CHECK(submitted.bounds == &bounds_geometry);
    CHECK(submitted.gizmo == &gizmo_geometry);
    CHECK(submitted.bounds != submitted.gizmo);
}

TEST_CASE("frozen gizmo inputs remain separate from current bounds and camera view")
{
    ai3::EditorState scene;
    const auto camera = scene.create_perspective_camera("Camera", {55.0F, 0.1F, 200.0F});
    ai3::ViewportView viewport;
    REQUIRE(viewport.use_scene_camera(scene, camera));
    const glm::vec2 frozen_size{960.0F, 540.0F};
    const ai3::ResolvedViewportView frozen_view = viewport.resolve(scene, 16.0F / 9.0F);
    const glm::mat3 frozen_basis = glm::mat3{glm::rotate(
        glm::mat4{1.0F}, glm::radians(27.0F), glm::normalize(glm::vec3{1.0F, 2.0F, 1.0F}))};

    ai3::Transform moved_camera = scene.find_object(camera)->transform;
    moved_camera.position = {1.0F, 0.0F, -5.0F};
    REQUIRE(scene.set_local_transform(camera, moved_camera));
    const ai3::ResolvedViewportView current_view = viewport.resolve(scene, 16.0F / 9.0F);
    CHECK(current_view.view != frozen_view.view);

    const auto gizmo = ai3::resolve_translation_helper_geometry(
        camera, scene.world_position(camera), frozen_basis, frozen_view, frozen_size, 108.0F, 1);
    REQUIRE_FALSE(gizmo.lines.empty());
    for (const ai3::ColoredLine& line : gizmo.lines)
    {
        const auto start = ai3::project_world_to_viewport(line.start, frozen_view, frozen_size);
        const auto end = ai3::project_world_to_viewport(line.end, frozen_view, frozen_size);
        REQUIRE(start);
        REQUIRE(end);
        CHECK(glm::length(*end - *start) == doctest::Approx(108.0F).epsilon(0.002));
    }

    const auto sphere = scene.create_sphere("Sphere");
    REQUIRE(scene.set_bounds_display(sphere, {true, false, false}));
    ai3::Transform moved_sphere = scene.find_object(sphere)->transform;
    moved_sphere.position = {7.0F, 8.0F, 9.0F};
    REQUIRE(scene.set_local_transform(sphere, moved_sphere));
    const auto bounds = ai3::resolve_bounds_helper_geometry(scene, sphere, ai3::no_object);
    REQUIRE_FALSE(bounds.lines.empty());
    CHECK(bounds.lines.front().start == glm::vec3{6.0F, 7.0F, 8.0F});
}
