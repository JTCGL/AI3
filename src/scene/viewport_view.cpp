#include "scene/viewport_view.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>
#include <stdexcept>

namespace ai3
{
namespace
{
bool is_supported_scene_camera(const SceneObject& object)
{
    if (object.category != ObjectCategory::camera)
        return false;
    switch (object.camera_kind)
    {
    case CameraKind::perspective:
        return true;
    case CameraKind::none:
        return false;
    }
    return false;
}

ResolvedViewportView resolve_scene_camera(const EditorState& scene, const SceneObject& camera,
                                          float aspect_ratio)
{
    const ResolvedTransform world = scene.world_transform(camera.id);
    const glm::quat orientation = glm::normalize(world.orientation);
    const glm::mat4 view = glm::mat4_cast(glm::conjugate(orientation)) *
                           glm::translate(glm::mat4{1.0F}, -world.position);
    switch (camera.camera_kind)
    {
    case CameraKind::perspective:
    {
        const PerspectiveCamera& perspective = camera.perspective_camera;
        return {view,
                glm::perspective(glm::radians(perspective.vertical_fov_degrees), aspect_ratio,
                                 perspective.near_plane_meters, perspective.far_plane_meters),
                world.position};
    }
    case CameraKind::none:
        break;
    }
    throw std::invalid_argument("Scene camera subtype is not supported as a viewport view");
}
} // namespace

void ViewportView::use_editor_view()
{
    source_ = ViewSource::editor_view;
    scene_camera_id_ = no_object;
}

bool ViewportView::use_scene_camera(const EditorState& scene, ObjectId camera_id)
{
    const SceneObject* camera = scene.find_object(camera_id);
    if (camera == nullptr || !is_supported_scene_camera(*camera))
        return false;
    source_ = ViewSource::scene_camera;
    scene_camera_id_ = camera_id;
    return true;
}

void ViewportView::set_interaction_mode(ViewportInteractionMode mode) { interaction_mode_ = mode; }

ObjectId ViewportView::helper_hover_object(ObjectId picked_object) const
{
    return interaction_mode_ == ViewportInteractionMode::selection ? picked_object : no_object;
}

bool ViewportView::navigate(float yaw_delta_degrees, float pitch_delta_degrees)
{
    if (interaction_mode_ != ViewportInteractionMode::navigation ||
        source_ != ViewSource::editor_view)
        return false;
    return orbit_.orbit(yaw_delta_degrees, pitch_delta_degrees);
}

bool ViewportView::transient_navigate(TransientNavigationOperation operation,
                                      glm::vec2 pointer_delta, float logical_viewport_height)
{
    if (source_ != ViewSource::editor_view)
        return false;
    if (operation == TransientNavigationOperation::pan)
        return orbit_.pan(pointer_delta, logical_viewport_height);
    return orbit_.orbit(pointer_delta.x * 0.25F, -pointer_delta.y * 0.25F);
}

bool ViewportView::zoom(float wheel_delta)
{
    if (source_ != ViewSource::editor_view)
        return false;
    return orbit_.zoom(wheel_delta);
}

ResolvedViewportView ViewportView::resolve(const EditorState& scene, float aspect_ratio)
{
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0.0F)
        throw std::invalid_argument("Viewport aspect ratio must be positive");

    if (source_ == ViewSource::scene_camera)
    {
        const SceneObject* camera = scene.find_object(scene_camera_id_);
        if (camera == nullptr || !is_supported_scene_camera(*camera))
            use_editor_view();
        else
            return resolve_scene_camera(scene, *camera, aspect_ratio);
    }

    return {orbit_.view_matrix(), orbit_.projection_matrix(aspect_ratio), orbit_.position()};
}

void ViewportView::reset()
{
    source_ = ViewSource::editor_view;
    scene_camera_id_ = no_object;
    orbit_.reset();
}

bool TransientNavigationGesture::acquire(bool shift_held)
{
    if (active_)
        return false;
    operation_ =
        shift_held ? TransientNavigationOperation::orbit : TransientNavigationOperation::pan;
    active_ = true;
    return true;
}

bool TransientNavigationGesture::dispatch(ViewportView& view, glm::vec2 pointer_delta,
                                          float logical_viewport_height) const
{
    return active_ && view.transient_navigate(operation_, pointer_delta, logical_viewport_height);
}
} // namespace ai3
