#pragma once

#include <memory>
#include <vector>
#include <string>

#include "geometry/planar/Point.h"

namespace freerouting {
namespace designforms {
namespace specctra {

class Circle
{
public:
    Circle(double p_radius, double p_center_x, double p_center_y);
    Circle(double p_radius, const geometry::planar::IntPoint& p_center);

    double get_radius() const { return coor_[0]; }
    double get_center_x() const { return coor_[1]; }
    double get_center_y() const { return coor_[2]; }

    geometry::planar::IntPoint get_center() const;

    std::string to_string() const;

private:
    std::array<double, 3> coor_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting