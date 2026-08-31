#include <doctest/doctest.h>

#include "editor/editor_state.h"
#include "scene/length_units.h"
#include "scene/scene_math.h"

#include <glm/geometric.hpp>

#include <stdexcept>

namespace
{
ai3::CreateObject sphere_object(std::string name, ai3::ObjectId parent = ai3::no_object)
{
    ai3::CreateObject object{std::move(name), parent};
    object.category = ai3::ObjectCategory::primitive;
    object.primitive_kind = ai3::PrimitiveKind::sphere;
    return object;
}

ai3::CreateObject camera_object(std::string name, ai3::ObjectId parent = ai3::no_object)
{
    ai3::CreateObject object{std::move(name), parent};
    object.category = ai3::ObjectCategory::camera;
    object.camera_kind = ai3::CameraKind::perspective;
    return object;
}

ai3::CreateObject light_object(std::string name, ai3::ObjectId parent = ai3::no_object)
{
    ai3::CreateObject object{std::move(name), parent};
    object.category = ai3::ObjectCategory::light;
    object.light_kind = ai3::LightKind::directional;
    return object;
}

void check_vec3(const glm::vec3& actual, const glm::vec3& expected)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.0001));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.0001));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.0001));
}
} // namespace

TEST_CASE("editor state starts with an empty scene and no selection")
{
    ai3::EditorState state;
    CHECK(state.objects().empty());
    CHECK(state.selection() == ai3::no_object);
    CHECK(state.document_revision() == 0);
}

TEST_CASE("document revision advances only for real authoritative mutations")
{
    ai3::EditorState state;
    const auto initial = state.document_revision();
    const ai3::ObjectId root = state.create_object(ai3::CreateObject{"Root"});
    CHECK(state.document_revision() == initial + 1);

    auto revision = state.document_revision();
    REQUIRE(state.rename_object(root, "Root"));
    REQUIRE(state.set_object_enabled(root, true));
    REQUIRE(state.set_object_visible(root, true));
    REQUIRE(state.set_local_transform(root, state.find_object(root)->transform));
    CHECK(state.document_revision() == revision);

    REQUIRE(state.rename_object(root, "Renamed"));
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.set_object_enabled(root, false));
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.set_object_visible(root, false));
    CHECK(state.document_revision() == ++revision);
    ai3::Transform moved = state.find_object(root)->transform;
    moved.position.x = 2.0F;
    REQUIRE(state.set_local_transform(root, moved));
    CHECK(state.document_revision() == ++revision);

    const ai3::ObjectId sphere = state.create_sphere("Sphere");
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.set_sphere(sphere, state.find_object(sphere)->sphere));
    CHECK(state.document_revision() == revision);
    REQUIRE(state.set_sphere(sphere, {2.0F}));
    CHECK(state.document_revision() == ++revision);

    const ai3::ObjectId camera = state.create_perspective_camera("Camera");
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.set_perspective_camera(camera, {60.0F, 0.2F, 200.0F}));
    CHECK(state.document_revision() == ++revision);
    const ai3::ObjectId light = state.create_directional_light("Light");
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.set_directional_light(light, {{0.5F, 0.6F, 0.7F}, 2.0F}));
    CHECK(state.document_revision() == ++revision);

    REQUIRE(state.reparent_object(sphere, root));
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.reparent_object(sphere, root));
    CHECK(state.document_revision() == revision);

    state.select(root);
    state.set_panel_visible(ai3::EditorPanel::console, false);
    state.add_console_message("test");
    state.request_layout_reset();
    state.consume_layout_reset_request();
    CHECK(state.document_revision() == revision);

    REQUIRE(state.delete_object(light));
    CHECK(state.document_revision() == ++revision);
    REQUIRE(state.reset_scene());
    CHECK(state.document_revision() == ++revision);
    CHECK_FALSE(state.reset_scene());
    CHECK(state.document_revision() == revision);
}

TEST_CASE("objects are created with stable monotonic scene IDs")
{
    ai3::EditorState state;
    const ai3::ObjectId root = state.create_object(ai3::CreateObject{"Scene"});
    const ai3::ObjectId first = state.create_object(sphere_object("Sphere", root));
    CHECK(root == 1);
    CHECK(first == 2);
    CHECK(state.delete_object(first));
    CHECK(state.create_object(sphere_object("Sphere", root)) == 3);
}

TEST_CASE("sphere creation preserves semantic radius and localized monotonic names")
{
    ai3::EditorState state;
    const ai3::ObjectId first = state.create_sphere("Sphere", {2.0F});
    const ai3::ObjectId second = state.create_sphere("Sphere");
    REQUIRE(state.find_object(first) != nullptr);
    CHECK(state.find_object(first)->category == ai3::ObjectCategory::primitive);
    CHECK(state.find_object(first)->primitive_kind == ai3::PrimitiveKind::sphere);
    CHECK(state.find_object(first)->sphere.radius_meters == doctest::Approx(2.0F));
    CHECK(state.find_object(first)->name == "Sphere 1");
    CHECK(state.find_object(second)->name == "Sphere 2");
    CHECK(ai3::length_from_meters(state.find_object(first)->sphere.radius_meters,
                                  ai3::LengthUnit::centimeter) == doctest::Approx(200.0F));
    CHECK(state.delete_object(first));
    CHECK(state.find_object(state.create_sphere("Esfera"))->name == "Esfera 3");
}

TEST_CASE("camera and directional light creation store semantic defaults")
{
    ai3::EditorState state;
    const ai3::SceneObject* camera = state.find_object(state.create_perspective_camera("Camera"));
    REQUIRE(camera != nullptr);
    CHECK(camera->category == ai3::ObjectCategory::camera);
    CHECK(camera->camera_kind == ai3::CameraKind::perspective);
    CHECK(camera->perspective_camera.vertical_fov_degrees == doctest::Approx(50.0F));
    CHECK(camera->perspective_camera.near_plane_meters == doctest::Approx(0.1F));
    CHECK(camera->perspective_camera.far_plane_meters == doctest::Approx(100.0F));
    const ai3::SceneObject* light =
        state.find_object(state.create_directional_light("Directional Light"));
    REQUIRE(light != nullptr);
    CHECK(light->category == ai3::ObjectCategory::light);
    CHECK(light->light_kind == ai3::LightKind::directional);
    check_vec3(light->directional_light.color, {1.0F, 1.0F, 1.0F});
    CHECK(light->directional_light.intensity == doctest::Approx(1.0F));
}

TEST_CASE("default naming is independent monotonic and failed creation consumes no number")
{
    ai3::EditorState state;
    CHECK_THROWS_AS(state.create_sphere("Sphere", {-1.0F}), std::invalid_argument);
    CHECK_THROWS_AS(state.create_perspective_camera("Camera", {180.0F, 0.1F, 100.0F}),
                    std::invalid_argument);
    CHECK_THROWS_AS(state.create_directional_light("Directional Light", {glm::vec3{1.0F}, -1.0F}),
                    std::invalid_argument);
    const ai3::ObjectId sphere = state.create_sphere("Sphere");
    const ai3::ObjectId camera = state.create_perspective_camera("Camera");
    const ai3::ObjectId light = state.create_directional_light("Directional Light");
    CHECK(state.find_object(sphere)->name == "Sphere 1");
    CHECK(state.find_object(camera)->name == "Camera 1");
    CHECK(state.find_object(light)->name == "Directional Light 1");
    CHECK(state.delete_object(camera));
    CHECK(state.find_object(state.create_perspective_camera("Camera"))->name == "Camera 2");
    CHECK(state.find_object(state.create_sphere("Sphere"))->name == "Sphere 2");
    CHECK(state.find_object(state.create_directional_light("Directional Light"))->name ==
          "Directional Light 2");
}

TEST_CASE("scene reset resets object identity and every default-name sequence")
{
    ai3::EditorState state;
    state.create_sphere("Sphere");
    state.create_perspective_camera("Camera");
    state.create_directional_light("Directional Light");
    state.reset_scene();
    CHECK(state.create_perspective_camera("Camera") == 1);
    CHECK(state.find_object(1)->name == "Camera 1");
    CHECK(state.find_object(state.create_sphere("Sphere"))->name == "Sphere 1");
    CHECK(state.find_object(state.create_directional_light("Directional Light"))->name ==
          "Directional Light 1");
}

TEST_CASE("camera and light semantic validation rejects invalid updates")
{
    ai3::EditorState state;
    const ai3::ObjectId camera = state.create_perspective_camera("Camera");
    for (float fov : {0.0F, 180.0F, -1.0F})
        CHECK_THROWS_AS(state.set_perspective_camera(camera, {fov, 0.1F, 100.0F}),
                        std::invalid_argument);
    CHECK_THROWS_AS(state.set_perspective_camera(camera, {50.0F, 0.0F, 100.0F}),
                    std::invalid_argument);
    CHECK_THROWS_AS(state.set_perspective_camera(camera, {50.0F, 10.0F, 10.0F}),
                    std::invalid_argument);
    CHECK_THROWS_AS(state.set_perspective_camera(camera, {50.0F, 10.0F, 5.0F}),
                    std::invalid_argument);
    const ai3::ObjectId light = state.create_directional_light("Directional Light");
    CHECK_THROWS_AS(state.set_directional_light(light, {glm::vec3{1.0F}, -0.01F}),
                    std::invalid_argument);
    CHECK(state.find_object(camera)->perspective_camera.vertical_fov_degrees ==
          doctest::Approx(50.0F));
    CHECK(state.find_object(light)->directional_light.intensity == doctest::Approx(1.0F));
}

TEST_CASE("root camera and light directions derive from quaternion negative Z")
{
    ai3::EditorState state;
    const ai3::ObjectId camera = state.create_perspective_camera("Camera");
    const ai3::ObjectId light = state.create_directional_light("Directional Light");
    check_vec3(ai3::camera_forward_direction(state, camera), {0.0F, 0.0F, -1.0F});
    ai3::Transform camera_transform = state.find_object(camera)->transform;
    camera_transform.orientation = ai3::orientation_from_euler_degrees({90.0F, 0.0F, 0.0F});
    REQUIRE(state.set_local_transform(camera, camera_transform));
    check_vec3(ai3::camera_forward_direction(state, camera), {0.0F, 1.0F, 0.0F});
    ai3::Transform light_transform = state.find_object(light)->transform;
    light_transform.orientation = ai3::orientation_from_euler_degrees({0.0F, 90.0F, 0.0F});
    REQUIRE(state.set_local_transform(light, light_transform));
    check_vec3(ai3::directional_light_direction(state, light), {-1.0F, 0.0F, 0.0F});
}

TEST_CASE("deletion preserves descendants and clears only a deleted selection")
{
    ai3::EditorState state;
    const ai3::ObjectId root = state.create_object(ai3::CreateObject{"Root"});
    const ai3::ObjectId camera = state.create_object(camera_object("Camera", root));
    const ai3::ObjectId light = state.create_object(light_object("Light", camera));
    const ai3::ObjectId survivor = state.create_sphere("Sphere");
    REQUIRE(state.select(light));
    CHECK(state.delete_object(root));
    CHECK(state.selection() == light);
    CHECK(state.find_object(camera)->parent_id() == ai3::no_object);
    CHECK(state.find_object(light)->parent_id() == camera);
    CHECK(state.find_object(survivor) != nullptr);

    REQUIRE(state.select(camera));
    CHECK(state.delete_object(camera));
    CHECK(state.selection() == ai3::no_object);
    CHECK(state.find_object(light)->parent_id() == ai3::no_object);
}

TEST_CASE("category and subtype queries distinguish objects and filter enabled visibility")
{
    ai3::EditorState state;
    ai3::CreateObject visible_sphere = sphere_object("Visible");
    const ai3::ObjectId visible = state.create_object(visible_sphere);
    ai3::CreateObject hidden_sphere = sphere_object("Hidden");
    hidden_sphere.visible = false;
    state.create_object(hidden_sphere);
    ai3::CreateObject disabled_light = light_object("Disabled light");
    disabled_light.enabled = false;
    state.create_object(disabled_light);
    const ai3::ObjectId light = state.create_directional_light("Directional Light");
    state.create_perspective_camera("Camera");
    CHECK(state.objects_by_category(ai3::ObjectCategory::primitive).size() == 2);
    const auto renderable = state.primitives(ai3::PrimitiveKind::sphere, {true, true});
    REQUIRE(renderable.size() == 1);
    CHECK(renderable.front()->id == visible);
    CHECK(state.cameras(ai3::CameraKind::perspective).size() == 1);
    const auto enabled_lights = state.lights(ai3::LightKind::directional, {true, false});
    REQUIRE(enabled_lights.size() == 1);
    CHECK(enabled_lights.front()->id == light);
}

TEST_CASE("selection mutable transforms panels console and layout remain headless state")
{
    ai3::EditorState state;
    const ai3::ObjectId sphere = state.create_sphere("Sphere");
    const std::size_t messages = state.console_messages().size();
    CHECK(state.select(sphere));
    CHECK(state.console_messages().size() == messages + 1);
    ai3::Transform transform = state.find_object(sphere)->transform;
    transform.position = {1.0F, 2.0F, 3.0F};
    REQUIRE(state.set_local_transform(sphere, transform));
    CHECK(state.find_object(sphere)->transform.position.z == doctest::Approx(3.0F));
    state.set_panel_visible(ai3::EditorPanel::console, false);
    CHECK_FALSE(state.panel_visible(ai3::EditorPanel::console));
    state.clear_console();
    state.add_console_message("test.message", "argument");
    state.request_layout_reset();
    CHECK(state.panel_visible(ai3::EditorPanel::console));
    CHECK(state.consume_layout_reset_request());
    CHECK_FALSE(state.consume_layout_reset_request());
}
