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
                                 perspective.near_plane_meters, perspective.far_plane_meters)};
    }
    case CameraKind::none:
        break;
    }
    throw std::invalid_argument("Scene camera subtype is not supported as a viewport view");
}
} // namespace

void ViewportView::use_orbit()
{
    source_ = ViewSource::orbit;
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

ResolvedViewportView ViewportView::resolve(const EditorState& scene, float aspect_ratio)
{
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0.0F)
        throw std::invalid_argument("Viewport aspect ratio must be positive");

    if (source_ == ViewSource::scene_camera)
    {
        const SceneObject* camera = scene.find_object(scene_camera_id_);
        if (camera == nullptr || !is_supported_scene_camera(*camera))
            use_orbit();
        else
            return resolve_scene_camera(scene, *camera, aspect_ratio);
    }

    return {orbit_.view_matrix(), orbit_.projection_matrix(aspect_ratio)};
}

void ViewportView::reset()
{
    source_ = ViewSource::orbit;
    scene_camera_id_ = no_object;
    orbit_.reset();
}
} // namespace ai3
