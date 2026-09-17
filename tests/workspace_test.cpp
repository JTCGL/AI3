#include "core/workspace.h"

#include <doctest/doctest.h>

#include <map>

TEST_CASE("Core Workspace defaults are display independent")
{
    const ai3::Workspace workspace;

    CHECK(workspace.selection() == ai3::no_object);
    CHECK(workspace.bounds_display_states().empty());
    CHECK_FALSE(workspace.bounds_display(42).show_bounding_box);
    CHECK_FALSE(workspace.bounds_display(42).show_bounding_sphere);
    CHECK_FALSE(workspace.bounds_display(42).hover_feedback);
    CHECK(workspace.active_material() == ai3::no_material);
    CHECK(workspace.display_length_unit() == ai3::default_display_length_unit);
    CHECK(workspace.viewport().source == ai3::ViewSource::editor_view);
    CHECK(workspace.viewport().scene_camera_id == ai3::no_object);
    CHECK(workspace.viewport().interaction_mode == ai3::ViewportInteractionMode::selection);
    CHECK(workspace.viewport().transform_tool == ai3::ViewportTransformTool::translation);
    CHECK(workspace.viewport().reference_space == ai3::CoordinateSpace::world);
}

TEST_CASE("Core Workspace stores and clears selection")
{
    ai3::Workspace workspace;

    workspace.set_selection(7);
    CHECK(workspace.selection() == 7);
    workspace.clear_selection();
    CHECK(workspace.selection() == ai3::no_object);
}

TEST_CASE("Core Workspace owns sparse replaceable bounds display state")
{
    ai3::Workspace workspace;

    workspace.set_bounds_display(3, {true, false, true});
    CHECK(workspace.bounds_display(3).show_bounding_box);
    CHECK_FALSE(workspace.bounds_display(3).show_bounding_sphere);
    CHECK(workspace.bounds_display(3).hover_feedback);

    workspace.set_bounds_display(3, {});
    CHECK(workspace.bounds_display_states().empty());

    workspace.replace_bounds_display(std::map<ai3::ObjectId, ai3::BoundsDisplayState>{
        {4, {false, true, false}}, {8, {true, true, true}}, {9, {}}});
    CHECK(workspace.bounds_display_states().size() == 2);
    CHECK(workspace.bounds_display(4).show_bounding_sphere);
    CHECK(workspace.bounds_display(8).hover_feedback);
    workspace.remove_bounds_display(4);
    CHECK(workspace.bounds_display_states().size() == 1);
    CHECK_FALSE(workspace.bounds_display(4).show_bounding_sphere);
}

TEST_CASE("Core Workspace owns active material selection and display length unit")
{
    ai3::Workspace workspace;

    workspace.set_active_material(12);
    CHECK(workspace.active_material() == 12);
    workspace.clear_active_material();
    CHECK(workspace.active_material() == ai3::no_material);

    workspace.set_display_length_unit(ai3::LengthUnit::centimeter);
    CHECK(workspace.display_length_unit() == ai3::LengthUnit::centimeter);
}
