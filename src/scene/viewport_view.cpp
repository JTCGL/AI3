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
bool is_perspective_camera(const EditorState& scene, ObjectId id)
{
    const SceneObject* object = scene.find_object(id);
    return object != nullptr && object->category == ObjectCategory::camera &&
           object->camera_kind == CameraKind::perspective;
}
} // namespace

void ViewportView::use_orbit()
{
    source_ = ViewSource::orbit;
    scene_camera_id_ = no_object;
}

bool ViewportView::use_scene_camera(const EditorState& scene, ObjectId camera_id)
{
    if (!is_perspective_camera(scene, camera_id))
        return false;
    source_ = ViewSource::perspective_camera;
    scene_camera_id_ = camera_id;
    return true;
}

ResolvedViewportView ViewportView::resolve(const EditorState& scene, float aspect_ratio)
{
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0.0F)
        throw std::invalid_argument("Viewport aspect ratio must be positive");

    if (source_ == ViewSource::perspective_camera)
    {
        const SceneObject* camera = scene.find_object(scene_camera_id_);
        if (!is_perspective_camera(scene, scene_camera_id_))
            use_orbit();
        else
        {
            const ResolvedTransform world = scene.world_transform(scene_camera_id_);
            const glm::quat orientation = glm::normalize(world.orientation);
            const glm::mat4 view = glm::mat4_cast(glm::conjugate(orientation)) *
                                   glm::translate(glm::mat4{1.0F}, -world.position);
            const PerspectiveCamera& perspective = camera->perspective_camera;
            return {view,
                    glm::perspective(glm::radians(perspective.vertical_fov_degrees), aspect_ratio,
                                     perspective.near_plane_meters, perspective.far_plane_meters)};
        }
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
