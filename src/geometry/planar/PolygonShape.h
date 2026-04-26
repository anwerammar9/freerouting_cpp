#pragma once

#include <vector>

#include "geometry/planar/Shape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::geometry::planar {

class PolygonShape : public Shape
{
public:
    std::vector<IntPoint> corners;

    PolygonShape() = default;
    explicit PolygonShape(const std::vector<IntPoint>& p_corners);

    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const;
    bool intersects(const Shape& other) const override;
    bool contains(const IntPoint& point) const override;
    bool corner_is_bounded(int p_no) const;
    int border_line_count() const { return static_cast<int>(corners.size()); }
    IntPoint corner(int p_no) const;

    std::string to_string() const override;
};

} // namespace freerouting::geometry::planar