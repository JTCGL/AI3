#pragma once

#include "editor/editor_state.h"
#include "scene/orbit_camera.h"
#include "scene/resolved_view.h"
#include "scene/scene_math.h"

namespace ai3
{
enum class ViewSource
{
    editor_view,
    scene_camera
};

enum class TransientNavigationOperation
{
    pan,
    orbit
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

// Display-independent state for one editor viewport. Scene-camera matrices are always derived
// from current scene data; only the selected camera identity is retained.
class ViewportView
{
    public:
    ViewSource source() const { return source_; }
    ViewportInteractionMode interaction_mode() const { return interaction_mode_; }
    ViewportTransformTool transform_tool() const { return transform_tool_; }
    CoordinateSpace reference_space() const { return reference_space_; }
    ObjectId scene_camera_id() const { return scene_camera_id_; }
    OrbitCamera& orbit() { return orbit_; }
    const OrbitCamera& orbit() const { return orbit_; }

    void use_editor_view();
    bool use_scene_camera(const EditorState& scene, ObjectId camera_id);
    void set_interaction_mode(ViewportInteractionMode mode);
    void set_transform_tool(ViewportTransformTool tool) { transform_tool_ = tool; }
    void set_reference_space(CoordinateSpace space) { reference_space_ = space; }
    ObjectId helper_hover_object(ObjectId picked_object) const;
    bool navigate(float yaw_delta_degrees, float pitch_delta_degrees);
    bool transient_navigate(TransientNavigationOperation operation, glm::vec2 pointer_delta,
                            float logical_viewport_height);
    bool zoom(float wheel_delta);
    ResolvedViewportView resolve(const EditorState& scene, float aspect_ratio);
    void reset();

    private:
    ViewSource source_ = ViewSource::editor_view;
    ViewportInteractionMode interaction_mode_ = ViewportInteractionMode::selection;
    ViewportTransformTool transform_tool_ = ViewportTransformTool::translation;
    CoordinateSpace reference_space_ = CoordinateSpace::world;
    ObjectId scene_camera_id_ = no_object;
    OrbitCamera orbit_;
};

class TransientNavigationGesture
{
    public:
    bool acquire(bool alt_held);
    bool active() const { return active_; }
    TransientNavigationOperation operation() const { return operation_; }
    bool dispatch(ViewportView& view, glm::vec2 pointer_delta, float logical_viewport_height) const;
    void release() { active_ = false; }

    private:
    bool active_ = false;
    TransientNavigationOperation operation_ = TransientNavigationOperation::pan;
};
} // namespace ai3
