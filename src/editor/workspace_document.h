#pragma once

#include "editor/editor_state.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace ai3
{
enum class WorkspaceHelperRenderingMode
{
    overlay,
    depth_tested
};

struct WorkspaceDocument
{
    WorkspaceHelperRenderingMode helper_rendering_mode = WorkspaceHelperRenderingMode::depth_tested;
    std::map<ObjectId, BoundsDisplayState> objects;
};

std::filesystem::path workspace_path_for_scene(const std::filesystem::path& scene_path);
bool serialize_workspace(const WorkspaceDocument& workspace, std::string& document,
                         std::string* error = nullptr);
bool deserialize_workspace(std::string_view document, WorkspaceDocument& workspace,
                           std::string* error = nullptr);
bool load_workspace_file(const std::filesystem::path& path, WorkspaceDocument& workspace,
                         std::string* error = nullptr);
bool save_workspace_file(const WorkspaceDocument& workspace, const std::filesystem::path& path,
                         std::string* error = nullptr);
} // namespace ai3
