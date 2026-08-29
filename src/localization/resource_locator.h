#pragma once

#include <filesystem>
#include <vector>

namespace ai3
{
std::vector<std::filesystem::path>
resource_search_paths(const std::filesystem::path& executable_path,
                      const std::filesystem::path& source_resource_path = {});
std::filesystem::path locate_resource_directory(const std::filesystem::path& executable_path);
} // namespace ai3
