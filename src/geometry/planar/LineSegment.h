#pragma once

#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"

namespace freerouting {
namespace geometry {
namespace planar {

class LineSegment
{
public:
    LineSegment(const IntPoint& p_start, const IntPoint& p_end);

    const IntPoint& start_point() const { return start_; }
    const IntPoint& end_point() const { return end_; }

    Line supporting_line() const;
    IntBox bounding_box() const;
    bool contains(const IntPoint& p_point) const;
    bool intersects(const LineSegment& p_other) const;

private:
    IntPoint start_;
    IntPoint end_;

    static int orientation(const IntPoint& p, const IntPoint& q, const IntPoint& r);
    static bool on_segment(const IntPoint& p, const IntPoint& q, const IntPoint& r);
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
