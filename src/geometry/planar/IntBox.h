#pragma once

#include <string>
#include <vector>

#include "geometry/planar/Point.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntOctagon
{
public:
    const int64_t right;
    const int64_t top;
    const int64_t left;
    const int64_t bottom;
    const int64_t right_bottom;
    const int64_t left_top;
    const int64_t right_top;
    const int64_t left_bottom;

    IntOctagon(int64_t p_right, int64_t p_top, int64_t p_left, int64_t p_bottom,
               int64_t p_right_bottom, int64_t p_left_top, int64_t p_right_top, int64_t p_left_bottom);

    IntOctagon();

    bool contains(const IntPoint& p_point) const;
    IntBox bounding_box() const;
    std::string to_string() const;
};

class IntBox
{
public:
    const IntPoint ll;
    const IntPoint ur;

    IntBox(int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y);
    IntBox(const IntPoint& p_ll, const IntPoint& p_ur);
    IntBox();

    bool contains(const IntPoint& p_point) const;
    bool contains(const IntBox& p_box) const;
    bool intersects(const IntBox& p_box) const;
    bool intersects_approx(const IntBox& p_box, double p_precision) const;

    IntPoint center() const;
    int64_t width() const;
    int64_t height() const;
    int64_t area() const;

    IntBox intersection(const IntBox& p_box) const;
    IntBox union_with(const IntBox& p_box) const;

    bool operator==(const IntBox& other) const;
    IntOctagon bounding_octagon() const;
    std::string to_string() const;

    static const IntBox EMPTY;
    static const IntBox INFINITE;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting