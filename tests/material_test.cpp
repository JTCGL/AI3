#include <doctest/doctest.h>

#include "editor/document_session.h"
#include "editor/editor_history.h"
#include "editor/scene_document.h"
#include "scene/color_space.h"

#include <limits>
#include <nlohmann/json.hpp>
#include <vector>

namespace
{
nlohmann::json encoded(const ai3::EditorState& state)
{
    std::string document;
    REQUIRE(ai3::serialize_scene_document(state, document));
    return nlohmann::json::parse(document);
}

bool load(const nlohmann::json& document, ai3::EditorState& state)
{
    return ai3::deserialize_scene_document(document.dump(), state);
}
} // namespace

TEST_CASE("sRGB transfer functions use the standard piecewise curve")
{
    CHECK(ai3::srgb_to_linear(0.0F) == doctest::Approx(0.0F));
    CHECK(ai3::srgb_to_linear(1.0F) == doctest::Approx(1.0F));
    CHECK(ai3::srgb_to_linear(0.04045F) == doctest::Approx(0.0031308F).epsilon(0.0001));
    CHECK(ai3::srgb_to_linear(0.5F) == doctest::Approx(0.214041F).epsilon(0.0001));
    CHECK(ai3::linear_to_srgb(0.214041F) == doctest::Approx(0.5F).epsilon(0.0001));
    for (float value : {0.0F, 0.01F, 0.18F, 0.5F, 1.0F})
        CHECK(ai3::linear_to_srgb(ai3::srgb_to_linear(value)) ==
              doctest::Approx(value).epsilon(0.0001));
    CHECK_THROWS_AS(ai3::srgb_to_linear(-0.1F), std::invalid_argument);
    CHECK_THROWS_AS(ai3::linear_to_srgb(1.1F), std::invalid_argument);
}

TEST_CASE("materials have independent monotonic identity and valid sphere assignment")
{
    ai3::EditorState state;
    const ai3::MaterialId first = state.create_material("Material");
    const ai3::ObjectId one = state.create_sphere("Sphere");
    const ai3::ObjectId two = state.create_sphere("Sphere");
    const ai3::ObjectId camera = state.create_perspective_camera("Camera");
    CHECK(first == 1);
    CHECK(state.find_material(first)->name == "Material 1");
    REQUIRE(state.assign_material(one, first));
    REQUIRE(state.assign_material(two, first));
    CHECK_FALSE(state.assign_material(camera, first));
    CHECK_FALSE(state.assign_material(one, 999));
    const glm::vec3 fallback = state.find_object(one)->sphere.fallback_color;
    REQUIRE(state.set_sphere_fallback_color(one, {0.1F, 0.2F, 0.3F}));
    CHECK(state.find_object(one)->sphere.material_id == first);
    CHECK(state.find_object(one)->sphere.fallback_color != fallback);
    REQUIRE(state.delete_object(one));
    CHECK(state.find_material(first) != nullptr);
    const ai3::MaterialId second = state.create_material("Material");
    const ai3::MaterialId third = state.create_material("Material");
    CHECK(second == 2);
    CHECK(third == 3);
    CHECK(state.find_material(second)->name == "Material 2");
    CHECK(state.find_material(third)->name == "Material 3");
    CHECK(state.materials().size() == 3);
}

TEST_CASE("material validation and semantic no-ops are transactional")
{
    ai3::EditorState state;
    const ai3::MaterialId id = state.create_material("Material");
    ai3::Material material = *state.find_material(id);
    const auto revision = state.document_revision();
    REQUIRE(state.set_material(id, material));
    CHECK(state.document_revision() == revision);
    material.shading = ai3::MaterialShading::phong;
    material.specular_power = 96.0F;
    REQUIRE(state.set_material(id, material));
    CHECK(state.document_revision() == revision + 1);
    const ai3::Material exact = *state.find_material(id);
    material.specular_power = std::numeric_limits<float>::quiet_NaN();
    CHECK_THROWS_AS(state.set_material(id, material), std::invalid_argument);
    CHECK(state.find_material(id)->specular_power == exact.specular_power);
}

TEST_CASE("material shading accepts only Lambert and Phong")
{
    ai3::EditorState state;
    ai3::Material lambert;
    lambert.shading = ai3::MaterialShading::lambert;
    CHECK_NOTHROW(state.create_material("Material", lambert));
    ai3::Material phong;
    phong.shading = ai3::MaterialShading::phong;
    const ai3::MaterialId phong_id = state.create_material("Material", phong);
    const ai3::Material original = *state.find_material(phong_id);
    const auto revision = state.document_revision();
    ai3::Material invalid = original;
    invalid.shading = static_cast<ai3::MaterialShading>(99);
    CHECK_THROWS_AS(state.set_material(phong_id, invalid), std::invalid_argument);
    CHECK(state.document_revision() == revision);
    CHECK(state.find_material(phong_id)->shading == ai3::MaterialShading::phong);
    CHECK_THROWS_AS(state.create_material("Material", invalid), std::invalid_argument);
    CHECK(state.document_revision() == revision);
}

TEST_CASE("history restores material creation edits assignments and cancellation")
{
    ai3::EditorState state;
    const ai3::ObjectId sphere = state.create_sphere("Sphere");
    ai3::EditorHistory history{state};
    REQUIRE(history.begin_transaction());
    const ai3::MaterialId material = state.create_material("Material");
    REQUIRE(state.assign_material(sphere, material));
    REQUIRE(history.commit_transaction());
    REQUIRE(history.undo());
    CHECK(state.materials().empty());
    CHECK(state.find_object(sphere)->sphere.material_id == ai3::no_material);
    REQUIRE(history.redo());
    CHECK(state.find_material(material) != nullptr);
    CHECK(state.find_object(sphere)->sphere.material_id == material);
    REQUIRE(history.begin_transaction());
    ai3::Material edited = *state.find_material(material);
    edited.diffuse_color = {0.1F, 0.2F, 0.3F};
    REQUIRE(state.set_material(material, edited));
    REQUIRE(history.cancel_transaction());
    CHECK(state.find_material(material)->diffuse_color != edited.diffuse_color);
}

TEST_CASE("active material edits participate in document dirty checkpoints")
{
    ai3::EditorState state;
    ai3::DocumentSession session{state};
    CHECK_FALSE(session.dirty());
    REQUIRE(session.history().begin_transaction());
    state.create_material("Material");
    CHECK(session.dirty());
    REQUIRE(session.history().cancel_transaction());
    CHECK_FALSE(session.dirty());
}

TEST_CASE("Scene Document v2 round trips materials and rejects dangling assignments")
{
    ai3::EditorState source;
    ai3::Material phong;
    phong.shading = ai3::MaterialShading::phong;
    phong.specular_power = 64.0F;
    const ai3::MaterialId material = source.create_material("Material", phong);
    const ai3::ObjectId sphere = source.create_sphere("Sphere");
    REQUIRE(source.assign_material(sphere, material));
    std::string document;
    REQUIRE(ai3::serialize_scene_document(source, document));
    CHECK(document.find("\"version\": 3") != std::string::npos);
    ai3::EditorState loaded;
    REQUIRE(ai3::deserialize_scene_document(document, loaded));
    CHECK(loaded.materials().size() == 1);
    CHECK(loaded.find_material(material)->shading == ai3::MaterialShading::phong);
    CHECK(loaded.find_object(sphere)->sphere.material_id == material);
    const std::string dangling = [&]
    {
        auto text = document;
        const auto pos = text.find("\"material_id\": 1");
        text.replace(pos, 16, "\"material_id\": 9");
        return text;
    }();
    CHECK_FALSE(ai3::deserialize_scene_document(dangling, loaded));
    CHECK(loaded.find_object(sphere)->sphere.material_id == material);
}

TEST_CASE("Scene Document v1 migrates visible colors and sphere defaults deterministically")
{
    ai3::EditorState source;
    source.create_sphere("Sphere");
    source.create_directional_light("Light", {{0.5F, 0.25F, 1.0F}, 2.0F});
    std::string text;
    REQUIRE(ai3::serialize_scene_document(source, text));
    auto legacy = nlohmann::json::parse(text);
    legacy["version"] = 1;
    legacy["metadata"]["default_name_counters"].erase("box");
    legacy.erase("materials");
    legacy["metadata"].erase("next_material_id");
    legacy["metadata"].erase("default_material_name_count");
    auto& sphere = legacy["objects"][0]["payload"];
    sphere.erase("material_id");
    sphere.erase("fallback_color_linear");
    auto& light = legacy["objects"][1]["payload"];
    light["color"] = light["color_linear"];
    light.erase("color_linear");
    ai3::EditorState loaded;
    REQUIRE(ai3::deserialize_scene_document(legacy.dump(), loaded));
    CHECK(loaded.materials().empty());
    CHECK(loaded.objects()[0].sphere.material_id == ai3::no_material);
    CHECK(ai3::linear_to_srgb(loaded.objects()[0].sphere.fallback_color.x) ==
          doctest::Approx(0.22F).epsilon(0.0001));
    CHECK(loaded.objects()[1].directional_light.color.x ==
          doctest::Approx(ai3::srgb_to_linear(0.5F)));
    CHECK(loaded.objects()[1].directional_light.color.y ==
          doctest::Approx(ai3::srgb_to_linear(0.25F)));
}

TEST_CASE("Scene Document v2 rejects invalid shading and linear colors transactionally")
{
    ai3::EditorState source;
    source.create_material("Material");
    source.create_sphere("Sphere");
    source.create_directional_light("Light");
    const nlohmann::json valid = encoded(source);

    std::vector<nlohmann::json> invalid_documents;
    auto shading = valid;
    shading["materials"][0]["shading"] = "unknown";
    invalid_documents.push_back(shading);
    for (const char* field :
         {"ambient_color_linear", "diffuse_color_linear", "specular_color_linear"})
    {
        auto material_color = valid;
        material_color["materials"][0][field][0] = 1.01;
        invalid_documents.push_back(material_color);
    }
    auto material_non_finite = valid;
    material_non_finite["materials"][0]["diffuse_color_linear"][0] =
        std::numeric_limits<double>::infinity();
    invalid_documents.push_back(material_non_finite);
    auto fallback_high = valid;
    fallback_high["objects"][0]["payload"]["fallback_color_linear"][1] = 1.01;
    invalid_documents.push_back(fallback_high);
    auto fallback_low = valid;
    fallback_low["objects"][0]["payload"]["fallback_color_linear"][1] = -0.01;
    invalid_documents.push_back(fallback_low);
    auto fallback_non_finite = valid;
    fallback_non_finite["objects"][0]["payload"]["fallback_color_linear"][1] =
        std::numeric_limits<double>::infinity();
    invalid_documents.push_back(fallback_non_finite);
    auto light_high = valid;
    light_high["objects"][1]["payload"]["color_linear"][2] = 1.01;
    invalid_documents.push_back(light_high);
    auto light_low = valid;
    light_low["objects"][1]["payload"]["color_linear"][2] = -0.01;
    invalid_documents.push_back(light_low);
    auto light_non_finite = valid;
    light_non_finite["objects"][1]["payload"]["color_linear"][2] =
        std::numeric_limits<double>::infinity();
    invalid_documents.push_back(light_non_finite);

    for (const nlohmann::json& candidate : invalid_documents)
    {
        ai3::EditorState destination;
        const ai3::ObjectId existing = destination.create_sphere("Existing");
        REQUIRE(destination.select(existing));
        const auto revision = destination.document_revision();
        CHECK_FALSE(load(candidate, destination));
        CHECK(destination.objects().size() == 1);
        CHECK(destination.selection() == existing);
        CHECK(destination.document_revision() == revision);
    }
}
