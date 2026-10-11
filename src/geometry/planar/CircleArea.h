#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Vector.h"

namespace freerouting::geometry::planar {

/**
 * A Circle used as an Area. In Java Circle implements ConvexShape, which extends Shape, which extends
 * Area; in C++ Area derives from Shape and Circle is a plain Shape, so circular keepouts and similar
 * areas need this adapter. Every method delegates to the Java Circle behaviour (split_to_convex returns
 * the bounding tile, like Java Circle.splitToConvex).
 */
class CircleArea : public Area
{
public:
    explicit CircleArea(const Circle& p_circle) : circle_(p_circle) {}

    const Circle& circle() const { return circle_; }

    IntBox bounding_box() const override { return circle_.bounding_box(); }
    IntOctagon bounding_octagon() const override { return circle_.bounding_octagon(); }
    bool contains(const IntPoint& p_point) const override { return circle_.contains(p_point); }
    bool contains(const FloatPoint& p_point) const override { return circle_.contains(p_point); }
    bool is_empty() const override { return circle_.is_empty(); }
    bool is_bounded() const override { return circle_.is_bounded(); }
    int dimension() const override { return circle_.dimension(); }
    bool is_contained_in(const IntBox& p_box) const override { return circle_.is_contained_in(p_box); }
    std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const override
    {
        return circle_.nearest_point_approx(p_from_point);
    }
    std::vector<FloatPoint> corner_approx_arr() const override { return circle_.corner_approx_arr(); }
    const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const override;
    std::unique_ptr<Area> clone() const override { return std::make_unique<CircleArea>(circle_); }

    std::unique_ptr<Area> turn_90_degree(int p_factor, const IntPoint& p_pole) const override
    {
        return std::make_unique<CircleArea>(circle_.turn_90_degree(p_factor, p_pole));
    }
    std::unique_ptr<Area> rotate_approx(double p_angle, const FloatPoint& p_pole) const override
    {
        return std::make_unique<CircleArea>(circle_.rotate_approx(p_angle, p_pole));
    }
    std::unique_ptr<Area> translate_by(const Vector& p_vector) const override
    {
        return std::make_unique<CircleArea>(circle_.translate_by(p_vector));
    }
    std::unique_ptr<Area> mirror_horizontal(const IntPoint& p_pole) const override
    {
        return std::make_unique<CircleArea>(circle_.mirror_horizontal(p_pole));
    }
    std::unique_ptr<Area> mirror_vertical(const IntPoint& p_pole) const override
    {
        return std::make_unique<CircleArea>(circle_.mirror_vertical(p_pole));
    }

    bool intersects(const Shape& p_other) const override { return circle_.intersects(p_other); }
    std::string to_string() const override { return circle_.to_string(); }

private:
    Circle circle_;
    mutable std::shared_ptr<std::vector<std::unique_ptr<TileShape>>> convex_pieces_;
};

} // namespace freerouting::geometry::planar
