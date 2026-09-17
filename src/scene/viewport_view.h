#pragma once

#include "core/workspace.h"
#include "scene/orbit_camera.h"
#include "scene/resolved_view.h"
#include "scene/scene_math.h"

namespace ai3
{
enum class TransientNavigationOperation
{
    pan,
    orbit
};

// Display-independent Scene-dependent behavior for one editor viewport. Authoritative retained
// state lives in Workspace; scene-camera matrices are derived from current Scene data.
class ViewportView
{
    public:
    explicit ViewportView(Workspace& workspace)
        : state_(workspace.viewport()), orbit_(state_.editor_view)
    {
    }
    ViewSource source() const { return state_.source; }
    ViewportInteractionMode interaction_mode() const { return state_.interaction_mode; }
    ViewportTransformTool transform_tool() const { return state_.transform_tool; }
    CoordinateSpace reference_space() const { return state_.reference_space; }
    ObjectId scene_camera_id() const { return state_.scene_camera_id; }
    OrbitCamera& orbit() { return orbit_; }
    const OrbitCamera& orbit() const { return orbit_; }

    void use_editor_view();
    bool use_scene_camera(const Scene& scene, ObjectId camera_id);
    void set_interaction_mode(ViewportInteractionMode mode);
    void set_transform_tool(ViewportTransformTool tool) { state_.transform_tool = tool; }
    void set_reference_space(CoordinateSpace space) { state_.reference_space = space; }
    ObjectId helper_hover_object(ObjectId picked_object) const;
    bool navigate(float yaw_delta_degrees, float pitch_delta_degrees);
    bool transient_navigate(TransientNavigationOperation operation, glm::vec2 pointer_delta,
                            float logical_viewport_height);
    bool zoom(float wheel_delta);
    ResolvedViewportView resolve(const Scene& scene, float aspect_ratio);
    void reset();

    private:
    ViewportState& state_;
    OrbitCamera orbit_;
};

class TransientNavigationGesture
{
    public:
    bool acquire(bool shift_held);
    bool active() const { return active_; }
    TransientNavigationOperation operation() const { return operation_; }
    bool dispatch(ViewportView& view, glm::vec2 pointer_delta, float logical_viewport_height) const;
    void release() { active_ = false; }

    private:
    bool active_ = false;
    TransientNavigationOperation operation_ = TransientNavigationOperation::pan;
};
} // namespace ai3
