#include "scene/length_units.h"

namespace ai3
{
namespace
{
float meters_per_unit(LengthUnit unit)
{
    switch (unit)
    {
    case LengthUnit::millimeter:
        return 0.001F;
    case LengthUnit::centimeter:
        return 0.01F;
    case LengthUnit::meter:
        return 1.0F;
    case LengthUnit::kilometer:
        return 1000.0F;
    }
    return 1.0F;
}
} // namespace

float length_from_meters(float meters, LengthUnit display_unit)
{
    return meters / meters_per_unit(display_unit);
}

float length_to_meters(float displayed_length, LengthUnit display_unit)
{
    return displayed_length * meters_per_unit(display_unit);
}

std::string_view length_unit_symbol(LengthUnit unit)
{
    switch (unit)
    {
    case LengthUnit::millimeter:
        return "mm";
    case LengthUnit::centimeter:
        return "cm";
    case LengthUnit::meter:
        return "m";
    case LengthUnit::kilometer:
        return "km";
    }
    return "m";
}
} // namespace ai3
