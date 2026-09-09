#pragma once

namespace freerouting {
namespace drc {

/**
 * Represents a position in the DRC report, matching KiCad's JSON schema.
 */
class DrcPosition
{
public:
    /**
     * X coordinate in the coordinate units specified in the report
     */
    double x;

    /**
     * Y coordinate in the coordinate units specified in the report
     */
    double y;

    DrcPosition(double p_x, double p_y)
        : x(p_x)
        , y(p_y)
    {
    }
};

} // namespace drc
} // namespace freerouting
