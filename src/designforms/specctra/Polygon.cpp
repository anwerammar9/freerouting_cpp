#include "designforms/specctra/Polygon.h"

#include <algorithm>
#include <cmath>

namespace freerouting {
namespace designforms {
namespace specctra {

Rectangle::Rectangle(double p_ll_x, double p_ll_y, double p_ur_x, double p_ur_y)
{
    coor_ = {{p_ll_x, p_ll_y, p_ur_x, p_ur_y}};
}

Rectangle::Rectangle(const std::array<double, 4>& p_coor)
    : coor_(p_coor)
{
}

geometry::planar::IntBox Rectangle::to_int_box() const
{
    int64_t ll_x = static_cast<int64_t>(std::round(std::min(coor_[0], coor_[2])));
    int64_t ll_y = static_cast<int64_t>(std::round(std::min(coor_[1], coor_[3])));
    int64_t ur_x = static_cast<int64_t>(std::round(std::max(coor_[0], coor_[2])));
    int64_t ur_y = static_cast<int64_t>(std::round(std::max(coor_[1], coor_[3])));
    return geometry::planar::IntBox(ll_x, ll_y, ur_x, ur_y);
}

Rectangle Rectangle::union_with(const Rectangle& p_other) const
{
    return Rectangle(
        std::min(coor_[0], p_other.coor_[0]),
        std::min(coor_[1], p_other.coor_[1]),
        std::max(coor_[2], p_other.coor_[2]),
        std::max(coor_[3], p_other.coor_[3]));
}

std::string Rectangle::to_string() const
{
    return "Rectangle(" + std::to_string(coor_[0]) + "," + std::to_string(coor_[1]) +
           "," + std::to_string(coor_[2]) + "," + std::to_string(coor_[3]) + ")";
}

Polygon::Polygon(const std::vector<double>& p_coor)
    : coor_(p_coor)
{
}

Polygon::Polygon(const std::vector<geometry::planar::IntPoint>& p_corners)
{
    for (const auto& corner : p_corners)
    {
        coor_.push_back(static_cast<double>(corner.x));
        coor_.push_back(static_cast<double>(corner.y));
    }
}

std::vector<geometry::planar::IntPoint> Polygon::to_int_corners() const
{
    std::vector<geometry::planar::IntPoint> corners;
    for (size_t i = 0; i < coor_.size(); i += 2)
    {
        corners.emplace_back(
            static_cast<int64_t>(std::round(coor_[i])),
            static_cast<int64_t>(std::round(coor_[i + 1])));
    }
    return corners;
}

Polygon::Rectangle Polygon::bounding_box() const
{
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();

    for (size_t i = 0; i < coor_.size(); i += 2)
    {
        min_x = std::min(min_x, coor_[i]);
        max_x = std::max(max_x, coor_[i]);
        min_y = std::min(min_y, coor_[i + 1]);
        max_y = std::max(max_y, coor_[i + 1]);
    }

    return Rectangle(min_x, min_y, max_x, max_y);
}

std::string Polygon::to_string() const
{
    return "Polygon(" + std::to_string(point_count()) + " points)";
}

} // namespace specctra
} // namespace designforms
} // namespace freerouting