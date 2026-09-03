#pragma once

#include "editor/editor_state.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace ai3
{
std::filesystem::path workspace_path_for_scene(const std::filesystem::path& scene_path);
bool serialize_workspace(const std::map<ObjectId, BoundsDisplayState>& workspace,
                         std::string& document, std::string* error = nullptr);
bool deserialize_workspace(std::string_view document,
                           std::map<ObjectId, BoundsDisplayState>& workspace,
                           std::string* error = nullptr);
bool load_workspace_file(const std::filesystem::path& path,
                         std::map<ObjectId, BoundsDisplayState>& workspace,
                         std::string* error = nullptr);
bool save_workspace_file(const std::map<ObjectId, BoundsDisplayState>& workspace,
                         const std::filesystem::path& path, std::string* error = nullptr);
} // namespace ai3
