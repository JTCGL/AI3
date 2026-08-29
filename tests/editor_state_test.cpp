#include <doctest/doctest.h>

#include "editor/editor_state.h"
#include "scene/length_units.h"

TEST_CASE("objects are created with stable monotonic scene IDs")
{
    ai3::EditorState state;
    CHECK(state.objects().empty());
    const ai3::ObjectId root = state.create_object({"Scene"});
    const ai3::ObjectId first =
        state.create_object({"Sphere", root, {}, true, true, ai3::PrimitiveKind::sphere});
    CHECK(root == 1);
    CHECK(first == 2);
    CHECK(state.delete_object(first));
    const ai3::ObjectId second =
        state.create_object({"Sphere", root, {}, true, true, ai3::PrimitiveKind::sphere});
    CHECK(second == 3);
    CHECK(state.find_object(root)->id == root);
    CHECK(state.find_object(first) == nullptr);
}

TEST_CASE("sphere creation stores semantic radius in meters")
{
    ai3::EditorState state;
    const ai3::ObjectId sphere = state.create_object(
        {"Sphere", ai3::no_object, {}, false, true, ai3::PrimitiveKind::sphere});
    const ai3::SceneObject* object = state.find_object(sphere);
    REQUIRE(object != nullptr);
    CHECK(object->primitive == ai3::PrimitiveKind::sphere);
    CHECK(object->sphere.radius_meters == doctest::Approx(1.0F));
    CHECK_FALSE(object->enabled);
    const float centimeters =
        ai3::length_from_meters(object->sphere.radius_meters, ai3::LengthUnit::centimeter);
    CHECK(centimeters == doctest::Approx(100.0F));
    CHECK(ai3::length_to_meters(centimeters, ai3::LengthUnit::centimeter) ==
          doctest::Approx(object->sphere.radius_meters));
}

TEST_CASE("deleting a parent recursively deletes descendants and selected objects")
{
    ai3::EditorState state;
    const ai3::ObjectId root = state.create_object({"Root"});
    const ai3::ObjectId child = state.create_object({"Child", root});
    const ai3::ObjectId grandchild =
        state.create_object({"Sphere", child, {}, true, true, ai3::PrimitiveKind::sphere});
    const ai3::ObjectId survivor = state.create_object({"Survivor"});
    REQUIRE(state.select(grandchild));
    CHECK(state.delete_object(root));
    CHECK(state.selection() == ai3::no_object);
    CHECK(state.find_object(root) == nullptr);
    CHECK(state.find_object(child) == nullptr);
    CHECK(state.find_object(grandchild) == nullptr);
    CHECK(state.find_object(survivor) != nullptr);
    CHECK_FALSE(state.delete_object(root));
}

TEST_CASE("renderer-facing enumeration returns every visible enabled sphere")
{
    ai3::EditorState state;
    const ai3::ObjectId visible = state.create_object(
        {"Visible", ai3::no_object, {}, true, true, ai3::PrimitiveKind::sphere});
    state.create_object({"Hidden", ai3::no_object, {}, true, false, ai3::PrimitiveKind::sphere});
    state.create_object({"Disabled", ai3::no_object, {}, false, true, ai3::PrimitiveKind::sphere});
    state.create_object({"Group"});
    const auto spheres = state.visible_spheres();
    REQUIRE(spheres.size() == 1);
    CHECK(spheres.front()->id == visible);
}

TEST_CASE("selection and mutable properties are headless editor state")
{
    ai3::EditorState state;
    const ai3::ObjectId sphere =
        state.create_object({"Sphere", ai3::no_object, {}, true, true, ai3::PrimitiveKind::sphere});
    const std::size_t initial_messages = state.console_messages().size();
    CHECK(state.select(sphere));
    CHECK(state.selection() == sphere);
    CHECK(state.console_messages().size() == initial_messages + 1);
    CHECK_FALSE(state.select(sphere));
    CHECK_FALSE(state.select(999));
    state.find_object(sphere)->transform.position = {1.0F, 2.0F, 3.0F};
    const ai3::EditorState& observed = state;
    CHECK(observed.find_object(sphere)->transform.position.z == doctest::Approx(3.0F));
}

TEST_CASE("panel visibility console and layout reset behavior remain explicit")
{
    ai3::EditorState state;
    state.set_panel_visible(ai3::EditorPanel::console, false);
    CHECK_FALSE(state.panel_visible(ai3::EditorPanel::console));
    state.clear_console();
    state.add_console_message("test.message", "argument");
    REQUIRE(state.console_messages().size() == 1);
    CHECK(state.console_messages()[0].argument == "argument");
    state.request_layout_reset();
    CHECK(state.panel_visible(ai3::EditorPanel::console));
    CHECK(state.consume_layout_reset_request());
    CHECK_FALSE(state.consume_layout_reset_request());
}
