#pragma once

#include "core/edit_operations.h"

#include <array>
#include <string>
#include <vector>

namespace ai3
{
struct ConsoleMessage
{
    std::string key;
    std::string argument;
};

enum class EditorPanel : std::size_t
{
    scene_graph,
    viewport,
    object_inspector,
    console,
    count
};

class EditorState
{
    public:
    EditorState();
    Scene& scene();
    const Scene& scene() const;
    Workspace& workspace();
    const Workspace& workspace() const;
    EditHistory& history();
    const EditHistory& history() const;
    EditOperations& operations();
    const EditOperations& operations() const;
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
    // Places the object pivot in world space while preserving local orientation, scale, and
    // hierarchy exactly.
    bool set_world_position(ObjectId id, glm::vec3 world_position);
    // Changes hierarchy transactionally while preserving the object's complete world pose.
    // Returns false for invalid hierarchy or a local affine matrix not faithfully representable as
    // TRS.
    bool reparent_object(ObjectId id, ObjectId new_parent);
    bool delete_object(ObjectId id);
    bool reset_scene();
    const BoundsDisplayState& bounds_display(ObjectId id) const;
    bool set_bounds_display(ObjectId id, BoundsDisplayState display);
    void replace_bounds_workspace(std::map<ObjectId, BoundsDisplayState> workspace);
    const std::map<ObjectId, BoundsDisplayState>& bounds_workspace() const;
    DocumentRevision document_revision() const;
    const std::vector<SceneObject>& objects() const;
    const SceneObject* find_object(ObjectId id) const;
    const std::vector<Material>& materials() const;
    const Material* find_material(MaterialId id) const;
    std::vector<ObjectId> children_of(ObjectId parent) const;
    std::vector<const SceneObject*> objects_by_category(ObjectCategory category,
                                                        ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> primitives(PrimitiveKind kind,
                                               ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> cameras(CameraKind kind, ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> lights(LightKind kind, ObjectQueryFilter filter = {}) const;
    ResolvedTransform world_transform(ObjectId id) const;
    glm::mat4 world_transform_matrix(ObjectId id) const;
    glm::vec3 world_position(ObjectId id) const;
    glm::quat world_orientation(ObjectId id) const;

    ObjectId selection() const;
    bool select(ObjectId id);
    void clear_selection();
    bool panel_visible(EditorPanel panel) const;
    void set_panel_visible(EditorPanel panel, bool visible);
    const std::vector<ConsoleMessage>& console_messages() const;
    void add_console_message(std::string key, std::string argument = {});
    void clear_console();
    void request_layout_reset();
    bool consume_layout_reset_request();

    private:
    Scene scene_;
    Workspace workspace_;
    EditHistory history_;
    EditOperations operations_;
    std::array<bool, static_cast<std::size_t>(EditorPanel::count)> panel_visibility_ = {true, true,
                                                                                        true, true};
    std::vector<ConsoleMessage> console_messages_;
    bool layout_reset_requested_ = false;
};
} // namespace ai3
