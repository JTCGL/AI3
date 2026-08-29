#include "localization/resource_locator.h"

#include <stdexcept>

namespace ai3
{
std::vector<std::filesystem::path>
resource_search_paths(const std::filesystem::path& executable_path,
                      const std::filesystem::path& source_resource_path)
{
    std::vector<std::filesystem::path> paths;
    const std::filesystem::path executable_directory = executable_path.has_parent_path()
                                                           ? executable_path.parent_path()
                                                           : std::filesystem::path{"."};
    paths.push_back(executable_directory / "assets");
    if (!source_resource_path.empty())
        paths.push_back(source_resource_path);
    return paths;
}

std::filesystem::path locate_resource_directory(const std::filesystem::path& executable_path)
{
    for (const std::filesystem::path& path :
         resource_search_paths(executable_path, AI3_SOURCE_RESOURCE_DIR))
    {
        if (std::filesystem::is_directory(path / "locales"))
            return path;
    }
    throw std::runtime_error(
        "AI3 resource directory not found beside executable or in source tree");
}
} // namespace ai3
