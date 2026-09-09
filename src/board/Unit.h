#pragma once

#include <string>

namespace freerouting {
namespace board {

enum class Unit
{
    MIL,
    INCH,
    MM,
    UM
};

class UnitHelper
{
public:
    static constexpr double micrometers(Unit p_unit)
    {
        switch (p_unit)
        {
            case Unit::MIL: return 25.4;
            case Unit::INCH: return 25400.0;
            case Unit::MM: return 1000.0;
            case Unit::UM: return 1.0;
            default: return 1.0;
        }
    }

    static double scale(double p_value, Unit p_from_unit, Unit p_to_unit)
    {
        return p_value * micrometers(p_from_unit) / micrometers(p_to_unit);
    }

    static Unit from_string(const std::string& p_string)
    {
        if (p_string == "mil" || p_string == "MIL")
            return Unit::MIL;
        if (p_string == "inch" || p_string == "INCH")
            return Unit::INCH;
        if (p_string == "mm" || p_string == "MM")
            return Unit::MM;
        if (p_string == "um" || p_string == "UM")
            return Unit::UM;
        return Unit::MM;
    }

    static std::string to_string(Unit p_unit)
    {
        switch (p_unit)
        {
            case Unit::MIL: return "mil";
            case Unit::INCH: return "inch";
            case Unit::MM: return "mm";
            case Unit::UM: return "um";
            default: return "mm";
        }
    }
};

} // namespace board
} // namespace freerouting