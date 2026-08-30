#pragma once

#include "editor/editor_state.h"
#include "scene/orbit_camera.h"
#include "scene/resolved_view.h"

namespace ai3
{
enum class ViewSource
{
    orbit,
    perspective_camera
};

// Display-independent state for one editor viewport. Scene-camera matrices are always derived
// from current scene data; only the selected camera identity is retained.
class ViewportView
{
    public:
    ViewSource source() const { return source_; }
    ObjectId scene_camera_id() const { return scene_camera_id_; }
    OrbitCamera& orbit() { return orbit_; }
    const OrbitCamera& orbit() const { return orbit_; }

    void use_orbit();
    bool use_scene_camera(const EditorState& scene, ObjectId camera_id);
    ResolvedViewportView resolve(const EditorState& scene, float aspect_ratio);
    void reset();

    private:
    ViewSource source_ = ViewSource::orbit;
    ObjectId scene_camera_id_ = no_object;
    OrbitCamera orbit_;
};
} // namespace ai3
