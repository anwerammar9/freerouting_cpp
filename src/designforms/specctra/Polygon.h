#pragma once

#include <array>
#include <memory>
#include <string>

#include "geometry/planar/IntPoint.h"

namespace freerouting {
namespace designforms {
namespace specctra {

class Rectangle
{
public:
    Rectangle(double p_ll_x, double p_ll_y, double p_ur_x, double p_ur_y);
    Rectangle(const std::array<double, 4>& p_coor);

    double get_lower_left_x() const { return coor_[0]; }
    double get_lower_left_y() const { return coor_[1]; }
    double get_upper_right_x() const { return coor_[2]; }
    double get_upper_right_y() const { return coor_[3]; }

    geometry::planar::IntBox to_int_box() const;
    Rectangle union_with(const Rectangle& p_other) const;

    std::string to_string() const;

private:
    std::array<double, 4> coor_;
};

class Polygon
{
public:
    Polygon(const std::vector<double>& p_coor);
    Polygon(const std::vector<geometry::planar::IntPoint>& p_corners);

    const std::vector<double>& get_coordinates() const { return coor_; }
    int point_count() const { return static_cast<int>(coor_.size() / 2); }

    std::vector<geometry::planar::IntPoint> to_int_corners() const;

    Rectangle bounding_box() const;
    std::string to_string() const;

private:
    std::vector<double> coor_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting