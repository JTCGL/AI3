#include <doctest/doctest.h>

#include "editor/scene_document.h"
#include "scene/scene_math.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>

namespace
{
using Json = nlohmann::json;

Json encoded(const ai3::EditorState& state)
{
    std::string text;
    REQUIRE(ai3::serialize_scene_document(state, text));
    return Json::parse(text);
}

bool load_json(const Json& document, ai3::EditorState& state)
{
    std::string error;
    return ai3::deserialize_scene_document(document.dump(), state, &error);
}

void check_vec3(const glm::vec3& actual, const glm::vec3& expected)
{
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void check_same_scene(const ai3::EditorState& actual, const ai3::EditorState& expected)
{
    REQUIRE(actual.objects().size() == expected.objects().size());
    for (std::size_t index = 0; index < expected.objects().size(); ++index)
    {
        const ai3::SceneObject& left = actual.objects()[index];
        const ai3::SceneObject& right = expected.objects()[index];
        CHECK(left.id == right.id);
        CHECK(left.name == right.name);
        CHECK(left.enabled == right.enabled);
        CHECK(left.visible == right.visible);
        CHECK(left.parent_id() == right.parent_id());
        check_vec3(left.transform.position, right.transform.position);
        check_vec3(left.transform.scale, right.transform.scale);
        CHECK(left.transform.orientation.w == doctest::Approx(right.transform.orientation.w));
        CHECK(left.transform.orientation.x == doctest::Approx(right.transform.orientation.x));
        CHECK(left.transform.orientation.y == doctest::Approx(right.transform.orientation.y));
        CHECK(left.transform.orientation.z == doctest::Approx(right.transform.orientation.z));
        CHECK(left.category == right.category);
        CHECK(left.primitive_kind == right.primitive_kind);
        CHECK(left.camera_kind == right.camera_kind);
        CHECK(left.light_kind == right.light_kind);
        CHECK(left.sphere.radius_meters == doctest::Approx(right.sphere.radius_meters));
        CHECK(left.perspective_camera.vertical_fov_degrees ==
              doctest::Approx(right.perspective_camera.vertical_fov_degrees));
        CHECK(left.perspective_camera.near_plane_meters ==
              doctest::Approx(right.perspective_camera.near_plane_meters));
        CHECK(left.perspective_camera.far_plane_meters ==
              doctest::Approx(right.perspective_camera.far_plane_meters));
        check_vec3(left.directional_light.color, right.directional_light.color);
        CHECK(left.directional_light.intensity ==
              doctest::Approx(right.directional_light.intensity));
    }
}

ai3::EditorState mixed_scene()
{
    ai3::EditorState scene;
    ai3::CreateObject root{"Raíz 世界"};
    root.enabled = false;
    root.transform.position = {1.25F, -2.5F, 3.75F};
    root.transform.orientation = ai3::orientation_from_euler_degrees({20.0F, -30.0F, 40.0F});
    root.transform.scale = {-1.0F, 2.0F, 0.5F};
    const ai3::ObjectId root_id = scene.create_object(root);

    ai3::CreateObject sphere{"Esfera ñ", root_id};
    sphere.visible = false;
    sphere.category = ai3::ObjectCategory::primitive;
    sphere.primitive_kind = ai3::PrimitiveKind::sphere;
    sphere.sphere.radius_meters = 2.25F;
    sphere.transform.position = {-4.0F, 5.0F, 6.0F};
    sphere.transform.scale = {1.0F, -3.0F, 2.0F};
    scene.create_object(sphere);

    ai3::CreateObject camera{"Cámara", root_id};
    camera.category = ai3::ObjectCategory::camera;
    camera.camera_kind = ai3::CameraKind::perspective;
    camera.perspective_camera = {72.0F, 0.25F, 850.0F};
    scene.create_object(camera);

    ai3::CreateObject light{"Luz"};
    light.category = ai3::ObjectCategory::light;
    light.light_kind = ai3::LightKind::directional;
    light.directional_light = {{0.2F, 0.4F, 0.8F}, 3.5F};
    scene.create_object(light);
    return scene;
}
} // namespace

TEST_CASE("empty Scene Document round trips")
{
    ai3::EditorState source;
    ai3::EditorState loaded;
    loaded.create_sphere("Old");
    std::string document;
    REQUIRE(ai3::serialize_scene_document(source, document));
    REQUIRE(ai3::deserialize_scene_document(document, loaded));
    CHECK(loaded.objects().empty());
    CHECK(loaded.create_object(ai3::CreateObject{"New"}) == 1);
}

TEST_CASE("transactional load advances revision only when document state changes")
{
    ai3::EditorState source;
    source.create_sphere("Sphere");
    std::string document;
    REQUIRE(ai3::serialize_scene_document(source, document));

    ai3::EditorState destination;
    const auto initial = destination.document_revision();
    REQUIRE(ai3::deserialize_scene_document(document, destination));
    CHECK(destination.document_revision() == initial + 1);
    const auto loaded = destination.document_revision();
    REQUIRE(ai3::deserialize_scene_document(document, destination));
    CHECK(destination.document_revision() == loaded);
    CHECK_FALSE(ai3::deserialize_scene_document("invalid", destination));
    CHECK(destination.document_revision() == loaded);
}

TEST_CASE("mixed Scene Document preserves ordering identity hierarchy local state and semantics")
{
    ai3::EditorState source = mixed_scene();
    REQUIRE(source.select(source.objects()[2].id));
    std::string document;
    REQUIRE(ai3::serialize_scene_document(source, document));
    CHECK(document.find("orientation_wxyz") != std::string::npos);
    CHECK(document.find("Raíz 世界") != std::string::npos);

    ai3::EditorState loaded;
    REQUIRE(ai3::deserialize_scene_document(document, loaded));
    check_same_scene(loaded, source);
    CHECK(loaded.selection() == ai3::no_object);
    CHECK(loaded.create_object(ai3::CreateObject{"After"}) == 5);
}

TEST_CASE("allocator and localized default-name counters continue after deleted objects")
{
    ai3::EditorState source;
    source.create_sphere("Sphere");
    const ai3::ObjectId deleted_sphere = source.create_sphere("Sphere");
    source.create_perspective_camera("Camera");
    const ai3::ObjectId deleted_camera = source.create_perspective_camera("Camera");
    source.create_directional_light("Light");
    const ai3::ObjectId deleted_light = source.create_directional_light("Light");
    REQUIRE(source.delete_object(deleted_sphere));
    REQUIRE(source.delete_object(deleted_camera));
    REQUIRE(source.delete_object(deleted_light));

    ai3::EditorState loaded;
    REQUIRE(load_json(encoded(source), loaded));
    const ai3::ObjectId sphere = loaded.create_sphere("Esfera");
    const ai3::ObjectId camera = loaded.create_perspective_camera("Cámara");
    const ai3::ObjectId light = loaded.create_directional_light("Luz");
    CHECK(sphere == 7);
    CHECK(loaded.find_object(sphere)->name == "Esfera 3");
    CHECK(loaded.find_object(camera)->name == "Cámara 3");
    CHECK(loaded.find_object(light)->name == "Luz 3");
}

TEST_CASE("malformed format version identity hierarchy and metadata are rejected transactionally")
{
    ai3::EditorState source = mixed_scene();
    const Json valid = encoded(source);
    std::vector<std::string> malformed = {"", "{", "[]", "null", "not json"};
    for (const std::string& text : malformed)
    {
        ai3::EditorState destination;
        const ai3::ObjectId existing = destination.create_sphere("Existing");
        REQUIRE(destination.select(existing));
        CHECK_FALSE(ai3::deserialize_scene_document(text, destination));
        CHECK(destination.objects().size() == 1);
        CHECK(destination.objects()[0].id == existing);
        CHECK(destination.selection() == existing);
    }

    std::vector<Json> invalid_documents;
    Json wrong_format = valid;
    wrong_format["format"] = "other";
    invalid_documents.push_back(wrong_format);
    Json wrong_version = valid;
    wrong_version["version"] = 4;
    invalid_documents.push_back(wrong_version);
    Json duplicate = valid;
    duplicate["objects"][1]["id"] = duplicate["objects"][0]["id"];
    invalid_documents.push_back(duplicate);
    Json zero_id = valid;
    zero_id["objects"][0]["id"] = 0;
    invalid_documents.push_back(zero_id);
    Json missing_parent = valid;
    missing_parent["objects"][0]["parent_id"] = 999;
    invalid_documents.push_back(missing_parent);
    Json self_parent = valid;
    self_parent["objects"][0]["parent_id"] = self_parent["objects"][0]["id"];
    invalid_documents.push_back(self_parent);
    Json cycle = valid;
    cycle["objects"][0]["parent_id"] = cycle["objects"][1]["id"];
    invalid_documents.push_back(cycle);
    Json allocator = valid;
    allocator["metadata"]["next_object_id"] = 4;
    invalid_documents.push_back(allocator);
    Json exhausted_allocator = valid;
    exhausted_allocator["metadata"]["next_object_id"] = std::numeric_limits<std::uint64_t>::max();
    invalid_documents.push_back(exhausted_allocator);
    Json counter = valid;
    counter["metadata"]["default_name_counters"]["sphere"] = 5;
    invalid_documents.push_back(counter);

    for (const Json& candidate : invalid_documents)
    {
        ai3::EditorState destination;
        const ai3::ObjectId existing = destination.create_sphere("Existing");
        REQUIRE(destination.select(existing));
        CHECK_FALSE(load_json(candidate, destination));
        CHECK(destination.objects().size() == 1);
        CHECK(destination.selection() == existing);
    }
}

TEST_CASE("Scene Document v3 preserves Box semantics and rejects legacy Box payloads")
{
    ai3::EditorState source;
    const auto material = source.create_material("Material");
    ai3::BoxPrimitive box;
    box.width_meters = 2.0F;
    box.length_meters = 3.0F;
    box.height_meters = 4.0F;
    box.width_segments = 2;
    box.length_segments = 3;
    box.height_segments = 4;
    box.material_id = material;
    box.fallback_color = {0.1F, 0.2F, 0.3F};
    const auto id = source.create_box("Box", box);
    std::string text;
    REQUIRE(ai3::serialize_scene_document(source, text));
    const Json document = Json::parse(text);
    CHECK(document.at("version") == 3);
    CHECK(document.at("objects")[0].at("subtype") == "box");
    ai3::EditorState loaded;
    REQUIRE(ai3::deserialize_scene_document(text, loaded));
    const auto* restored = loaded.find_object(id);
    REQUIRE(restored != nullptr);
    CHECK(restored->primitive_kind == ai3::PrimitiveKind::box);
    CHECK(restored->box.width_meters == 2.0F);
    CHECK(restored->box.length_meters == 3.0F);
    CHECK(restored->box.height_meters == 4.0F);
    CHECK(restored->box.width_segments == 2);
    CHECK(restored->box.length_segments == 3);
    CHECK(restored->box.height_segments == 4);
    CHECK(restored->box.material_id == material);
    CHECK(restored->box.fallback_color == glm::vec3{0.1F, 0.2F, 0.3F});
    for (int version : {1, 2})
    {
        Json legacy = document;
        legacy["version"] = version;
        legacy["metadata"]["default_name_counters"].erase("box");
        if (version == 1)
        {
            legacy.erase("materials");
            legacy["metadata"].erase("next_material_id");
            legacy["metadata"].erase("default_material_name_count");
        }
        ai3::EditorState destination;
        CHECK_FALSE(ai3::deserialize_scene_document(legacy.dump(), destination));
        legacy["metadata"]["default_name_counters"]["box"] = 0;
        CHECK_FALSE(ai3::deserialize_scene_document(legacy.dump(), destination));
    }
}

TEST_CASE("invalid types transforms quaternions and semantic payloads are rejected")
{
    const Json valid = encoded(mixed_scene());
    std::vector<Json> invalid_documents;
    Json subtype = valid;
    subtype["objects"][0]["subtype"] = "sphere";
    invalid_documents.push_back(subtype);
    Json non_finite = valid;
    non_finite["objects"][0]["transform"]["position_meters"][0] =
        std::numeric_limits<double>::infinity();
    invalid_documents.push_back(non_finite);
    Json zero_quaternion = valid;
    zero_quaternion["objects"][0]["transform"]["orientation_wxyz"] = {0, 0, 0, 0};
    invalid_documents.push_back(zero_quaternion);
    Json non_normalized = valid;
    non_normalized["objects"][0]["transform"]["orientation_wxyz"] = {2, 0, 0, 0};
    invalid_documents.push_back(non_normalized);
    Json sphere = valid;
    sphere["objects"][1]["payload"]["radius_meters"] = 0;
    invalid_documents.push_back(sphere);
    Json camera = valid;
    camera["objects"][2]["payload"]["far_plane_meters"] = 0.1;
    invalid_documents.push_back(camera);
    Json light = valid;
    light["objects"][3]["payload"]["intensity"] = -1;
    invalid_documents.push_back(light);
    Json light_color = valid;
    light_color["objects"][3]["payload"]["color"][0] = std::numeric_limits<double>::infinity();
    invalid_documents.push_back(light_color);

    for (const Json& candidate : invalid_documents)
    {
        ai3::EditorState destination;
        destination.create_object(ai3::CreateObject{"Keep"});
        CHECK_FALSE(load_json(candidate, destination));
        CHECK(destination.objects()[0].name == "Keep");
    }
}

TEST_CASE("Scene Document filesystem round trip uses ordinary temporary files")
{
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("ai3-scene-document-" + std::to_string(unique) + ".ai3scene");
    const ai3::EditorState source = mixed_scene();
    ai3::EditorState loaded;
    std::string error;
    REQUIRE(ai3::save_scene_document_file(source, path, &error));
    REQUIRE(ai3::load_scene_document_file(path, loaded, &error));
    check_same_scene(loaded, source);
    std::filesystem::remove(path);
}
