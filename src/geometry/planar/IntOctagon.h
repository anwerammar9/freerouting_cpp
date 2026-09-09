#pragma once

#include <string>
#include <vector>

#include "geometry/planar/Point.h"
#include "geometry/planar/RegularTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/FortyfiveDegreeDirection.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class Simplex;

class IntOctagon : public RegularTileShape
{
public:
    // Java parity: leftX, bottomY, rightX, topY, upperLeftDiagonalX,
    // lowerRightDiagonalX, lowerLeftDiagonalX, upperRightDiagonalX
    int64_t left;
    int64_t bottom;
    int64_t right;
    int64_t top;
    int64_t left_top;       // ulx  (upper-left diagonal x-intercept)
    int64_t right_bottom;   // lrx  (lower-right diagonal x-intercept)
    int64_t left_bottom;    // llx  (lower-left diagonal x-intercept)
    int64_t right_top;      // urx  (upper-right diagonal x-intercept)

    IntOctagon(int64_t p_left, int64_t p_bottom, int64_t p_right, int64_t p_top,
               int64_t p_left_top, int64_t p_right_bottom,
               int64_t p_left_bottom, int64_t p_right_top);
    IntOctagon();

    // ---- TileShape pure virtuals -------------------------------------------

    bool is_empty() const override;
    bool is_bounded() const override;
    int dimension() const override;
    int border_line_count() const override { return 8; }
    bool corner_is_bounded(int /*p_no*/) const override { return true; }
    IntPoint corner(int p_no) const override;
    FloatPoint corner_approx(int p_no) const override;
    std::vector<FloatPoint> corner_approx_arr() const override;
    Line border_line(int p_no) const override;
    int border_line_index(const Line& p_line) const override;
    bool is_IntBox() const override;
    bool is_IntOctagon() const override { return true; }
    IntBox bounding_box() const override;
    Simplex to_Simplex() const override;
    bool contains(const IntPoint& p_point) const override;
    FloatPoint centre_of_gravity() const override;
    double circumference() const override;
    std::unique_ptr<TileShape> simplify() const override;
    std::unique_ptr<TileShape> clone() const override;

    // ---- TileShape derived queries -----------------------------------------

    using TileShape::contains;

    bool is_normalized() const;
    int64_t corner_x(int p_no) const;
    int64_t corner_y(int p_no) const;
    double area() const;
    double max_width() const;
    double min_width() const;

    IntOctagon bounding_octagon() const { return *this; }

    bool contains(const FloatPoint& p_point) const;
    bool is_contained_in(const IntBox& p_box) const override;
    bool is_contained_in(const IntOctagon& p_other) const override;
    bool intersects(const IntOctagon& p_other) const;
    bool intersects(const IntBox& p_other) const;
    bool overlaps(const IntOctagon& p_other) const;

    IntOctagon intersection(const IntOctagon& p_other) const;
    IntOctagon intersection(const IntBox& p_other) const;
    IntOctagon union_with(const IntOctagon& p_other) const;
    IntOctagon union_with(const IntBox& p_other) const;

    IntOctagon bounding_tile() const { return *this; }
    std::vector<IntOctagon> cutout_from(const IntOctagon& p_d) const;
    std::vector<IntOctagon> cutout_from(const IntBox& p_d) const;
    std::vector<Simplex> cutout_from(const Simplex& p_simplex) const;

    IntOctagon translate_by(const IntVector& p_rel) const;
    IntOctagon offset(double p_distance) const;
    IntOctagon enlarge(double p_offset) const { return offset(p_offset); }

    int64_t left_x_value(int64_t p_y) const;
    int64_t right_x_value(int64_t p_y) const;
    int64_t lower_y_value(int64_t p_x) const;
    int64_t upper_y_value(int64_t p_x) const;

    Side compare(const IntOctagon& p_other, int p_edge_no) const override;
    Side compare(const IntBox& p_other, int p_edge_no) const override;
    Side side_of_border_line(int p_x, int p_y, int p_border_line_no) const;

    // ---- RegularTileShape polymorphic ops -----------------------------------

    Side compare(const RegularTileShape& p_other, int p_edge_no) const override;
    std::unique_ptr<RegularTileShape> union_with(const RegularTileShape& p_other) const override;
    bool contains(const RegularTileShape& p_other) const override;
    bool intersects(const RegularTileShape& p_other) const override;

    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const override;

    IntPoint border_point(const IntPoint& p_point, FortyfiveDegreeDirection p_dir) const;

    /**
     * Calculates the sorted p_max_result_points nearest points on the border of
     * this octagon in the 45-degree directions. p_point is assumed to be
     * located in the interior of this octagon. p_max_result_points is capped
     * at 8. Port of IntOctagon.nearest_border_projections(IntPoint, int).
     */
    std::vector<IntPoint> nearest_border_projections(const IntPoint& p_point, int p_max_result_points) const;

    bool operator==(const IntOctagon& other) const;
    std::string to_string() const;

    IntOctagon normalize() const;
    IntOctagon intersection_with_simplify(const IntOctagon& p_other) const;

    static const IntOctagon EMPTY;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting