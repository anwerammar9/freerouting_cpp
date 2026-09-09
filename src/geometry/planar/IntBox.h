#pragma once

#include <string>
#include <vector>

#include "geometry/planar/Point.h"
#include "geometry/planar/RegularTileShape.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntOctagon;
class Simplex;
class Circle;

class IntBox : public RegularTileShape
{
public:
    IntPoint ll;
    IntPoint ur;

    IntBox(int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y);
    IntBox(const IntPoint& p_ll, const IntPoint& p_ur);
    IntBox();

    // ---- TileShape pure virtuals -------------------------------------------

    bool is_empty() const override;
    bool is_bounded() const override;
    int dimension() const override;
    int border_line_count() const override { return 4; }
    bool corner_is_bounded(int /*p_no*/) const override { return true; }
    IntPoint corner(int p_no) const override;
    FloatPoint corner_approx(int p_no) const override;
    std::vector<FloatPoint> corner_approx_arr() const override;
    Line border_line(int p_no) const override;
    int border_line_index(const Line& p_line) const override;
    bool is_IntBox() const override { return true; }
    bool is_IntOctagon() const override { return true; }
    IntBox bounding_box() const override { return *this; }
    Simplex to_Simplex() const override;
    bool contains(const IntPoint& p_point) const override;
    FloatPoint centre_of_gravity() const override;
    double circumference() const override { return 2.0 * (width() + height()); }
    std::unique_ptr<TileShape> simplify() const override;
    std::unique_ptr<TileShape> clone() const override;

    // ---- TileShape derived queries -----------------------------------------

    using TileShape::contains;

    bool contains(const IntBox& p_box) const;
    bool intersects(const IntBox& p_box) const;
    bool intersects_approx(const IntBox& p_box, double p_precision) const;
    bool intersects(const IntOctagon& p_oct) const;
    bool intersects(const Simplex& p_simplex) const;
    bool overlaps(const IntBox& p_box) const;

    IntPoint center() const;
    int64_t width() const;
    int64_t height() const;

    double max_width() const;
    double min_width() const;
    double area() const;

    IntBox intersection(const IntBox& p_box) const;
    IntBox union_with(const IntBox& p_box) const;

    IntBox bounding_tile() const { return *this; }
    bool is_contained_in(const IntBox& p_other) const override;
    bool is_contained_in(const IntOctagon& p_other) const override;
    bool contains_in_interior(const IntBox& p_other) const;
    IntBox nearest_part(const IntBox& p_from_box) const;
    double weighted_distance(const IntBox& p_other, double p_horizontal_weight, double p_vertical_weight) const;
    Side compare(const IntBox& p_other, int p_edge_no) const override;

    /**
     * Calculates the nearest point of this box to p_from_point.
     * Port of IntBox.nearest_point(FloatPoint).
     */
    FloatPoint nearest_point(const FloatPoint& p_from_point) const;

    /**
     * Calculates the sorted p_max_result_points nearest points on the border of
     * this box. p_point is assumed to be located in the interior of this box.
     * The function is only implemented for p_max_result_points <= 2.
     * Port of IntBox.nearest_border_projections(IntPoint, int).
     */
    std::vector<IntPoint> nearest_border_projections(const IntPoint& p_point, int p_max_result_points) const;

    /**
     * Calculates the distance of this box to p_from_point.
     * Port of IntBox.distance(FloatPoint).
     */
    double distance(const FloatPoint& p_from_point) const;

    /**
     * Returns true if this box intersects p_other.
     * Port of IntBox.intersects(Circle): delegates to Circle.intersects(IntBox).
     */
    bool intersects(const Circle& p_other) const;

    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const override;

    // ---- RegularTileShape polymorphic ops -----------------------------------

    Side compare(const RegularTileShape& p_other, int p_edge_no) const override;
    std::unique_ptr<RegularTileShape> union_with(const RegularTileShape& p_other) const override;
    bool contains(const RegularTileShape& p_other) const override;
    bool intersects(const RegularTileShape& p_other) const override;

    // ---- cross type ops on the concrete operand types -----------------------

    Side compare(const IntOctagon& p_other, int p_edge_no) const override;
    std::unique_ptr<IntOctagon> union_with(const IntOctagon& p_other) const;

    std::vector<IntBox> divide_into_sections(double p_max_section_width) const;

    std::vector<IntBox> cutout_from(const IntBox& p_d) const;
    std::vector<Simplex> cutout_from(const Simplex& p_simplex) const;
    std::vector<IntOctagon> cutout_from(const IntOctagon& p_oct) const;

    bool operator==(const IntBox& other) const;
    IntOctagon bounding_octagon() const;
    IntOctagon to_IntOctagon() const;
    std::string to_string() const;

    // ---- mutable shape operations ------------------------------------------

    IntBox translate_by(const IntVector& p_rel) const;
    IntBox offset(double p_dist) const;
    IntOctagon enlarge(double p_offset) const;
    IntBox horizontal_offset(double p_dist) const;
    IntBox vertical_offset(double p_dist) const;
    IntBox shrink(int p_width) const;
    IntBox turn_90_degree(int p_factor, const IntPoint& p_pole) const;

    static const IntBox EMPTY;
    static const IntBox INFINITE;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting