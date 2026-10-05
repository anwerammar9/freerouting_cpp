#include "drc/ZoneIslandViolation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace freerouting {
namespace drc {

using geometry::planar::FloatPoint;

ZoneIslandViolation::ZoneIslandViolation(board::ConductionArea* p_conduction_area,
                                         std::vector<FloatPoint> p_island_contour,
                                         int p_layer,
                                         int p_net_number,
                                         const std::string& p_type,
                                         std::vector<board::Item*> p_items_in_island)
    : conduction_area_(p_conduction_area)
    , island_contour_(std::move(p_island_contour))
    , layer_(p_layer)
    , net_number_(p_net_number)
    , type_(p_type)
    , items_in_island_(std::move(p_items_in_island))
{
}

double ZoneIslandViolation::get_center_x() const
{
    if (island_contour_.empty())
    {
        return 0.0;
    }
    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    for (const FloatPoint& corner : island_contour_)
    {
        min_x = std::min(min_x, corner.x);
        max_x = std::max(max_x, corner.x);
    }
    return (min_x + max_x) / 2.0;
}

double ZoneIslandViolation::get_center_y() const
{
    if (island_contour_.empty())
    {
        return 0.0;
    }
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    for (const FloatPoint& corner : island_contour_)
    {
        min_y = std::min(min_y, corner.y);
        max_y = std::max(max_y, corner.y);
    }
    return (min_y + max_y) / 2.0;
}

double ZoneIslandViolation::get_area_in_board_units() const
{
    if (island_contour_.empty())
    {
        return 0.0;
    }
    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    for (const FloatPoint& corner : island_contour_)
    {
        min_x = std::min(min_x, corner.x);
        max_x = std::max(max_x, corner.x);
        min_y = std::min(min_y, corner.y);
        max_y = std::max(max_y, corner.y);
    }
    return (max_x - min_x) * (max_y - min_y);
}

} // namespace drc
} // namespace freerouting