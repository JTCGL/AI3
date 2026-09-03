#include "editor/document_session.h"
#include "editor/scene_document.h"
#include "editor/workspace_document.h"
#include "scene/helper_geometry.h"
#include "scene/viewport_view.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace
{
std::filesystem::path temporary_path(const char* name)
{
    return std::filesystem::temp_directory_path() / name;
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
    REQUIRE(scene.delete_object(id));
    CHECK(scene.bounds_workspace().empty());
    const auto another = scene.create_sphere("Sphere");
    REQUIRE(scene.set_bounds_display(another, {true, false, false}));
    REQUIRE(scene.reset_scene());
    CHECK(scene.bounds_workspace().empty());
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
    std::map<ai3::ObjectId, ai3::BoundsDisplayState> source = {{12, {true, false, true}},
                                                               {44, {false, true, false}}};
    std::string encoded;
    REQUIRE(ai3::serialize_workspace(source, encoded));
    CHECK(encoded.find("radius") == std::string::npos);
    std::map<ai3::ObjectId, ai3::BoundsDisplayState> loaded;
    REQUIRE(ai3::deserialize_workspace(encoded, loaded));
    CHECK(loaded.size() == 2);
    CHECK(loaded.at(12).show_bounding_box);
    CHECK(loaded.at(12).hover_feedback);
    REQUIRE(ai3::deserialize_workspace(
        R"({"format":"ai3-workspace","version":1,"objects":{"12":{"showBoundingBox":true}}})",
        loaded));
    CHECK(loaded.at(12).show_bounding_box);
    CHECK_FALSE(loaded.at(12).show_bounding_sphere);
    const auto unchanged = loaded;
    CHECK_FALSE(ai3::deserialize_workspace("{bad", loaded));
    CHECK(loaded.size() == unchanged.size());
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
    REQUIRE(session.save_as(first));
    REQUIRE(std::filesystem::exists(ai3::workspace_path_for_scene(first)));
    REQUIRE(session.save_as(second));
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
    ai3::ResolvedViewportView view;
    view.view = glm::mat4{1.0F};
    view.projection = glm::mat4{1.0F};
    auto hovered = ai3::resolve_helper_geometry(scene, ai3::no_object, id, {}, glm::mat3{1.0F},
                                                view, {800.0F, 600.0F}, 72.0F);
    REQUIRE_FALSE(hovered.lines.empty());
    CHECK(hovered.lines[0].color == glm::vec3{1.0F, 1.0F, 0.0F});
    auto selected = ai3::resolve_helper_geometry(scene, id, id, {}, glm::mat3{1.0F}, view,
                                                 {800.0F, 600.0F}, 72.0F);
    REQUIRE_FALSE(selected.lines.empty());
    CHECK(selected.lines[0].color == glm::vec3{1.0F});
    ai3::ViewportView viewport;
    CHECK(viewport.helper_rendering_mode() == ai3::HelperRenderingMode::overlay);
    viewport.set_helper_rendering_mode(ai3::HelperRenderingMode::depth_tested);
    CHECK(viewport.helper_rendering_mode() == ai3::HelperRenderingMode::depth_tested);
    viewport.set_interaction_mode(ai3::ViewportInteractionMode::navigation);
    CHECK(viewport.helper_rendering_mode() == ai3::HelperRenderingMode::depth_tested);
}
