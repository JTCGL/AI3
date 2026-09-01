#include "editor/scene_document.h"
#include "scene/color_space.h"

#include <glm/common.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace ai3
{
namespace
{
using Json = nlohmann::json;
constexpr std::string_view format_name = "ai3-scene";
constexpr std::uint64_t format_version = 2;

[[noreturn]] void invalid(const std::string& message) { throw std::invalid_argument(message); }

void require_fields(const Json& value, std::initializer_list<std::string_view> fields,
                    std::string_view context)
{
    if (!value.is_object())
        invalid(std::string(context) + " must be an object");
    std::set<std::string> expected;
    for (const std::string_view field : fields)
    {
        expected.emplace(field);
        if (!value.contains(field))
            invalid(std::string(context) + " is missing " + std::string(field));
    }
    for (const auto& entry : value.items())
        if (expected.count(entry.key()) == 0)
            invalid(std::string(context) + " contains unsupported field " + entry.key());
}

std::uint64_t unsigned_value(const Json& value, std::string_view context)
{
    if (!value.is_number_unsigned())
        invalid(std::string(context) + " must be an unsigned integer");
    return value.get<std::uint64_t>();
}

float number(const Json& value, std::string_view context)
{
    if (!value.is_number())
        invalid(std::string(context) + " must be a number");
    const double result = value.get<double>();
    if (!std::isfinite(result) || result < -std::numeric_limits<float>::max() ||
        result > std::numeric_limits<float>::max())
        invalid(std::string(context) + " must be a finite float");
    return static_cast<float>(result);
}

glm::vec3 vec3(const Json& value, std::string_view context)
{
    if (!value.is_array() || value.size() != 3)
        invalid(std::string(context) + " must contain three numbers");
    return {number(value[0], context), number(value[1], context), number(value[2], context)};
}

Json encode_vec3(const glm::vec3& value) { return Json::array({value.x, value.y, value.z}); }

Transform decode_transform(const Json& value)
{
    require_fields(value, {"position_meters", "orientation_wxyz", "scale"}, "transform");
    Transform result;
    result.position = vec3(value.at("position_meters"), "position");
    result.scale = vec3(value.at("scale"), "scale");
    const Json& orientation = value.at("orientation_wxyz");
    if (!orientation.is_array() || orientation.size() != 4)
        invalid("orientation_wxyz must contain four numbers");
    result.orientation = {
        number(orientation[0], "orientation"), number(orientation[1], "orientation"),
        number(orientation[2], "orientation"), number(orientation[3], "orientation")};
    if (!valid_transform(result))
        invalid("transform values must be finite and orientation must be normalized and nonzero");
    result.orientation = glm::normalize(result.orientation);
    return result;
}

Json encode_transform(const Transform& transform)
{
    if (!valid_transform(transform))
        invalid("scene contains an invalid transform");
    const glm::quat orientation = glm::normalize(transform.orientation);
    return {{"position_meters", encode_vec3(transform.position)},
            {"orientation_wxyz",
             Json::array({orientation.w, orientation.x, orientation.y, orientation.z})},
            {"scale", encode_vec3(transform.scale)}};
}

std::string category_string(ObjectCategory category)
{
    switch (category)
    {
    case ObjectCategory::general:
        return "general";
    case ObjectCategory::primitive:
        return "primitive";
    case ObjectCategory::camera:
        return "camera";
    case ObjectCategory::light:
        return "light";
    }
    invalid("scene contains an unsupported category");
}

std::string subtype_string(const SceneObject& object)
{
    const bool no_primitive = object.primitive_kind == PrimitiveKind::none;
    const bool no_camera = object.camera_kind == CameraKind::none;
    const bool no_light = object.light_kind == LightKind::none;
    if (object.category == ObjectCategory::general && no_primitive && no_camera && no_light)
        return "none";
    if (object.category == ObjectCategory::primitive &&
        object.primitive_kind == PrimitiveKind::sphere && no_camera && no_light)
        return "sphere";
    if (object.category == ObjectCategory::camera && no_primitive &&
        object.camera_kind == CameraKind::perspective && no_light)
        return "perspective";
    if (object.category == ObjectCategory::light && no_primitive && no_camera &&
        object.light_kind == LightKind::directional)
        return "directional";
    invalid("scene contains inconsistent category and subtype values");
}

Json encode_payload(const SceneObject& object)
{
    if (object.primitive_kind == PrimitiveKind::sphere)
    {
        if (!std::isfinite(object.sphere.radius_meters) || object.sphere.radius_meters <= 0.0F)
            invalid("scene contains an invalid sphere radius");
        return {{"radius_meters", object.sphere.radius_meters},
                {"material_id", object.sphere.material_id},
                {"fallback_color_linear", encode_vec3(object.sphere.fallback_color)}};
    }
    if (object.camera_kind == CameraKind::perspective)
    {
        const PerspectiveCamera& camera = object.perspective_camera;
        if (!std::isfinite(camera.vertical_fov_degrees) || camera.vertical_fov_degrees <= 0.0F ||
            camera.vertical_fov_degrees >= 180.0F || !std::isfinite(camera.near_plane_meters) ||
            camera.near_plane_meters <= 0.0F || !std::isfinite(camera.far_plane_meters) ||
            camera.far_plane_meters <= camera.near_plane_meters)
            invalid("scene contains invalid perspective camera parameters");
        return {{"vertical_fov_degrees", camera.vertical_fov_degrees},
                {"near_plane_meters", camera.near_plane_meters},
                {"far_plane_meters", camera.far_plane_meters}};
    }
    if (object.light_kind == LightKind::directional)
    {
        const DirectionalLight& light = object.directional_light;
        if (!std::isfinite(light.color.x) || !std::isfinite(light.color.y) ||
            !std::isfinite(light.color.z) || !std::isfinite(light.intensity) ||
            light.intensity < 0.0F)
            invalid("scene contains invalid directional-light parameters");
        return {{"color_linear", encode_vec3(light.color)}, {"intensity", light.intensity}};
    }
    return Json::object();
}

void decode_semantics(const std::string& category, const std::string& subtype, const Json& payload,
                      SceneObject& object, bool legacy)
{
    if (category == "general" && subtype == "none")
    {
        require_fields(payload, {}, "general payload");
        return;
    }
    if (category == "primitive" && subtype == "sphere")
    {
        if (legacy)
            require_fields(payload, {"radius_meters"}, "sphere payload");
        else
            require_fields(payload, {"radius_meters", "material_id", "fallback_color_linear"},
                           "sphere payload");
        object.category = ObjectCategory::primitive;
        object.primitive_kind = PrimitiveKind::sphere;
        object.sphere.radius_meters = number(payload.at("radius_meters"), "sphere radius");
        if (!legacy)
        {
            object.sphere.material_id = unsigned_value(payload.at("material_id"), "material ID");
            object.sphere.fallback_color =
                vec3(payload.at("fallback_color_linear"), "fallback color");
            if (!valid_linear_color(object.sphere.fallback_color))
                invalid("sphere fallback color is invalid");
        }
        if (object.sphere.radius_meters <= 0.0F)
            invalid("sphere radius must be positive");
        return;
    }
    if (category == "camera" && subtype == "perspective")
    {
        require_fields(payload, {"vertical_fov_degrees", "near_plane_meters", "far_plane_meters"},
                       "perspective-camera payload");
        object.category = ObjectCategory::camera;
        object.camera_kind = CameraKind::perspective;
        PerspectiveCamera& camera = object.perspective_camera;
        camera.vertical_fov_degrees = number(payload.at("vertical_fov_degrees"), "camera FOV");
        camera.near_plane_meters = number(payload.at("near_plane_meters"), "camera near plane");
        camera.far_plane_meters = number(payload.at("far_plane_meters"), "camera far plane");
        if (camera.vertical_fov_degrees <= 0.0F || camera.vertical_fov_degrees >= 180.0F ||
            camera.near_plane_meters <= 0.0F || camera.far_plane_meters <= camera.near_plane_meters)
            invalid("perspective-camera parameters are invalid");
        return;
    }
    if (category == "light" && subtype == "directional")
    {
        require_fields(payload, {legacy ? "color" : "color_linear", "intensity"},
                       "directional-light payload");
        object.category = ObjectCategory::light;
        object.light_kind = LightKind::directional;
        object.directional_light.color =
            vec3(payload.at(legacy ? "color" : "color_linear"), "directional-light color");
        if (legacy)
            object.directional_light.color = srgb_to_linear(
                glm::clamp(object.directional_light.color, glm::vec3{0.0F}, glm::vec3{1.0F}));
        if (!valid_linear_color(object.directional_light.color))
            invalid("directional-light color is invalid");
        object.directional_light.intensity = number(payload.at("intensity"), "light intensity");
        if (object.directional_light.intensity < 0.0F)
            invalid("directional-light intensity must be non-negative");
        return;
    }
    invalid("object category and subtype are unsupported or inconsistent");
}

std::string message(const std::exception& error)
{
    return std::string("Scene Document error: ") + error.what();
}
} // namespace

class SceneDocumentCodec
{
    public:
    static Json encode(const EditorState& scene)
    {
        Json materials = Json::array();
        std::set<MaterialId> material_ids;
        MaterialId maximum_material_id = no_material;
        for (const Material& material : scene.materials_)
        {
            if (material.id == no_material || !material_ids.insert(material.id).second ||
                !valid_linear_color(material.ambient_color) ||
                !valid_linear_color(material.diffuse_color) ||
                !valid_linear_color(material.specular_color) ||
                !std::isfinite(material.specular_power) || material.specular_power <= 0.0F)
                invalid("scene contains an invalid material");
            maximum_material_id = std::max(maximum_material_id, material.id);
            materials.push_back(
                {{"id", material.id},
                 {"name", material.name},
                 {"shading", material.shading == MaterialShading::lambert ? "lambert" : "phong"},
                 {"ambient_color_linear", encode_vec3(material.ambient_color)},
                 {"diffuse_color_linear", encode_vec3(material.diffuse_color)},
                 {"specular_color_linear", encode_vec3(material.specular_color)},
                 {"specular_power", material.specular_power}});
        }
        Json objects = Json::array();
        std::set<ObjectId> ids;
        ObjectId maximum_id = no_object;
        for (const SceneObject& object : scene.objects_)
        {
            if (object.id == no_object || !ids.insert(object.id).second)
                invalid("scene contains an invalid or duplicate object ID");
            maximum_id = std::max(maximum_id, object.id);
            objects.push_back({{"id", object.id},
                               {"name", object.name},
                               {"enabled", object.enabled},
                               {"visible", object.visible},
                               {"parent_id", object.parent_id_},
                               {"transform", encode_transform(object.transform)},
                               {"category", category_string(object.category)},
                               {"subtype", subtype_string(object)},
                               {"payload", encode_payload(object)}});
        }
        if (scene.next_object_id_ == no_object ||
            scene.next_object_id_ == std::numeric_limits<ObjectId>::max() ||
            scene.next_object_id_ <= maximum_id)
            invalid("scene allocator state is inconsistent with object IDs");
        for (const SceneObject& object : scene.objects_)
            if (object.parent_id_ != no_object && ids.count(object.parent_id_) == 0)
                invalid("scene contains a missing parent");
            else if (object.sphere.material_id != no_material &&
                     material_ids.count(object.sphere.material_id) == 0)
                invalid("scene contains a missing material reference");
        if (scene.next_material_id_ == no_material ||
            scene.next_material_id_ == std::numeric_limits<MaterialId>::max() ||
            scene.next_material_id_ <= maximum_material_id ||
            scene.default_material_name_count_ >= scene.next_material_id_)
            invalid("scene material allocator state is inconsistent");

        const auto counter = [&](ObjectCategory category, int subtype)
        {
            const auto found = scene.default_name_counts_.find({category, subtype});
            return found == scene.default_name_counts_.end() ? std::uint64_t{0} : found->second;
        };
        Json counters = {
            {"sphere", counter(ObjectCategory::primitive, static_cast<int>(PrimitiveKind::sphere))},
            {"perspective_camera",
             counter(ObjectCategory::camera, static_cast<int>(CameraKind::perspective))},
            {"directional_light",
             counter(ObjectCategory::light, static_cast<int>(LightKind::directional))}};
        for (const auto& entry : counters.items())
            if (entry.value().get<std::uint64_t>() >= scene.next_object_id_)
                invalid("scene name-counter state is inconsistent with allocator state");
        return {{"format", format_name},
                {"version", format_version},
                {"metadata",
                 {{"next_object_id", scene.next_object_id_},
                  {"next_material_id", scene.next_material_id_},
                  {"default_material_name_count", scene.default_material_name_count_},
                  {"default_name_counters", std::move(counters)}}},
                {"materials", std::move(materials)},
                {"objects", std::move(objects)}};
    }

    static void decode(const Json& root, EditorState& destination)
    {
        if (!root.is_object() || !root.contains("version"))
            invalid("document is missing version");
        const std::uint64_t version = unsigned_value(root.at("version"), "document version");
        const bool legacy = version == 1;
        if (!legacy && version != format_version)
            invalid("document version is unsupported");
        require_fields(root,
                       legacy ? std::initializer_list<std::string_view>{"format", "version",
                                                                        "metadata", "objects"}
                              : std::initializer_list<std::string_view>{"format", "version",
                                                                        "metadata", "materials",
                                                                        "objects"},
                       "document");
        if (!root.at("format").is_string() || root.at("format").get<std::string>() != format_name)
            invalid("document format is not ai3-scene");
        const Json& metadata = root.at("metadata");
        if (legacy)
            require_fields(metadata, {"next_object_id", "default_name_counters"}, "metadata");
        else
            require_fields(metadata,
                           {"next_object_id", "next_material_id", "default_material_name_count",
                            "default_name_counters"},
                           "metadata");
        const ObjectId next_id = unsigned_value(metadata.at("next_object_id"), "next object ID");
        if (next_id == no_object || next_id == std::numeric_limits<ObjectId>::max())
            invalid("next object ID cannot be allocated safely");
        const Json& counters = metadata.at("default_name_counters");
        require_fields(counters, {"sphere", "perspective_camera", "directional_light"},
                       "default-name counters");
        const std::uint64_t sphere_count = unsigned_value(counters.at("sphere"), "sphere counter");
        const std::uint64_t camera_count =
            unsigned_value(counters.at("perspective_camera"), "camera counter");
        const std::uint64_t light_count =
            unsigned_value(counters.at("directional_light"), "light counter");
        if (sphere_count >= next_id || camera_count >= next_id || light_count >= next_id)
            invalid("default-name counter is inconsistent with allocator state");

        const Json& values = root.at("objects");
        if (!values.is_array())
            invalid("objects must be an array");
        EditorState candidate;
        candidate.objects_.clear();
        candidate.next_object_id_ = next_id;
        candidate.default_name_counts_.clear();
        if (!legacy)
        {
            candidate.next_material_id_ =
                unsigned_value(metadata.at("next_material_id"), "next material ID");
            candidate.default_material_name_count_ =
                unsigned_value(metadata.at("default_material_name_count"), "material name counter");
            if (candidate.next_material_id_ == no_material ||
                candidate.next_material_id_ == std::numeric_limits<MaterialId>::max() ||
                candidate.default_material_name_count_ >= candidate.next_material_id_)
                invalid("material allocator metadata is invalid");
            const Json& material_values = root.at("materials");
            if (!material_values.is_array())
                invalid("materials must be an array");
            MaterialId max_material = no_material;
            for (const Json& value : material_values)
            {
                require_fields(value,
                               {"id", "name", "shading", "ambient_color_linear",
                                "diffuse_color_linear", "specular_color_linear", "specular_power"},
                               "material");
                Material material;
                material.id = unsigned_value(value.at("id"), "material ID");
                if (material.id == no_material || candidate.find_material(material.id) != nullptr ||
                    !value.at("name").is_string() || !value.at("shading").is_string())
                    invalid("material identity or text is invalid");
                material.name = value.at("name").get<std::string>();
                const std::string shading = value.at("shading").get<std::string>();
                if (shading == "lambert")
                    material.shading = MaterialShading::lambert;
                else if (shading == "phong")
                    material.shading = MaterialShading::phong;
                else
                    invalid("material shading is unsupported");
                material.ambient_color = vec3(value.at("ambient_color_linear"), "ambient color");
                material.diffuse_color = vec3(value.at("diffuse_color_linear"), "diffuse color");
                material.specular_color = vec3(value.at("specular_color_linear"), "specular color");
                material.specular_power = number(value.at("specular_power"), "specular power");
                if (!valid_linear_color(material.ambient_color) ||
                    !valid_linear_color(material.diffuse_color) ||
                    !valid_linear_color(material.specular_color) || material.specular_power <= 0.0F)
                    invalid("material payload is invalid");
                max_material = std::max(max_material, material.id);
                candidate.materials_.push_back(std::move(material));
            }
            if (candidate.next_material_id_ <= max_material)
                invalid("next material ID must exceed stored IDs");
        }
        std::unordered_map<ObjectId, std::size_t> indices;
        ObjectId maximum_id = no_object;
        for (const Json& value : values)
        {
            require_fields(value,
                           {"id", "name", "enabled", "visible", "parent_id", "transform",
                            "category", "subtype", "payload"},
                           "scene object");
            SceneObject object;
            object.id = unsigned_value(value.at("id"), "object ID");
            if (object.id == no_object ||
                !indices.emplace(object.id, candidate.objects_.size()).second)
                invalid("object IDs must be unique and nonzero");
            maximum_id = std::max(maximum_id, object.id);
            if (!value.at("name").is_string() || !value.at("enabled").is_boolean() ||
                !value.at("visible").is_boolean() || !value.at("category").is_string() ||
                !value.at("subtype").is_string())
                invalid("scene object contains a field with the wrong type");
            object.name = value.at("name").get<std::string>();
            object.enabled = value.at("enabled").get<bool>();
            object.visible = value.at("visible").get<bool>();
            object.parent_id_ = unsigned_value(value.at("parent_id"), "parent ID");
            object.transform = decode_transform(value.at("transform"));
            decode_semantics(value.at("category").get<std::string>(),
                             value.at("subtype").get<std::string>(), value.at("payload"), object,
                             legacy);
            candidate.objects_.push_back(std::move(object));
        }
        if (next_id <= maximum_id)
            invalid("next object ID must exceed every stored object ID");
        for (const SceneObject& object : candidate.objects_)
        {
            if (object.parent_id_ == object.id)
                invalid("object cannot parent itself");
            if (object.parent_id_ != no_object && indices.count(object.parent_id_) == 0)
                invalid("object parent does not exist");
            if (object.sphere.material_id != no_material &&
                candidate.find_material(object.sphere.material_id) == nullptr)
                invalid("sphere material does not exist");
            std::set<ObjectId> ancestors;
            for (ObjectId parent = object.parent_id_; parent != no_object;)
            {
                if (!ancestors.insert(parent).second || parent == object.id)
                    invalid("scene hierarchy contains a cycle");
                parent = candidate.objects_[indices.at(parent)].parent_id_;
            }
        }
        candidate.default_name_counts_[{ObjectCategory::primitive,
                                        static_cast<int>(PrimitiveKind::sphere)}] = sphere_count;
        candidate.default_name_counts_[{ObjectCategory::camera,
                                        static_cast<int>(CameraKind::perspective)}] = camera_count;
        candidate.default_name_counts_[{ObjectCategory::light,
                                        static_cast<int>(LightKind::directional)}] = light_count;

        const bool changed = encode(destination) != encode(candidate);
        destination.objects_ = std::move(candidate.objects_);
        destination.materials_ = std::move(candidate.materials_);
        destination.next_object_id_ = candidate.next_object_id_;
        destination.next_material_id_ = candidate.next_material_id_;
        destination.default_material_name_count_ = candidate.default_material_name_count_;
        destination.default_name_counts_ = std::move(candidate.default_name_counts_);
        destination.selection_ = no_object;
        if (changed)
            destination.advance_document_revision();
    }
};

bool serialize_scene_document(const EditorState& scene, std::string& document, std::string* error)
{
    try
    {
        document = SceneDocumentCodec::encode(scene).dump(2) + "\n";
        return true;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr)
            *error = message(exception);
        return false;
    }
}

bool deserialize_scene_document(std::string_view document, EditorState& destination,
                                std::string* error)
{
    try
    {
        SceneDocumentCodec::decode(Json::parse(document), destination);
        return true;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr)
            *error = message(exception);
        return false;
    }
}

bool save_scene_document_file(const EditorState& scene, const std::filesystem::path& path,
                              std::string* error)
{
    std::string document;
    if (!serialize_scene_document(scene, document, error))
        return false;
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        if (error != nullptr)
            *error = "Unable to open Scene Document for writing: " + path.string();
        return false;
    }
    stream.write(document.data(), static_cast<std::streamsize>(document.size()));
    if (!stream)
    {
        if (error != nullptr)
            *error = "Unable to write Scene Document: " + path.string();
        return false;
    }
    return true;
}

bool load_scene_document_file(const std::filesystem::path& path, EditorState& destination,
                              std::string* error)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        if (error != nullptr)
            *error = "Unable to open Scene Document: " + path.string();
        return false;
    }
    const std::string document{std::istreambuf_iterator<char>(stream), {}};
    if (!stream.good() && !stream.eof())
    {
        if (error != nullptr)
            *error = "Unable to read Scene Document: " + path.string();
        return false;
    }
    return deserialize_scene_document(document, destination, error);
}
} // namespace ai3
