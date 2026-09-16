#pragma once

#include <string_view>

namespace ai3
{
enum class LengthUnit
{
    millimeter,
    centimeter,
    meter,
    kilometer
};

inline constexpr LengthUnit default_display_length_unit = LengthUnit::meter;

float length_from_meters(float meters, LengthUnit display_unit);
float length_to_meters(float displayed_length, LengthUnit display_unit);
std::string_view length_unit_symbol(LengthUnit unit);
} // namespace ai3
