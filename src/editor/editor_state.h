#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ai3
{
class SceneDocumentCodec;
class EditorHistory;
using ObjectId = std::uint64_t;
using MaterialId = std::uint64_t;
using DocumentRevision = std::uint64_t;
constexpr ObjectId no_object = 0;
constexpr MaterialId no_material = 0;

struct Transform
{
    glm::vec3 position{0.0F};
    glm::quat orientation{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F};
};

glm::mat4 compose_transform(const Transform& transform);
bool valid_transform(const Transform& transform);

struct ResolvedTransform
{
    glm::mat4 matrix{1.0F};
    glm::vec3 position{0.0F};
    glm::quat orientation{1.0F, 0.0F, 0.0F, 0.0F};
};

enum class ObjectCategory
{
    general,
    primitive,
    camera,
    light
};
enum class PrimitiveKind
{
    none,
    sphere
};
enum class CameraKind
{
    none,
    perspective
};
enum class LightKind
{
    none,
    directional
};

struct SpherePrimitive
{
    float radius_meters = 1.0F;
    MaterialId material_id = no_material;
    // Linear RGB equivalent of the established artist-facing sRGB blue (0.22, 0.58, 0.92).
    glm::vec3 fallback_color{0.0396819F, 0.2957F, 0.827571F};
};
struct PerspectiveCamera
{
    float vertical_fov_degrees = 50.0F;
    float near_plane_meters = 0.1F;
    float far_plane_meters = 100.0F;
};
struct DirectionalLight
{
    glm::vec3 color{1.0F};
    float intensity = 1.0F;
};

enum class MaterialShading
{
    lambert,
    phong
};

struct Material
{
    MaterialId id = no_material;
    std::string name;
    MaterialShading shading = MaterialShading::lambert;
    glm::vec3 ambient_color{0.02F};
    glm::vec3 diffuse_color{0.214041F};
    glm::vec3 specular_color{1.0F};
    float specular_power = 32.0F;
};

struct SceneObject
{
    ObjectId id = no_object;
    std::string name;
    bool enabled = true;
    bool visible = true;
    // Authoritative local-to-parent transform; for a root object this is also its world transform.
    Transform transform;
    ObjectCategory category = ObjectCategory::general;
    PrimitiveKind primitive_kind = PrimitiveKind::none;
    CameraKind camera_kind = CameraKind::none;
    LightKind light_kind = LightKind::none;
    SpherePrimitive sphere;
    PerspectiveCamera perspective_camera;
    DirectionalLight directional_light;

    ObjectId parent_id() const { return parent_id_; }

    private:
    ObjectId parent_id_ = no_object;
    friend class EditorState;
    friend class SceneDocumentCodec;
};

struct CreateObject
{
    explicit CreateObject(std::string object_name, ObjectId object_parent = no_object,
                          Transform object_transform = {}, bool object_enabled = true,
                          bool object_visible = true)
        : name(std::move(object_name)), parent(object_parent), transform(object_transform),
          enabled(object_enabled), visible(object_visible)
    {
    }

    std::string name;
    ObjectId parent = no_object;
    Transform transform;
    bool enabled = true;
    bool visible = true;
    ObjectCategory category = ObjectCategory::general;
    PrimitiveKind primitive_kind = PrimitiveKind::none;
    CameraKind camera_kind = CameraKind::none;
    LightKind light_kind = LightKind::none;
    SpherePrimitive sphere;
    PerspectiveCamera perspective_camera;
    DirectionalLight directional_light;
};

struct ObjectQueryFilter
{
    bool enabled_only = false;
    bool visible_only = false;
};

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
    ObjectId create_object(CreateObject object);
    ObjectId create_sphere(std::string localized_base_name, SpherePrimitive sphere = {});
    ObjectId create_perspective_camera(std::string localized_base_name,
                                       PerspectiveCamera camera = {});
    ObjectId create_directional_light(std::string localized_base_name, DirectionalLight light = {});
    bool set_sphere(ObjectId id, SpherePrimitive sphere);
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
    // Changes hierarchy transactionally while preserving the object's complete world pose.
    // Returns false for invalid hierarchy or a local affine matrix not faithfully representable as
    // TRS.
    bool reparent_object(ObjectId id, ObjectId new_parent);
    bool delete_object(ObjectId id);
    bool reset_scene();
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
    friend class EditorHistory;
    friend class SceneDocumentCodec;
    struct SubtypeKey
    {
        ObjectCategory category;
        int subtype;
        bool operator<(const SubtypeKey& other) const
        {
            return std::pair{category, subtype} < std::pair{other.category, other.subtype};
        }
        bool operator==(const SubtypeKey& other) const
        {
            return category == other.category && subtype == other.subtype;
        }
    };
    ObjectId create_named_object(std::string localized_base_name, CreateObject object,
                                 SubtypeKey subtype);
    SceneObject* find_object_mutable(ObjectId id);
    Material* find_material_mutable(MaterialId id);
    void advance_document_revision();

    std::vector<SceneObject> objects_;
    std::vector<Material> materials_;
    ObjectId next_object_id_ = 1;
    MaterialId next_material_id_ = 1;
    std::uint64_t default_material_name_count_ = 0;
    std::map<SubtypeKey, std::uint64_t> default_name_counts_;
    DocumentRevision document_revision_ = 0;
    ObjectId selection_ = no_object;
    std::array<bool, static_cast<std::size_t>(EditorPanel::count)> panel_visibility_ = {true, true,
                                                                                        true, true};
    std::vector<ConsoleMessage> console_messages_;
    bool layout_reset_requested_ = false;
};
} // namespace ai3
