#include "designforms/specctra/Circle.h"

namespace freerouting {
namespace designforms {
namespace specctra {

Circle::Circle(double p_radius, double p_center_x, double p_center_y)
{
    coor_ = {{p_radius, p_center_x, p_center_y}};
}

Circle::Circle(double p_radius, const geometry::planar::IntPoint& p_center)
{
    coor_ = {{p_radius, static_cast<double>(p_center.x), static_cast<double>(p_center.y)}};
}

geometry::planar::IntPoint Circle::get_center() const
{
    return geometry::planar::IntPoint(
        static_cast<int64_t>(std::round(coor_[1])),
        static_cast<int64_t>(std::round(coor_[2])));
}

std::string Circle::to_string() const
{
    return "Circle(r=" + std::to_string(coor_[0]) +
           ", center=(" + std::to_string(coor_[1]) + "," +
           std::to_string(coor_[2]) + "))";
}

} // namespace specctra
} // namespace designforms
} // namespace freerouting