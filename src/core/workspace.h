#pragma once

#include "core/length_units.h"
#include "core/scene.h"

#include <glm/vec3.hpp>

#include <map>

namespace ai3
{
struct BoundsDisplayState
{
    bool show_bounding_box = false;
    bool show_bounding_sphere = false;
    bool hover_feedback = false;
};

enum class ViewSource
{
    editor_view,
    scene_camera
};

enum class ViewportInteractionMode
{
    selection,
    navigation
};

enum class ViewportTransformTool
{
    translation
};

enum class CoordinateSpace
{
    local,
    parent,
    world,
    view
};

struct EditorViewState
{
    glm::vec3 target{};
    float yaw_degrees = 35.0F;
    float pitch_degrees = 20.0F;
    float distance = 6.0F;
};

struct ViewportState
{
    ViewSource source = ViewSource::editor_view;
    ObjectId scene_camera_id = no_object;
    EditorViewState editor_view;
    ViewportInteractionMode interaction_mode = ViewportInteractionMode::selection;
    ViewportTransformTool transform_tool = ViewportTransformTool::translation;
    CoordinateSpace reference_space = CoordinateSpace::world;
};

class Workspace
{
    public:
    ObjectId selection() const;
    void set_selection(ObjectId id);
    void clear_selection();

    const BoundsDisplayState& bounds_display(ObjectId id) const;
    void set_bounds_display(ObjectId id, BoundsDisplayState display);
    void remove_bounds_display(ObjectId id);
    void replace_bounds_display(std::map<ObjectId, BoundsDisplayState> display);
    const std::map<ObjectId, BoundsDisplayState>& bounds_display_states() const;

    MaterialId active_material() const;
    void set_active_material(MaterialId id);
    void clear_active_material();

    LengthUnit display_length_unit() const;
    void set_display_length_unit(LengthUnit unit);

    ViewportState& viewport();
    const ViewportState& viewport() const;

    private:
    ObjectId selection_ = no_object;
    std::map<ObjectId, BoundsDisplayState> bounds_display_;
    MaterialId active_material_ = no_material;
    LengthUnit display_length_unit_ = default_display_length_unit;
    ViewportState viewport_;
};
} // namespace ai3
