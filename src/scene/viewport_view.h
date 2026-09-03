#pragma once

#include "editor/editor_state.h"
#include "scene/orbit_camera.h"
#include "scene/resolved_view.h"
#include "scene/scene_math.h"

namespace ai3
{
enum class ViewSource
{
    orbit,
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
enum class HelperRenderingMode
{
    overlay,
    depth_tested
};

// Display-independent state for one editor viewport. Scene-camera matrices are always derived
// from current scene data; only the selected camera identity is retained.
class ViewportView
{
    public:
    ViewSource source() const { return source_; }
    ViewportInteractionMode interaction_mode() const { return interaction_mode_; }
    ViewportTransformTool transform_tool() const { return transform_tool_; }
    CoordinateSpace reference_space() const { return reference_space_; }
    HelperRenderingMode helper_rendering_mode() const { return helper_rendering_mode_; }
    ObjectId scene_camera_id() const { return scene_camera_id_; }
    OrbitCamera& orbit() { return orbit_; }
    const OrbitCamera& orbit() const { return orbit_; }

    void use_orbit();
    bool use_scene_camera(const EditorState& scene, ObjectId camera_id);
    void set_interaction_mode(ViewportInteractionMode mode);
    void set_transform_tool(ViewportTransformTool tool) { transform_tool_ = tool; }
    void set_reference_space(CoordinateSpace space) { reference_space_ = space; }
    void set_helper_rendering_mode(HelperRenderingMode mode) { helper_rendering_mode_ = mode; }
    ObjectId helper_hover_object(ObjectId picked_object) const;
    bool navigate(float yaw_delta_degrees, float pitch_delta_degrees);
    bool zoom(float wheel_delta);
    ResolvedViewportView resolve(const EditorState& scene, float aspect_ratio);
    void reset();

    private:
    ViewSource source_ = ViewSource::orbit;
    ViewportInteractionMode interaction_mode_ = ViewportInteractionMode::selection;
    ViewportTransformTool transform_tool_ = ViewportTransformTool::translation;
    CoordinateSpace reference_space_ = CoordinateSpace::world;
    HelperRenderingMode helper_rendering_mode_ = HelperRenderingMode::overlay;
    ObjectId scene_camera_id_ = no_object;
    OrbitCamera orbit_;
};
} // namespace ai3
