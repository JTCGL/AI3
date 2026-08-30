#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ai3
{
using ObjectId = std::uint64_t;
constexpr ObjectId no_object = 0;

struct Transform
{
    glm::vec3 position{0.0F};
    glm::quat orientation{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 scale{1.0F};
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

struct SceneObject
{
    ObjectId id = no_object;
    ObjectId parent = no_object;
    std::string name;
    bool enabled = true;
    bool visible = true;
    Transform transform;
    ObjectCategory category = ObjectCategory::general;
    PrimitiveKind primitive_kind = PrimitiveKind::none;
    CameraKind camera_kind = CameraKind::none;
    LightKind light_kind = LightKind::none;
    SpherePrimitive sphere;
    PerspectiveCamera perspective_camera;
    DirectionalLight directional_light;
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
    bool delete_object(ObjectId id);
    void reset_scene();
    const std::vector<SceneObject>& objects() const;
    SceneObject* find_object(ObjectId id);
    const SceneObject* find_object(ObjectId id) const;
    std::vector<ObjectId> children_of(ObjectId parent) const;
    std::vector<const SceneObject*> objects_by_category(ObjectCategory category,
                                                        ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> primitives(PrimitiveKind kind,
                                               ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> cameras(CameraKind kind, ObjectQueryFilter filter = {}) const;
    std::vector<const SceneObject*> lights(LightKind kind, ObjectQueryFilter filter = {}) const;

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
    struct SubtypeKey
    {
        ObjectCategory category;
        int subtype;
        bool operator<(const SubtypeKey& other) const
        {
            return std::pair{category, subtype} < std::pair{other.category, other.subtype};
        }
    };
    ObjectId create_named_object(std::string localized_base_name, CreateObject object,
                                 SubtypeKey subtype);

    std::vector<SceneObject> objects_;
    ObjectId next_object_id_ = 1;
    std::map<SubtypeKey, std::uint64_t> default_name_counts_;
    ObjectId selection_ = no_object;
    std::array<bool, static_cast<std::size_t>(EditorPanel::count)> panel_visibility_ = {true, true,
                                                                                        true, true};
    std::vector<ConsoleMessage> console_messages_;
    bool layout_reset_requested_ = false;
};
} // namespace ai3
