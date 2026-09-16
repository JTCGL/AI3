#pragma once

#include "core/edit_history.h"

#include <map>
#include <string>

namespace ai3
{
class EditOperations
{
    public:
    EditOperations(Scene& scene, Workspace& workspace, EditHistory& history);
    EditOperations(const EditOperations&) = delete;
    EditOperations& operator=(const EditOperations&) = delete;

    ObjectId create_object(CreateObject object);
    ObjectId create_sphere(std::string localized_base_name, SpherePrimitive sphere = {});
    ObjectId create_box(std::string localized_base_name, BoxPrimitive box = {});
    ObjectId create_perspective_camera(std::string localized_base_name,
                                       PerspectiveCamera camera = {});
    ObjectId create_directional_light(std::string localized_base_name, DirectionalLight light = {});
    bool set_sphere(ObjectId id, SpherePrimitive sphere);
    bool set_box(ObjectId id, BoxPrimitive box);
    bool set_perspective_camera(ObjectId id, PerspectiveCamera camera);
    bool set_directional_light(ObjectId id, DirectionalLight light);
    MaterialId create_material(std::string localized_base_name, Material material = {});
    bool rename_material(MaterialId id, std::string name);
    bool set_material(MaterialId id, Material material);
    bool assign_material(ObjectId id, MaterialId material_id);
    bool set_sphere_fallback_color(ObjectId id, glm::vec3 linear_color);
    bool rename_object(ObjectId id, std::string name);
    bool set_object_enabled(ObjectId id, bool enabled);
    bool set_object_visible(ObjectId id, bool visible);
    bool set_local_transform(ObjectId id, Transform transform);
    bool set_world_position(ObjectId id, glm::vec3 world_position);
    bool reparent_object(ObjectId id, ObjectId new_parent);
    bool delete_object(ObjectId id);
    bool reset_scene();

    bool select(ObjectId id);
    void clear_selection();
    bool set_bounds_display(ObjectId id, BoundsDisplayState display);
    void replace_bounds_display(std::map<ObjectId, BoundsDisplayState> display);

    private:
    Scene& scene_;
    Workspace& workspace_;
    EditHistory& history_;
};
} // namespace ai3
