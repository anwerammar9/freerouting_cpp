#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "geometry/planar/Shape.h"
#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Vector;

/**
 * An area whose outside border curve and hole borders consist of straight
 * lines. Port of the Java PolylineArea (geometry/planar/PolylineArea.java).
 */
class PolylineArea : public Area
{
public:
    PolylineArea(std::shared_ptr<PolylineShape> p_border_shape,
                 std::vector<std::shared_ptr<PolylineShape>> p_hole_arr = {});

    // ---- Shape / Area interface overrides ----
    IntBox bounding_box() const override;
    IntOctagon bounding_octagon() const override;
    bool contains(const IntPoint& p_point) const override;
    bool is_empty() const override;
    std::unique_ptr<Area> clone() const override;
    bool intersects(const Shape& p_other) const override;
    std::string to_string() const override;

    // ---- Java PolylineArea surface (Area interface overrides) ----
    int dimension() const override;
    bool is_bounded() const override;
    bool is_contained_in(const IntBox& p_box) const override;
    const std::shared_ptr<PolylineShape>& get_border() const;
    const std::vector<std::shared_ptr<PolylineShape>>& get_holes() const;
    bool contains(const FloatPoint& p_point) const override;
    std::optional<FloatPoint> nearest_point_approx(const FloatPoint& p_from_point) const override;
    std::vector<FloatPoint> corner_approx_arr() const override;
    const std::vector<std::unique_ptr<TileShape>>& split_to_convex() const override;
    std::unique_ptr<Area> translate_by(const Vector& p_vector) const override;
    std::unique_ptr<Area> turn_90_degree(int p_factor, const IntPoint& p_pole) const override;
    std::unique_ptr<Area> rotate_approx(double p_angle, const FloatPoint& p_pole) const override;
    std::unique_ptr<Area> mirror_vertical(const IntPoint& p_pole) const override;
    std::unique_ptr<Area> mirror_horizontal(const IntPoint& p_pole) const override;

private:
    static void cutout_hole_piece(const TileShape& p_divide_piece,
                                  const TileShape& p_hole_piece,
                                  std::vector<std::unique_ptr<TileShape>>& p_result_pieces);

    std::shared_ptr<PolylineShape> border_shape_;
    std::vector<std::shared_ptr<PolylineShape>> hole_arr_;
    mutable std::shared_ptr<std::vector<std::unique_ptr<TileShape>>> precalculated_convex_pieces_;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
