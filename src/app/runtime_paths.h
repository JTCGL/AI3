#pragma once

#include <filesystem>
#include <string>

namespace ai3
{
std::string imgui_ini_filename(const std::filesystem::path& executable_directory,
                               bool save_settings);
} // namespace ai3
