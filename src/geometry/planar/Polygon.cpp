#include "geometry/planar/Polygon.h"

#include <cmath>
#include <vector>

#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

namespace freerouting::geometry::planar {

namespace {

constexpr double PI = 3.14159265358979323846;

} // namespace

Polygon::Polygon(const std::vector<IntPoint>& p_point_arr)
    : corners_(p_point_arr)
{
    if (corners_.empty())
    {
        return;
    }

    bool corner_removed = true;
    while (corner_removed)
    {
        corner_removed = false;

        // remove multiple points
        std::vector<IntPoint> dedup_corners;
        dedup_corners.reserve(corners_.size());
        bool multiple_point_removed = false;
        for (const IntPoint& corner : corners_)
        {
            if (dedup_corners.empty() || !(dedup_corners.back() == corner))
            {
                dedup_corners.emplace_back(corner.x, corner.y);
            }
            else
            {
                multiple_point_removed = true;
            }
        }
        if (multiple_point_removed)
        {
            corners_ = std::move(dedup_corners);
            corner_removed = true;
        }

        // remove points which are collinear with the previous and next point.
        // Java parity: only interior points are checked (no wrap-around at the ends) and only one
        // point is removed per pass before the loop starts over.
        for (size_t k = 1; k + 1 < corners_.size(); ++k)
        {
            if (corners_[k].side_of(corners_[k - 1], corners_[k + 1]) == Side::COLLINEAR)
            {
                corners_.erase(corners_.begin() + static_cast<std::ptrdiff_t>(k));
                corner_removed = true;
                break;
            }
        }
    }
}

std::vector<IntPoint> Polygon::corner_array() const
{
    return corners_;
}

Polygon Polygon::revert_corners() const
{
    std::vector<IntPoint> corner_arr = corner_array();
    std::vector<IntPoint> reverse_corner_arr(corner_arr.rbegin(), corner_arr.rend());
    return Polygon(reverse_corner_arr);
}

int Polygon::winding_number_after_closing() const
{
    std::vector<IntPoint> corner_arr = corner_array();
    if (corner_arr.size() < 2)
    {
        return 0;
    }
    int corner_count = static_cast<int>(corner_arr.size());
    // Skip the last corner, if it is equal to the first corner.
    if (corner_arr[0] == corner_arr[corner_count - 1])
    {
        --corner_count;
    }
    if (corner_count < 2)
    {
        return 0;
    }
    double angle_sum = 0;
    for (int i = 0; i < corner_count; ++i)
    {
        int i_minus = (i == 0) ? corner_count - 1 : i - 1;
        int i_plus = (i + 1) % corner_count;
        IntVector prev_side_vector(corner_arr[i].x - corner_arr[i_minus].x,
                                   corner_arr[i].y - corner_arr[i_minus].y);
        IntVector next_side_vector(corner_arr[i_plus].x - corner_arr[i].x,
                                   corner_arr[i_plus].y - corner_arr[i].y);
        angle_sum += prev_side_vector.angle_approx(next_side_vector);
    }
    angle_sum /= (2.0 * PI);
    if (std::abs(angle_sum) < 0.5)
    {
        logger::FRLogger::instance().warn(
            "Polygon.winding_number_after_closing: winding number != 0 expected");
    }
    return static_cast<int>(std::lround(angle_sum));
}

} // namespace freerouting::geometry::planar