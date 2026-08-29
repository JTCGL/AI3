#pragma once

#include <string>
#include <string_view>

namespace ai3
{
inline std::string stable_imgui_label(std::string_view visible_text, std::string_view stable_id)
{
    return std::string(visible_text) + "###" + std::string(stable_id);
}
} // namespace ai3
