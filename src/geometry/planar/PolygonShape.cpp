#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polygon.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/FloatLine.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <random>
#include <vector>

namespace freerouting::geometry::planar {

namespace {

// Replicate java.util.Random with the same seed and sequence to match the
// decomposition produced by the Java PolygonShape.split_to_convex_recu.
struct JavaRandomFixed
{
    uint64_t seed;

    explicit JavaRandomFixed(uint64_t p_seed = 99)
    {
        reset(p_seed);
    }

    void reset(uint64_t p_seed = 99)
    {
        // seed = (initial ^ 0x5DEECE66DL) as Java does
        seed = static_cast<uint64_t>(static_cast<int64_t>(p_seed) ^ 0x5DEECE66DLL) & 0xFFFFFFFFFFFFULL;
    }

    int next_int(int bound)
    {
        if (bound <= 0)
        {
            return 0;
        }
        if ((bound & (bound - 1)) == 0)
        {
            advance();
            return static_cast<int>((static_cast<uint64_t>(bound) * next_bits(31)) >> 31);
        }
        for (;;)
        {
            advance();
            uint32_t bits = next_bits(31);
            uint32_t val = bits % static_cast<uint32_t>(bound);
            if (static_cast<int32_t>(bits) - static_cast<int32_t>(val) + (bound - 1) >= 0)
            {
                return static_cast<int>(val);
            }
        }
    }

private:
    void advance()
    {
        seed = (seed * 0x5DEECE66ULL + 0xBULL) & 0xFFFFFFFFFFFFULL;
    }

    uint32_t next_bits(int bits)
    {
        return static_cast<uint32_t>(seed >> (48 - bits));
    }
};

// Caching a static instance per thread-local RNG avoids pollution of callers.
static thread_local JavaRandomFixed g_java_random{99};

// Division point: axis-aligned split through a concave corner.
struct DivisionPoint
{
    int corner_no_after_projection;
    FloatPoint projection;
    bool found;

    DivisionPoint(const std::vector<IntPoint>& corners, int concave_corner_no)
        : corner_no_after_projection(0), projection(0.0, 0.0), found(false)
    {
        int n = static_cast<int>(corners.size());
        FloatPoint concave_corner = corners[concave_corner_no].to_float();
        FloatPoint before_concave_corner =
            (concave_corner_no != 0)
                ? corners[concave_corner_no - 1].to_float()
                : corners[n - 1].to_float();
        FloatPoint after_concave_corner =
            (concave_corner_no == n - 1)
                ? corners[0].to_float()
                : corners[concave_corner_no + 1].to_float();

        bool search_right = before_concave_corner.y > concave_corner.y
                         || concave_corner.y > after_concave_corner.y;
        bool search_left  = before_concave_corner.y < concave_corner.y
                         || concave_corner.y < after_concave_corner.y;
        bool search_up    = before_concave_corner.x < concave_corner.x
                         || concave_corner.x < after_concave_corner.x;
        bool search_down  = before_concave_corner.x > concave_corner.x
                         || concave_corner.x > after_concave_corner.x;

        double min_projection_dist = static_cast<double>(std::numeric_limits<int>::max());
        FloatPoint min_projection(0.0, 0.0);
        int corner_no_after_min_projection = 0;

        int corner_no_after_curr_projection = (concave_corner_no + 2) % n;
        IntPoint corner_before_curr_projection =
            (corner_no_after_curr_projection != 0)
                ? corners[corner_no_after_curr_projection - 1]
                : corners[n - 1];
        FloatPoint corner_before_projection_approx = corner_before_curr_projection.to_float();

        int loop_end = n - 2;
        for (int i = 0; i < loop_end; ++i)
        {
            IntPoint corner_after_curr_projection = corners[corner_no_after_curr_projection];
            FloatPoint corner_after_projection_approx = corner_after_curr_projection.to_float();

            if (corner_before_projection_approx.y != corner_after_projection_approx.y)
            {
                double min_y, max_y;
                if (corner_after_projection_approx.y > corner_before_projection_approx.y)
                {
                    min_y = corner_before_projection_approx.y;
                    max_y = corner_after_projection_approx.y;
                }
                else
                {
                    min_y = corner_after_projection_approx.y;
                    max_y = corner_before_projection_approx.y;
                }
                if (concave_corner.y >= min_y && concave_corner.y <= max_y)
                {
                    Line curr_line(corner_before_curr_projection, corner_after_curr_projection);
                    double x_intersect = curr_line.function_in_y_value_approx(concave_corner.y);
                    double curr_dist = std::abs(x_intersect - concave_corner.x);
                    bool projection_ok = curr_dist < min_projection_dist
                        && ((search_right && x_intersect > concave_corner.x && concave_corner.y <= corner_after_projection_approx.y)
                         || (search_left  && x_intersect < concave_corner.x && concave_corner.y >= corner_after_projection_approx.y));
                    if (projection_ok)
                    {
                        min_projection_dist = curr_dist;
                        corner_no_after_min_projection = corner_no_after_curr_projection;
                        min_projection = FloatPoint(x_intersect, concave_corner.y);
                    }
                }
            }

            if (corner_before_projection_approx.x != corner_after_projection_approx.x)
            {
                double min_x, max_x;
                if (corner_after_projection_approx.x > corner_before_projection_approx.x)
                {
                    min_x = corner_before_projection_approx.x;
                    max_x = corner_after_projection_approx.x;
                }
                else
                {
                    min_x = corner_after_projection_approx.x;
                    max_x = corner_before_projection_approx.x;
                }
                if (concave_corner.x >= min_x && concave_corner.x <= max_x)
                {
                    Line curr_line(corner_before_curr_projection, corner_after_curr_projection);
                    double y_intersect = curr_line.function_value_approx(concave_corner.x);
                    double curr_dist = std::abs(y_intersect - concave_corner.y);
                    bool projection_ok = curr_dist < min_projection_dist
                        && ((search_up   && y_intersect > concave_corner.y && concave_corner.x >= corner_after_projection_approx.x)
                         || (search_down && y_intersect < concave_corner.y && concave_corner.x <= corner_after_projection_approx.x));
                    if (projection_ok)
                    {
                        min_projection_dist = curr_dist;
                        corner_no_after_min_projection = corner_no_after_curr_projection;
                        min_projection = FloatPoint(concave_corner.x, y_intersect);
                    }
                }
            }

            corner_before_curr_projection = corner_after_curr_projection;
            corner_before_projection_approx = corner_after_projection_approx;
            if (corner_no_after_curr_projection == n - 1)
            {
                corner_no_after_curr_projection = 0;
            }
            else
            {
                ++corner_no_after_curr_projection;
            }
        }
        if (min_projection_dist == static_cast<double>(std::numeric_limits<int>::max()))
        {
            logger::FRLogger::instance().warn("PolygonShape::DivisionPoint: projection not found");
            found = false;
            return;
        }
        found = true;
        projection = min_projection;
        corner_no_after_projection = corner_no_after_min_projection;
    }
};

} // anonymous namespace

// --------------------------------------------------------------------------
// Constructor: normalise corners (match Java PolygonShape(Point[]) ctor)
// --------------------------------------------------------------------------

PolygonShape::PolygonShape(const Polygon& p_polygon)
    : PolygonShape(p_polygon.corner_array())
{
}

PolygonShape::PolygonShape(const std::vector<IntPoint>& p_corner_arr)
{
    if (p_corner_arr.empty())
    {
        return;
    }
    Polygon curr_polygon(p_corner_arr);
    if (curr_polygon.winding_number_after_closing() < 0)
    {
        curr_polygon = curr_polygon.revert_corners();
    }
    std::vector<IntPoint> curr_corners = curr_polygon.corner_array();

    int last_corner_no = static_cast<int>(curr_corners.size()) - 1;
    if (last_corner_no > 0)
    {
        if (curr_corners[0] == curr_corners[last_corner_no])
        {
            --last_corner_no;
        }
    }
    if (last_corner_no < 0)
    {
        return;
    }
    bool last_point_collinear = false;
    if (last_corner_no >= 2)
    {
        last_point_collinear =
            curr_corners[last_corner_no].side_of(
                curr_corners[last_corner_no - 1], curr_corners[0]) == Side::COLLINEAR;
    }
    if (last_point_collinear)
    {
        --last_corner_no;
        if (last_corner_no < 0)
        {
            return;
        }
    }

    int first_corner_no = 0;
    bool first_point_collinear = false;
    if (last_corner_no - first_corner_no >= 2)
    {
        first_point_collinear =
            curr_corners[0].side_of(
                curr_corners[1], curr_corners[last_corner_no]) == Side::COLLINEAR;
    }
    if (first_point_collinear)
    {
        ++first_corner_no;
    }

    // Search the point with the lowest y and then with the lowest x.
    int start_corner_no = first_corner_no;
    FloatPoint start_corner = curr_corners[start_corner_no].to_float();
    for (int i = first_corner_no + 1; i <= last_corner_no; ++i)
    {
        FloatPoint curr_corner = curr_corners[i].to_float();
        if (curr_corner.y < start_corner.y
            || (curr_corner.y == start_corner.y && curr_corner.x < start_corner.x))
        {
            start_corner_no = i;
            start_corner = curr_corner;
        }
    }

    int new_corner_count = last_corner_no - first_corner_no + 1;
    corners.reserve(new_corner_count);
    for (int i = start_corner_no; i <= last_corner_no; ++i)
    {
        corners.push_back(curr_corners[i]);
    }
    for (int i = first_corner_no; i < start_corner_no; ++i)
    {
        corners.push_back(curr_corners[i]);
    }
}

// --------------------------------------------------------------------------
// Shape interface
// --------------------------------------------------------------------------

IntBox PolygonShape::bounding_box() const
{
    if (corners.empty())
    {
        return IntBox::EMPTY;
    }
    int64_t min_x = std::numeric_limits<int64_t>::max();
    int64_t max_x = std::numeric_limits<int64_t>::min();
    int64_t min_y = std::numeric_limits<int64_t>::max();
    int64_t max_y = std::numeric_limits<int64_t>::min();
    for (const auto& pt : corners)
    {
        if (pt.x < min_x) min_x = pt.x;
        if (pt.x > max_x) max_x = pt.x;
        if (pt.y < min_y) min_y = pt.y;
        if (pt.y > max_y) max_y = pt.y;
    }
    return IntBox(min_x, min_y, max_x, max_y);
}

IntOctagon PolygonShape::bounding_octagon() const
{
    return bounding_box().bounding_octagon();
}

bool PolygonShape::intersects(const Shape& other) const
{
    if (auto* c = dynamic_cast<const Circle*>(&other))
        return intersects(*c);
    if (auto* s = dynamic_cast<const Simplex*>(&other))
        return intersects(*s);
    if (auto* o = dynamic_cast<const IntOctagon*>(&other))
        return intersects(*o);
    if (auto* b = dynamic_cast<const IntBox*>(&other))
        return intersects(*b);
    if (auto* p = dynamic_cast<const PolygonShape*>(&other))
    {
        const auto& this_pieces = split_to_convex();
        const auto& other_pieces = p->split_to_convex();
        for (const auto& this_piece : this_pieces)
        {
            const Simplex this_simplex = this_piece->to_Simplex();
            for (const auto& other_piece : other_pieces)
            {
                if (this_simplex.intersects(other_piece->to_Simplex()))
                {
                    return true;
                }
            }
        }
        return false;
    }
    // Unknown Shape implementations retain the inexpensive broad-phase check.
    return bounding_box().intersects(other.bounding_box());
}

bool PolygonShape::intersects(const Circle& p_circle) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        Simplex sx = piece->to_Simplex();
        if (p_circle.intersects(sx))
        {
            return true;
        }
    }
    return false;
}

bool PolygonShape::intersects(const Simplex& p_simplex) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        Simplex sx = piece->to_Simplex();
        if (sx.intersects(p_simplex))
        {
            return true;
        }
    }
    return false;
}

bool PolygonShape::intersects(const IntOctagon& p_oct) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        Simplex sx = piece->to_Simplex();
        if (sx.intersects(p_oct))
        {
            return true;
        }
    }
    return false;
}

bool PolygonShape::intersects(const IntBox& p_box) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        Simplex sx = piece->to_Simplex();
        if (sx.intersects(p_box))
        {
            return true;
        }
    }
    return false;
}

bool PolygonShape::contains(const IntPoint& point) const
{
    return !is_outside(point);
}

bool PolygonShape::contains(const FloatPoint& p_point) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        if (piece->contains(p_point))
        {
            return true;
        }
    }
    return false;
}

bool PolygonShape::contains_inside(const IntPoint& p_point) const
{
    if (contains_on_border(p_point))
    {
        return false;
    }
    return !is_outside(p_point);
}

bool PolygonShape::contains_inside(const FloatPoint& p_point) const
{
    if (contains_on_border(p_point))
    {
        return false;
    }
    return !is_outside(p_point);
}

bool PolygonShape::is_outside(const IntPoint& p_point) const
{
    const auto& pieces = split_to_convex();
    for (const auto& piece : pieces)
    {
        if (!piece->is_outside(p_point))
        {
            return false;
        }
    }
    return true;
}

bool PolygonShape::is_outside(const FloatPoint& p_point) const
{
    return !contains(p_point);
}

bool PolygonShape::contains_on_border(const IntPoint&) const
{
    // Java returns false.
    return false;
}

bool PolygonShape::contains_on_border(const FloatPoint&) const
{
    // Java returns false.
    return false;
}

bool PolygonShape::is_empty() const
{
    return corners.empty();
}

bool PolygonShape::is_bounded() const
{
    return true;
}

int PolygonShape::dimension() const
{
    if (corners.empty()) return -1;
    if (corners.size() == 1) return 0;
    if (corners.size() == 2) return 1;
    return 2;
}

bool PolygonShape::corner_is_bounded(int) const
{
    return true;
}

int PolygonShape::border_line_count() const
{
    return static_cast<int>(corners.size());
}

IntPoint PolygonShape::corner(int p_no) const
{
    if (p_no < 0 || p_no >= border_line_count())
    {
        logger::FRLogger::instance().warn("PolygonShape.corner: p_no out of range");
        return IntPoint(0, 0);
    }
    return corners[p_no];
}

Line PolygonShape::border_line(int p_no) const
{
    int n = border_line_count();
    if (p_no < 0 || p_no >= n)
    {
        logger::FRLogger::instance().warn("PolygonShape.border_line: p_no out of range");
        return Line(IntPoint(0, 0), IntPoint(1, 0));
    }
    return Line(corners[p_no], (p_no == n - 1) ? corners[0] : corners[p_no + 1]);
}

// --------------------------------------------------------------------------
// Bounding / queries
// --------------------------------------------------------------------------

IntOctagon PolygonShape::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return p_dirs.bounds(*this);
}

bool PolygonShape::is_convex() const
{
    size_t n = corners.size();
    if (n <= 2)
    {
        return true;
    }

    IntPoint prev_point = corners[n - 1];
    IntPoint curr_point = corners[0];
    IntPoint next_point = corners[1];

    for (size_t ind = 0; ind < n; ++ind)
    {
        if (next_point.side_of(prev_point, curr_point) == Side::ON_THE_RIGHT)
        {
            return false;
        }
        prev_point = curr_point;
        curr_point = next_point;
        next_point = (ind == n - 2) ? corners[0] : corners[ind + 2];
    }

    // Check that the sum of interior angles is at most 2*pi.
    Line first_line(corners[n - 1], corners[0]);
    Line curr_line(corners[0], corners[1]);
    IntVector first_direction = first_line.direction();
    double last_det = static_cast<double>(first_direction.determinant(curr_line.direction()));

    for (size_t ind = 2; ind < n; ++ind)
    {
        curr_line = Line(curr_line.b, corners[ind]);
        double curr_det = static_cast<double>(first_direction.determinant(curr_line.direction()));
        if (last_det <= 0 && curr_det > 0)
        {
            return false;
        }
        last_det = curr_det;
    }
    return true;
}

PolygonShape PolygonShape::convex_hull() const
{
    // Java's implementation only removes strict right-turn vertices, so
    // corners lying on a hull edge are kept (e.g. after removing the notch of
    // an L-shape the polygon "corner" on the hull boundary survives, making the
    // hull non-minimal and its area wrong). We compute the true convex hull
    // with Andrew's monotone chain instead.
    size_t n = corners.size();
    if (n <= 2)
    {
        return *this;
    }

    std::vector<IntPoint> sorted = corners;
    std::sort(sorted.begin(), sorted.end(),
              [](const IntPoint& a, const IntPoint& b)
              {
                  return a.x < b.x || (a.x == b.x && a.y < b.y);
              });
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.size() < 3)
    {
        return *this;
    }

    auto cross = [](const IntPoint& o, const IntPoint& a, const IntPoint& b) -> int64_t
    {
        IntVector va(a.x - o.x, a.y - o.y);
        IntVector vb(b.x - o.x, b.y - o.y);
        return va.determinant(vb);
    };

    std::vector<IntPoint> lower;
    for (const IntPoint& p : sorted)
    {
        while (lower.size() >= 2 && cross(lower[lower.size() - 2], lower.back(), p) <= 0)
        {
            lower.pop_back();
        }
        lower.push_back(p);
    }
    std::vector<IntPoint> upper;
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        while (upper.size() >= 2 && cross(upper[upper.size() - 2], upper.back(), *it) <= 0)
        {
            upper.pop_back();
        }
        upper.push_back(*it);
    }
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    if (lower.size() < 3)
    {
        return *this;
    }
    return PolygonShape(lower);
}

std::unique_ptr<TileShape> PolygonShape::bounding_tile() const
{
    PolygonShape hull = convex_hull();
    size_t n = hull.corners.size();
    std::vector<Line> bounding_lines;
    bounding_lines.reserve(n);
    for (size_t i = 0; i < n - 1; ++i)
    {
        bounding_lines.emplace_back(hull.corners[i], hull.corners[i + 1]);
    }
    bounding_lines.emplace_back(hull.corners[n - 1], hull.corners[0]);
    return TileShape::get_instance(bounding_lines);
}

double PolygonShape::area() const
{
    // Java's guard `dimension() <= 2` is a no-op for polygons (dimension() is
    // 2 for any 3+-corner polygon), so area() always returned 0 there.
    // We use a correct shoelace guard instead.
    if (corners.size() < 3)
    {
        return 0.0;
    }
    double result = 0.0;
    size_t n = corners.size();
    FloatPoint prev_corner = corners[n - 2].to_float();
    FloatPoint curr_corner = corners[n - 1].to_float();
    for (size_t i = 0; i < n; ++i)
    {
        FloatPoint next_corner = corners[i].to_float();
        result += curr_corner.x * (next_corner.y - prev_corner.y);
        prev_corner = curr_corner;
        curr_corner = next_corner;
    }
    return 0.5 * std::abs(result);
}

double PolygonShape::distance(const FloatPoint& p_point) const
{
    const auto& pieces = split_to_convex();
    double min_dist = std::numeric_limits<double>::max();
    for (const auto& piece : pieces)
    {
        if (piece->contains(p_point))
        {
            return 0.0;
        }
        auto np = piece->nearest_point_approx(p_point);
        if (np)
        {
            double d = np->distance(p_point);
            if (d < min_dist)
            {
                min_dist = d;
            }
        }
    }
    if (min_dist == std::numeric_limits<double>::max())
    {
        return 0.0;
    }
    return min_dist;
}

double PolygonShape::border_distance(const FloatPoint& p_point) const
{
    const auto& pieces = split_to_convex();
    double min_dist = std::numeric_limits<double>::max();
    for (const auto& piece : pieces)
    {
        double d = piece->border_distance(p_point);
        if (d < min_dist)
        {
            min_dist = d;
        }
    }
    if (min_dist == std::numeric_limits<double>::max())
    {
        return 0.0;
    }
    return min_dist;
}

double PolygonShape::smallest_radius() const
{
    return border_distance(centre_of_gravity());
}

std::optional<FloatPoint> PolygonShape::nearest_point_approx(const FloatPoint& p_from_point) const
{
    const auto& pieces = split_to_convex();
    double min_dist_square = std::numeric_limits<double>::max();
    std::optional<FloatPoint> result;
    for (const auto& piece : pieces)
    {
        auto np = piece->nearest_point_approx(p_from_point);
        if (np)
        {
            double d2 = np->distance_square(p_from_point);
            if (d2 < min_dist_square)
            {
                min_dist_square = d2;
                result = *np;
            }
        }
    }
    return result;
}

std::unique_ptr<Shape> PolygonShape::enlarge(double p_offset) const
{
    if (p_offset == 0 || corners.size() < 3 || !is_convex())
    {
        return std::make_unique<PolygonShape>(*this);
    }

    double signed_area = 0.0;
    for (std::size_t index = 0; index < corners.size(); ++index)
    {
        const IntPoint& current = corners[index];
        const IntPoint& next = corners[(index + 1) % corners.size()];
        signed_area += static_cast<double>(current.x) * next.y
            - static_cast<double>(next.x) * current.y;
    }

    const double outward_offset = signed_area >= 0 ? -p_offset : p_offset;
    std::vector<Line> offset_lines;
    offset_lines.reserve(corners.size());
    for (std::size_t index = 0; index < corners.size(); ++index)
    {
        const IntPoint& current = corners[index];
        const IntPoint& next = corners[(index + 1) % corners.size()];
        offset_lines.push_back(Line(current, next).translate(outward_offset));
    }

    std::vector<IntPoint> enlarged_corners;
    enlarged_corners.reserve(offset_lines.size());
    for (std::size_t index = 0; index < offset_lines.size(); ++index)
    {
        const Line& previous = offset_lines[(index + offset_lines.size() - 1) % offset_lines.size()];
        const auto intersection = previous.intersection(offset_lines[index]);
        if (!intersection)
        {
            return std::make_unique<PolygonShape>(*this);
        }
        enlarged_corners.push_back(intersection->to_float().round());
    }
    return std::make_unique<PolygonShape>(enlarged_corners);
}

std::vector<std::unique_ptr<Polyline>> PolygonShape::cutout(const Polyline&) const
{
    logger::FRLogger::instance().warn("PolygonShape.cutout not yet implemented");
    return {};
}

// --------------------------------------------------------------------------
// Transforms
// --------------------------------------------------------------------------

PolygonShape PolygonShape::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& pt : corners)
    {
        new_corners.push_back(pt.turn_90_degree(p_factor, p_pole));
    }
    return PolygonShape(new_corners);
}

PolygonShape PolygonShape::rotate_approx(double p_angle, const FloatPoint& p_pole) const
{
    if (p_angle == 0)
    {
        return *this;
    }
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& pt : corners)
    {
        new_corners.push_back(pt.to_float().rotate(p_angle, p_pole).round());
    }
    return PolygonShape(new_corners);
}

PolygonShape PolygonShape::translate_by(const Vector& p_vector) const
{
    const auto* iv = dynamic_cast<const IntVector*>(&p_vector);
    if (iv == nullptr || (iv->x == 0 && iv->y == 0))
    {
        return *this;
    }
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& pt : corners)
    {
        new_corners.push_back(IntPoint(pt.x + iv->x, pt.y + iv->y));
    }
    return PolygonShape(new_corners);
}

PolygonShape PolygonShape::mirror_horizontal(const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& pt : corners)
    {
        new_corners.push_back(pt.mirror_horizontal(p_pole));
    }
    return PolygonShape(new_corners);
}

PolygonShape PolygonShape::mirror_vertical(const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& pt : corners)
    {
        new_corners.push_back(pt.mirror_vertical(p_pole));
    }
    return PolygonShape(new_corners);
}

// --------------------------------------------------------------------------
// Convex split
// --------------------------------------------------------------------------

const std::vector<std::unique_ptr<TileShape>>& PolygonShape::split_to_convex() const
{
    if (!precalculated_convex_pieces)
    {
        g_java_random.reset(99);
        precalculated_convex_pieces =
            std::make_shared<std::vector<std::unique_ptr<TileShape>>>(split_to_convex_recu());
    }
    return *precalculated_convex_pieces;
}

// --------------------------------------------------------------------------

std::vector<std::unique_ptr<TileShape>> PolygonShape::split_to_convex_recu() const
{
    int n = static_cast<int>(corners.size());
    if (n == 0)
    {
        return {};
    }

    int start_corner_no = g_java_random.next_int(n);
    IntPoint curr_corner = corners[start_corner_no];
    IntPoint prev_corner =
        (start_corner_no != 0) ? corners[start_corner_no - 1] : corners[n - 1];

    int concave_corner_no = -1;
    for (int i = 0; i < n; ++i)
    {
        IntPoint next_corner =
            (start_corner_no < n - 1) ? corners[start_corner_no + 1] : corners[0];
        if (next_corner.side_of(prev_corner, curr_corner) == Side::ON_THE_RIGHT)
        {
            concave_corner_no = start_corner_no;
            break;
        }
        prev_corner = curr_corner;
        curr_corner = next_corner;
        start_corner_no = (start_corner_no + 1) % n;
    }

    std::vector<std::unique_ptr<TileShape>> result;
    if (concave_corner_no < 0)
    {
        result.push_back(TileShape::get_instance(corners));
        return result;
    }

    DivisionPoint d(corners, concave_corner_no);
    if (!d.found)
    {
        return {};
    }

    IntPoint projection_rounded = d.projection.round();

    // First piece
    int corner_count = d.corner_no_after_projection - concave_corner_no;
    if (corner_count < 0) corner_count += n;
    ++corner_count;
    std::vector<IntPoint> first_arr;
    first_arr.reserve(corner_count);
    int corner_ind = concave_corner_no;
    for (int i = 0; i < corner_count - 1; ++i)
    {
        first_arr.push_back(corners[corner_ind]);
        corner_ind = (corner_ind + 1) % n;
    }
    first_arr.push_back(projection_rounded);
    PolygonShape first_piece(first_arr);

    // Last piece
    corner_count = concave_corner_no - d.corner_no_after_projection;
    if (corner_count < 0) corner_count += n;
    corner_count += 2;
    std::vector<IntPoint> last_arr;
    last_arr.reserve(corner_count);
    last_arr.push_back(projection_rounded);
    corner_ind = d.corner_no_after_projection;
    for (int i = 1; i < corner_count; ++i)
    {
        last_arr.push_back(corners[corner_ind]);
        corner_ind = (corner_ind + 1) % n;
    }
    PolygonShape last_piece(last_arr);

    auto c1 = first_piece.split_to_convex_recu();
    auto c2 = last_piece.split_to_convex_recu();

    if (c1.empty() || c2.empty())
    {
        return {};
    }
    result.reserve(c1.size() + c2.size());
    for (auto& p : c1) result.push_back(std::move(p));
    for (auto& p : c2) result.push_back(std::move(p));
    return result;
}

// --------------------------------------------------------------------------
// to_string
// --------------------------------------------------------------------------

std::string PolygonShape::to_string() const
{
    std::ostringstream ss;
    ss << "PolygonShape[";
    for (size_t i = 0; i < corners.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << corners[i].to_string();
    }
    ss << "]";
    return ss.str();
}

} // namespace freerouting::geometry::planar