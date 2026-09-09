#pragma once

#include <string>
#include <vector>
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace geometry {
namespace planar {

class IntOctagon;
class ShapeBoundingDirections;

class Simplex : public TileShape
{
public:
    std::vector<Line> lines;

    explicit Simplex(const std::vector<Line>& p_lines);
    Simplex(const Simplex&) = default;
    Simplex& operator=(const Simplex&) = default;
    Simplex& operator=(Simplex&&) = default;

    static Simplex get_instance(std::vector<Line> p_line_arr);

    using TileShape::contains;

    bool is_empty() const override;
    bool is_bounded() const override;
    bool corner_is_bounded(int p_no) const override;
    int dimension() const override;
    int border_line_count() const override { return static_cast<int>(lines.size()); }
    Line border_line(int p_no) const override;
    Line& mutable_border_line(int p_no);

    IntPoint corner(int p_no) const override;
    FloatPoint corner_approx(int p_no) const override;
    std::vector<FloatPoint> corner_approx_arr() const override;

    bool contains(const IntPoint& p_point) const override;
    FloatPoint centre_of_gravity() const override;
    double circumference() const override;
    std::unique_ptr<TileShape> simplify() const override;
    std::unique_ptr<TileShape> clone() const override;

    bool is_IntBox() const override;
    bool is_IntOctagon() const override;
    IntBox bounding_box() const override;
    Simplex bounding_tile() const { return *this; }
    IntOctagon bounding_octagon() const;
    IntOctagon to_IntOctagon() const;
    Simplex to_Simplex() const override { return *this; }

    double max_width() const;
    double min_width() const;

    Simplex translate_by(const IntVector& p_vector) const;
    Simplex offset(double p_width) const;
    Simplex enlarge(double p_offset) const;

    bool intersects(const Simplex& p_other) const;
    bool intersects(const IntBox& p_box) const;
    bool intersects(const IntOctagon& p_oct) const;

    Simplex intersection(const IntBox& p_box) const;
    Simplex intersection(const Simplex& p_other) const;
    Simplex intersection(const IntOctagon& p_other) const;

    int border_line_index(const Line& p_line) const override;
    Simplex remove_border_line(int p_no) const;

    int index_of_right_most_corner(const IntPoint& p_from_point) const;

    Simplex remove_redundant_lines() const;

    /**
     * Cuts this simplex out of p_outer_simplex. Divides the resulting shape into
     * simplices along the minimal distance lines from the vertices of the inner
     * simplex to the outer simplex. Returns the convex pieces constructed by this
     * division.
     * Returns an empty vector if this simplex has dimension < 2.
     */
    std::vector<Simplex> cutout_from(const Simplex& p_outer_simplex) const;
    std::vector<Simplex> cutout_from(const IntOctagon& p_oct) const;
    std::vector<Simplex> cutout_from(const IntBox& p_box) const;

    /**
     * Calculates a surrounding regular tile shape (as IntOctagon) with the fixed
     * directions of p_dirs.
     */
    IntOctagon bounding_shape(const ShapeBoundingDirections& p_dirs) const;

    std::string to_string() const;

    static const Simplex EMPTY;

private:
    /**
     * For each corner of this inner simplex 1 or 2 perpendicular projections onto
     * lines of the outer simplex are constructed, so that the resulting pieces after
     * cutting out the inner simplex are convex. 2 projections may be necessary at
     * sharp angle corners. Used by cutout_from. Returns an empty vector on failure.
     */
    std::vector<Line> calc_division_lines(int p_inner_corner_no, const Simplex& p_outer_simplex) const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
