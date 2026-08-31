#include <doctest/doctest.h>

#include "app/runtime_paths.h"

#include <filesystem>

TEST_CASE("ImGui settings path is disabled or fixed beside the executable")
{
    const std::filesystem::path executable_directory =
        std::filesystem::temp_directory_path() / "ai3-runtime-path-test";
    CHECK(ai3::imgui_ini_filename(executable_directory, false).empty());

    const std::string expected = (executable_directory / "imgui.ini").string();
    CHECK(ai3::imgui_ini_filename(executable_directory, true) == expected);

    const std::filesystem::path original_working_directory = std::filesystem::current_path();
    std::filesystem::current_path(std::filesystem::temp_directory_path());
    CHECK(ai3::imgui_ini_filename(executable_directory, true) == expected);
    std::filesystem::current_path(original_working_directory);
}
