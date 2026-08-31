#include "app/runtime_paths.h"

namespace ai3
{
std::string imgui_ini_filename(const std::filesystem::path& executable_directory,
                               bool save_settings)
{
    return save_settings ? (executable_directory / "imgui.ini").string() : std::string{};
}
} // namespace ai3
