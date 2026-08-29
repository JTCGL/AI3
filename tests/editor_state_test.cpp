#include <doctest/doctest.h>

#include "editor/editor_state.h"

TEST_CASE("dummy scene hierarchy supports lookup and selection")
{
    ai3::EditorState state;
    REQUIRE(state.find_object(4) != nullptr);
    CHECK(state.find_object(4)->name == "Cube");
    CHECK(state.children_of(ai3::no_object) == std::vector<ai3::ObjectId>{1});
    CHECK(state.children_of(5) == std::vector<ai3::ObjectId>{6, 7});

    const std::size_t initial_messages = state.console_messages().size();
    CHECK(state.select(4));
    CHECK(state.selection() == 4);
    CHECK(state.console_messages().size() == initial_messages + 1);
    CHECK(state.console_messages().back() == "Selected Cube.");
    CHECK_FALSE(state.select(4));
    CHECK_FALSE(state.select(999));
    CHECK(state.console_messages().size() == initial_messages + 1);
}

TEST_CASE("object properties are shared mutable editor state")
{
    ai3::EditorState state;
    ai3::SceneObject* cube = state.find_object(4);
    REQUIRE(cube != nullptr);
    cube->enabled = false;
    cube->transform.position = {1.0F, 2.0F, 3.0F};

    const ai3::EditorState& observed = state;
    CHECK_FALSE(observed.find_object(4)->enabled);
    CHECK(observed.find_object(4)->transform.position[2] == doctest::Approx(3.0F));
}

TEST_CASE("panel visibility and console behavior are explicit")
{
    ai3::EditorState state;
    CHECK(state.panel_visible(ai3::EditorPanel::console));
    state.set_panel_visible(ai3::EditorPanel::console, false);
    CHECK_FALSE(state.panel_visible(ai3::EditorPanel::console));

    CHECK_FALSE(state.console_messages().empty());
    state.clear_console();
    CHECK(state.console_messages().empty());
    state.add_console_message("test message");
    CHECK(state.console_messages() == std::vector<std::string>{"test message"});
}

TEST_CASE("layout reset restores core panels and is consumed once")
{
    ai3::EditorState state;
    state.set_panel_visible(ai3::EditorPanel::scene_graph, false);
    state.set_panel_visible(ai3::EditorPanel::viewport, false);
    state.set_panel_visible(ai3::EditorPanel::object_inspector, false);
    state.set_panel_visible(ai3::EditorPanel::console, false);

    CHECK_FALSE(state.consume_layout_reset_request());
    state.request_layout_reset();
    CHECK(state.panel_visible(ai3::EditorPanel::scene_graph));
    CHECK(state.panel_visible(ai3::EditorPanel::viewport));
    CHECK(state.panel_visible(ai3::EditorPanel::object_inspector));
    CHECK(state.panel_visible(ai3::EditorPanel::console));
    CHECK(state.consume_layout_reset_request());
    CHECK_FALSE(state.consume_layout_reset_request());
}
