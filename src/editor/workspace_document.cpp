#include "editor/workspace_document.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>

namespace ai3
{
namespace
{
using Json = nlohmann::json;
void fail(std::string message) { throw std::runtime_error(std::move(message)); }

ObjectId parse_id(const std::string& text)
{
    if (text.empty() || !std::all_of(text.begin(), text.end(),
                                     [](unsigned char value) { return std::isdigit(value) != 0; }))
        fail("workspace object ID is invalid");
    std::size_t used = 0;
    const unsigned long long value = std::stoull(text, &used);
    if (used != text.size() || value == no_object || value > std::numeric_limits<ObjectId>::max())
        fail("workspace object ID is invalid");
    return static_cast<ObjectId>(value);
}
} // namespace

std::filesystem::path workspace_path_for_scene(const std::filesystem::path& scene_path)
{
    std::filesystem::path result = scene_path;
    result.replace_extension(".ai3workspace");
    return result;
}

bool serialize_workspace(const WorkspaceDocument& workspace, std::string& document,
                         std::string* error)
{
    try
    {
        Json objects = Json::object();
        for (const auto& [id, state] : workspace.objects)
            objects[std::to_string(id)] = {{"showBoundingBox", state.show_bounding_box},
                                           {"showBoundingSphere", state.show_bounding_sphere},
                                           {"hoverFeedback", state.hover_feedback}};
        document = Json{{"format", "ai3-workspace"},
                        {"version", 1},
                        {"helperRenderingMode",
                         workspace.helper_rendering_mode == WorkspaceHelperRenderingMode::overlay
                             ? "overlay"
                             : "depth-tested"},
                        {"objects", objects}}
                       .dump(2) +
                   "\n";
        return true;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr)
            *error = exception.what();
        return false;
    }
}

bool deserialize_workspace(std::string_view document, WorkspaceDocument& workspace,
                           std::string* error)
{
    try
    {
        const Json root = Json::parse(document);
        if (!root.is_object() || root.value("format", "") != "ai3-workspace" ||
            !root.contains("version") || !root["version"].is_number_integer() ||
            root["version"].get<int>() != 1 || !root.contains("objects") ||
            !root["objects"].is_object())
            fail("unsupported or malformed AI3 workspace");
        for (const auto& [key, ignored] : root.items())
            if (key != "format" && key != "version" && key != "helperRenderingMode" &&
                key != "objects")
                fail("workspace contains an unsupported field");
        WorkspaceDocument candidate;
        if (root.contains("helperRenderingMode"))
        {
            if (!root["helperRenderingMode"].is_string())
                fail("workspace helper rendering mode must be a string");
            const std::string mode = root["helperRenderingMode"].get<std::string>();
            if (mode == "overlay")
                candidate.helper_rendering_mode = WorkspaceHelperRenderingMode::overlay;
            else if (mode == "depth-tested")
                candidate.helper_rendering_mode = WorkspaceHelperRenderingMode::depth_tested;
            else
                fail("workspace helper rendering mode is unsupported");
        }
        for (const auto& [key, value] : root["objects"].items())
        {
            if (!value.is_object())
                fail("workspace object entry must be an object");
            for (const auto& [field_name, ignored] : value.items())
                if (field_name != "showBoundingBox" && field_name != "showBoundingSphere" &&
                    field_name != "hoverFeedback")
                    fail("workspace object contains an unsupported field");
            BoundsDisplayState state;
            const auto field = [&](const char* name)
            {
                if (!value.contains(name))
                    return false;
                if (!value[name].is_boolean())
                    fail(std::string{"workspace field is not boolean: "} + name);
                return value[name].get<bool>();
            };
            state.show_bounding_box = field("showBoundingBox");
            state.show_bounding_sphere = field("showBoundingSphere");
            state.hover_feedback = field("hoverFeedback");
            candidate.objects.emplace(parse_id(key), state);
        }
        workspace = std::move(candidate);
        return true;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr)
            *error = exception.what();
        return false;
    }
}

bool load_workspace_file(const std::filesystem::path& path, WorkspaceDocument& workspace,
                         std::string* error)
{
    std::error_code status_error;
    if (!std::filesystem::exists(path, status_error))
    {
        if (status_error && error != nullptr)
            *error = status_error.message();
        if (status_error)
            return false;
        workspace = {};
        return true;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        if (error != nullptr)
            *error = "could not open workspace file";
        return false;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return deserialize_workspace(contents.str(), workspace, error);
}

bool save_workspace_file(const WorkspaceDocument& workspace, const std::filesystem::path& path,
                         std::string* error)
{
    std::string document;
    if (!serialize_workspace(workspace, document, error))
        return false;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path temporary = path;
    temporary += ".tmp-" + std::to_string(nonce);
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output || !(output << document) || !output.flush())
        {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            if (error != nullptr)
                *error = "could not write workspace temporary file";
            return false;
        }
    }
    std::error_code replace_error;
    std::filesystem::rename(temporary, path, replace_error);
    if (replace_error)
    {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        if (error != nullptr)
            *error = "could not replace workspace file: " + replace_error.message();
        return false;
    }
    return true;
}
} // namespace ai3
