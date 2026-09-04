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
        source_ != ViewSource::editor_view || !std::isfinite(yaw_delta_degrees) ||
        !std::isfinite(pitch_delta_degrees) ||
        (yaw_delta_degrees == 0.0F && pitch_delta_degrees == 0.0F))
        return false;
    editor_view_.orbit(yaw_delta_degrees, pitch_delta_degrees);
    return true;
}

std::optional<TransientNavigationGesture>
ViewportView::begin_transient_navigation(bool orbit_requested, glm::vec2 viewport_size) const
{
    if (source_ != ViewSource::editor_view || !std::isfinite(viewport_size.x) ||
        !std::isfinite(viewport_size.y) || viewport_size.x <= 0.0F || viewport_size.y <= 0.0F)
        return std::nullopt;
    return TransientNavigationGesture{
        orbit_requested ? TransientNavigationKind::orbit : TransientNavigationKind::pan,
        viewport_size};
}

bool ViewportView::update_transient_navigation(const TransientNavigationGesture& gesture,
                                               glm::vec2 pointer_delta_pixels)
{
    if (source_ != ViewSource::editor_view || !std::isfinite(gesture.viewport_size.x) ||
        !std::isfinite(gesture.viewport_size.y) || gesture.viewport_size.x <= 0.0F ||
        gesture.viewport_size.y <= 0.0F || !std::isfinite(pointer_delta_pixels.x) ||
        !std::isfinite(pointer_delta_pixels.y) ||
        (pointer_delta_pixels.x == 0.0F && pointer_delta_pixels.y == 0.0F))
        return false;
    switch (gesture.kind)
    {
    case TransientNavigationKind::pan:
        return editor_view_.pan(pointer_delta_pixels, gesture.viewport_size);
    case TransientNavigationKind::orbit:
        editor_view_.orbit(pointer_delta_pixels.x * 0.25F, -pointer_delta_pixels.y * 0.25F);
        return true;
    }
    return false;
}

bool ViewportView::zoom(float wheel_delta)
{
    if (source_ != ViewSource::editor_view || !std::isfinite(wheel_delta) || wheel_delta == 0.0F)
        return false;
    editor_view_.zoom(wheel_delta);
    return true;
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

    return {editor_view_.view_matrix(), editor_view_.projection_matrix(aspect_ratio),
            editor_view_.position()};
}

void ViewportView::reset()
{
    source_ = ViewSource::editor_view;
    scene_camera_id_ = no_object;
    editor_view_.reset();
}
} // namespace ai3
