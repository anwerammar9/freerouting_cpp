#include "geometry/planar/TileShape.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "geometry/planar/Direction.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace geometry {
namespace planar {

bool TileShape::is_outside(const IntPoint& p_point) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return true;
    }
    for (int i = 0; i < line_count; i++)
    {
        if (border_line(i).side_of(p_point) == Side::ON_THE_LEFT)
        {
            return true;
        }
    }
    return false;
}

bool TileShape::contains_inside(const IntPoint& p_point) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return false;
    }
    for (int i = 0; i < line_count; i++)
    {
        if (border_line(i).side_of(p_point) != Side::ON_THE_RIGHT)
        {
            return false;
        }
    }
    return true;
}

bool TileShape::contains(const FloatPoint& p_point) const
{
    return contains(p_point, 0);
}

bool TileShape::contains(const FloatPoint& p_point, double p_tolerance) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return false;
    }
    for (int i = 0; i < line_count; i++)
    {
        if (border_line(i).side_of(p_point, p_tolerance) != Side::ON_THE_RIGHT)
        {
            return false;
        }
    }
    return true;
}

Side TileShape::side_of_border(const FloatPoint& p_point, double p_tolerance) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return Side::COLLINEAR;
    }
    Side result = Side::ON_THE_RIGHT; // point is inside
    for (int i = 0; i < line_count; i++)
    {
        Side curr_side = border_line(i).side_of(p_point, p_tolerance);
        if (curr_side == Side::ON_THE_LEFT)
        {
            return Side::ON_THE_LEFT; // point is outside
        }
        if (curr_side == Side::COLLINEAR)
        {
            result = curr_side;
        }
    }
    return result;
}

int TileShape::contains_on_border_line_no(const IntPoint& p_point) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return -1;
    }
    int containing_line_no = -1;
    for (int i = 0; i < line_count; i++)
    {
        Side side_of = border_line(i).side_of(p_point);
        if (side_of == Side::ON_THE_LEFT)
        {
            // p_point outside the convex shape
            return -1;
        }
        if (side_of == Side::COLLINEAR)
        {
            containing_line_no = i;
        }
    }
    return containing_line_no;
}

bool TileShape::contains_on_border(const IntPoint& p_point) const
{
    return contains_on_border_line_no(p_point) >= 0;
}

double TileShape::area() const
{
    if (!is_bounded())
    {
        return std::numeric_limits<double>::max();
    }
    if (dimension() < 2)
    {
        return 0;
    }
    // calculate half of the absolute value of
    // x0 (y1 - yn-1) + x1 (y2 - y0) + x2 (y3 - y1) + ...+ xn-1( y0 - yn-2)
    double result = 0;
    int corner_count = border_line_count();
    FloatPoint prev_corner = corner_approx(corner_count - 2);
    FloatPoint curr_corner = corner_approx(corner_count - 1);
    for (int i = 0; i < corner_count; i++)
    {
        FloatPoint next_corner = corner_approx(i);
        result += curr_corner.x * (next_corner.y - prev_corner.y);
        prev_corner = curr_corner;
        curr_corner = next_corner;
    }
    result = 0.5 * std::abs(result);
    return result;
}

double TileShape::distance(const FloatPoint& p_point) const
{
    std::optional<FloatPoint> nearest_point = nearest_point_approx(p_point);
    if (!nearest_point.has_value())
    {
        return std::numeric_limits<double>::max();
    }
    return nearest_point->distance(p_point);
}

double TileShape::border_distance(const FloatPoint& p_point) const
{
    std::optional<FloatPoint> nearest_point = nearest_border_point_approx(p_point);
    if (!nearest_point.has_value())
    {
        return std::numeric_limits<double>::max();
    }
    return nearest_point->distance(p_point);
}

double TileShape::smallest_radius() const
{
    return border_distance(centre_of_gravity());
}

std::optional<IntPoint> TileShape::nearest_point(const IntPoint& p_from_point) const
{
    if (!is_outside(p_from_point))
    {
        return p_from_point;
    }
    return nearest_border_point(p_from_point);
}

std::optional<FloatPoint> TileShape::nearest_point_approx(const FloatPoint& p_from_point) const
{
    if (this->contains(p_from_point))
    {
        return p_from_point;
    }
    return nearest_border_point_approx(p_from_point);
}

std::optional<IntPoint> TileShape::nearest_border_point(const IntPoint& p_from_point) const
{
    int line_count = border_line_count();
    if (line_count == 0)
    {
        return std::nullopt;
    }
    FloatPoint from_point_f = p_from_point.to_float();
    if (line_count == 1)
    {
        std::unique_ptr<Point> projection = p_from_point.perpendicular_projection(border_line(0));
        return std::optional<IntPoint>(static_cast<IntPoint&>(*projection));
    }
    double min_dist = std::numeric_limits<double>::max();
    int min_dist_ind = 0;

    // calculate the distance to the nearest corner first
    for (int i = 0; i < line_count; i++)
    {
        FloatPoint curr_corner_f = corner_approx(i);
        double curr_dist = curr_corner_f.distance_square(from_point_f);
        if (curr_dist < min_dist)
        {
            min_dist = curr_dist;
            min_dist_ind = i;
        }
    }

    IntPoint nearest_point = corner(min_dist_ind);

    int prev_ind = line_count - 2;
    int curr_ind = line_count - 1;

    for (int next_ind = 0; next_ind < line_count; next_ind++)
    {
        std::unique_ptr<Point> projection = p_from_point.perpendicular_projection(border_line(curr_ind));
        IntPoint projected_point = static_cast<IntPoint&>(*projection);
        if ((!corner_is_bounded(curr_ind)
                || border_line(prev_ind).side_of(projected_point) == Side::ON_THE_RIGHT)
            && (!corner_is_bounded(next_ind)
                || border_line(next_ind).side_of(projected_point) == Side::ON_THE_RIGHT))
        {
            FloatPoint projected_point_f = projected_point.to_float();
            double curr_dist = projected_point_f.distance_square(from_point_f);
            if (curr_dist < min_dist)
            {
                min_dist = curr_dist;
                nearest_point = projected_point;
            }
        }
        prev_ind = curr_ind;
        curr_ind = next_ind;
    }
    return nearest_point;
}

std::optional<FloatPoint> TileShape::nearest_border_point_approx(const FloatPoint& p_from_point) const
{
    std::vector<FloatPoint> nearest_points = nearest_border_points_approx(p_from_point, 1);
    if (nearest_points.empty())
    {
        return std::nullopt;
    }
    return nearest_points[0];
}

std::vector<FloatPoint> TileShape::nearest_border_points_approx(const FloatPoint& p_from_point, int p_count) const
{
    if (p_count <= 0)
    {
        return {};
    }
    int line_count = border_line_count();
    int result_count = std::min(p_count, line_count);
    if (line_count == 0)
    {
        return {};
    }
    if (line_count == 1)
    {
        std::vector<FloatPoint> result;
        std::unique_ptr<Point> projection = p_from_point.perpendicular_projection(border_line(0));
        result.emplace_back(static_cast<FloatPoint&>(*projection));
        return result;
    }
    if (this->dimension() == 0)
    {
        std::vector<FloatPoint> result;
        result.emplace_back(corner_approx(0));
        return result;
    }
    std::vector<FloatPoint> nearest_points(result_count, FloatPoint(0, 0));
    std::vector<double> min_dists(result_count, std::numeric_limits<double>::max());

    auto insert_point = [&nearest_points, &min_dists, result_count](double p_curr_dist, const FloatPoint& p_point)
    {
        for (int j = 0; j < result_count; j++)
        {
            if (p_curr_dist < min_dists[j])
            {
                for (int k = j + 1; k < result_count; k++)
                {
                    min_dists[k] = min_dists[k - 1];
                    nearest_points[k] = nearest_points[k - 1];
                }
                min_dists[j] = p_curr_dist;
                nearest_points[j] = p_point;
                break;
            }
        }
    };

    // calculate the distances to the nearest corners first
    for (int i = 0; i < line_count; i++)
    {
        if (corner_is_bounded(i))
        {
            FloatPoint curr_corner = corner_approx(i);
            double curr_dist = curr_corner.distance_square(p_from_point);
            insert_point(curr_dist, curr_corner);
        }
    }

    int prev_ind = line_count - 2;
    int curr_ind = line_count - 1;

    for (int next_ind = 0; next_ind < line_count; next_ind++)
    {
        std::unique_ptr<Point> projection = p_from_point.perpendicular_projection(border_line(curr_ind));
        FloatPoint projected_point = static_cast<FloatPoint&>(*projection);
        if ((!corner_is_bounded(curr_ind)
                || border_line(prev_ind).side_of(projected_point) == Side::ON_THE_RIGHT)
            && (!corner_is_bounded(next_ind)
                || border_line(next_ind).side_of(projected_point) == Side::ON_THE_RIGHT))
        {
            double curr_dist = projected_point.distance_square(p_from_point);
            insert_point(curr_dist, projected_point);
        }
        prev_ind = curr_ind;
        curr_ind = next_ind;
    }
    return nearest_points;
}

int TileShape::index_of_nearest_corner(const IntPoint& p_from_point) const
{
    FloatPoint from_point_f = p_from_point.to_float();
    int result = 0;
    int corner_count = border_line_count();
    // The Java original initializes min_dist with Double.MIN_VALUE which
    // prevents the loop from ever updating result. The C++ port fixes this by
    // starting from the maximal distance.
    double min_dist = std::numeric_limits<double>::max();
    for (int i = 0; i < corner_count; i++)
    {
        double curr_dist = corner_approx(i).distance(from_point_f);
        if (curr_dist < min_dist)
        {
            min_dist = curr_dist;
            result = i;
        }
    }
    return result;
}

std::optional<FloatLine> TileShape::diagonal_corner_segment() const
{
    if (this->is_empty())
    {
        return std::nullopt;
    }
    FloatPoint first_corner = this->corner_approx(0);
    FloatPoint last_corner = this->corner_approx(this->border_line_count() / 2);
    return FloatLine(first_corner, last_corner);
}

double TileShape::length() const
{
    if (!this->is_bounded())
    {
        return static_cast<double>(std::numeric_limits<int>::max());
    }
    int dim = this->dimension();
    if (dim <= 0)
    {
        return 0;
    }
    if (dim == 1)
    {
        return this->circumference() / 2;
    }
    // now the shape is 2-dimensional
    double max_distance = -1;
    double max_distance_2 = -1;
    FloatPoint gravity_point = this->centre_of_gravity();
    for (int i = 0; i < border_line_count(); i++)
    {
        double curr_distance = std::abs(border_line(i).signed_distance(gravity_point));
        if (curr_distance > max_distance)
        {
            max_distance_2 = max_distance;
            max_distance = curr_distance;
        }
        else if (curr_distance > max_distance_2)
        {
            max_distance_2 = curr_distance;
        }
    }
    return max_distance + max_distance_2;
}

double TileShape::distance_to_the_left(const Line& p_line) const
{
    double result = static_cast<double>(std::numeric_limits<int>::max());
    for (int i = 0; i < this->border_line_count(); i++)
    {
        FloatPoint curr_corner = this->corner_approx(i);
        Side line_side = p_line.side_of(curr_corner, 1);
        if (line_side == Side::COLLINEAR)
        {
            line_side = p_line.side_of(this->corner(i));
        }
        if (line_side == Side::ON_THE_RIGHT)
        {
            // curr_point would be outside the result shape
            result = -1;
            break;
        }
        result = std::min(result, p_line.signed_distance(curr_corner));
    }
    return result;
}

Side TileShape::side_of(const Line& p_line) const
{
    bool on_the_left = false;
    bool on_the_right = false;
    for (int i = 0; i < this->border_line_count(); i++)
    {
        Side curr_side = p_line.side_of(this->corner(i));
        if (curr_side == Side::ON_THE_LEFT)
        {
            on_the_right = true;
        }
        else if (curr_side == Side::ON_THE_RIGHT)
        {
            on_the_left = true;
        }
        if (on_the_left && on_the_right)
        {
            return Side::COLLINEAR;
        }
    }
    Side result;
    if (on_the_left)
    {
        result = Side::ON_THE_LEFT;
    }
    else
    {
        result = Side::ON_THE_RIGHT;
    }
    return result;
}

std::vector<std::unique_ptr<TileShape>> TileShape::split_to_convex() const
{
    std::vector<std::unique_ptr<TileShape>> result;
    result.emplace_back(clone());
    return result;
}

std::vector<std::pair<int, int>> TileShape::entrance_points(const Polyline& p_polyline) const
{
    std::vector<std::pair<int, int>> result;
    result.reserve(2 * p_polyline.arr.size());
    int prev_line_no = -1;
    int prev_edge_no = -1;
    for (size_t line_no = 1; line_no + 1 < p_polyline.arr.size(); ++line_no)
    {
        LineSegment curr_seg(p_polyline, static_cast<int>(line_no));
        std::vector<int> curr_intersections = curr_seg.border_intersections(*this);
        for (int edge_no : curr_intersections)
        {
            if (static_cast<int>(line_no) != prev_line_no || edge_no != prev_edge_no)
            {
                result.emplace_back(static_cast<int>(line_no), edge_no);
                prev_line_no = static_cast<int>(line_no);
                prev_edge_no = edge_no;
            }
        }
    }
    return result;
}

namespace {

template <typename T>
std::vector<std::unique_ptr<TileShape>> wrap_tile_pieces(std::vector<T> pieces)
{
    std::vector<std::unique_ptr<TileShape>> result;
    result.reserve(pieces.size());
    for (auto& piece : pieces)
    {
        result.emplace_back(std::make_unique<T>(std::move(piece)));
    }
    return result;
}

} // namespace

// Port of Java TileShape.cutout: this.cutout(p_shape) == p_shape.cutout_from(this).
std::vector<std::unique_ptr<TileShape>> TileShape::cutout(const TileShape& p_shape) const
{
    if (const auto* hole_simplex = dynamic_cast<const Simplex*>(&p_shape))
    {
        if (const auto* divide_simplex = dynamic_cast<const Simplex*>(this))
        {
            return wrap_tile_pieces(hole_simplex->cutout_from(*divide_simplex));
        }
        if (const auto* divide_octagon = dynamic_cast<const IntOctagon*>(this))
        {
            return wrap_tile_pieces(divide_octagon->cutout_from(*hole_simplex));
        }
        if (const auto* divide_box = dynamic_cast<const IntBox*>(this))
        {
            return wrap_tile_pieces(divide_box->cutout_from(*hole_simplex));
        }
    }
    else if (const auto* hole_octagon = dynamic_cast<const IntOctagon*>(&p_shape))
    {
        if (const auto* divide_simplex = dynamic_cast<const Simplex*>(this))
        {
            return wrap_tile_pieces(hole_octagon->cutout_from(*divide_simplex));
        }
        if (const auto* divide_octagon = dynamic_cast<const IntOctagon*>(this))
        {
            return wrap_tile_pieces(hole_octagon->cutout_from(*divide_octagon));
        }
        if (const auto* divide_box = dynamic_cast<const IntBox*>(this))
        {
            return wrap_tile_pieces(hole_octagon->cutout_from(*divide_box));
        }
    }
    else if (const auto* hole_box = dynamic_cast<const IntBox*>(&p_shape))
    {
        if (const auto* divide_simplex = dynamic_cast<const Simplex*>(this))
        {
            return wrap_tile_pieces(hole_box->cutout_from(*divide_simplex));
        }
        if (const auto* divide_octagon = dynamic_cast<const IntOctagon*>(this))
        {
            return wrap_tile_pieces(hole_box->cutout_from(*divide_octagon));
        }
        if (const auto* divide_box = dynamic_cast<const IntBox*>(this))
        {
            return wrap_tile_pieces(hole_box->cutout_from(*divide_box));
        }
    }

    logger::FRLogger::instance().warn("TileShape.cutout: unsupported shape combination");
    return {};
}

std::unique_ptr<TileShape> TileShape::get_instance(const std::vector<Line>& p_line_arr)
{
    Simplex simplex = Simplex::get_instance(p_line_arr);
    return simplex.simplify();
}

std::unique_ptr<TileShape> TileShape::get_instance(const std::vector<IntPoint>& p_convex_polygon)
{
    std::vector<Line> line_arr;
    line_arr.reserve(p_convex_polygon.size());
    for (size_t j = 0; j + 1 < p_convex_polygon.size(); j++)
    {
        line_arr.emplace_back(p_convex_polygon[j], p_convex_polygon[j + 1]);
    }
    if (!p_convex_polygon.empty())
    {
        line_arr.emplace_back(p_convex_polygon.back(), p_convex_polygon.front());
    }
    return get_instance(line_arr);
}

std::unique_ptr<TileShape> TileShape::offset(double p_dist) const
{
    if (auto* s = dynamic_cast<const Simplex*>(this))
    {
        return std::make_unique<Simplex>(s->offset(p_dist));
    }
    if (auto* b = dynamic_cast<const IntBox*>(this))
    {
        return std::make_unique<IntBox>(b->offset(p_dist));
    }
    if (auto* o = dynamic_cast<const IntOctagon*>(this))
    {
        return std::make_unique<IntOctagon>(o->offset(p_dist));
    }
    logger::FRLogger::instance().warn("TileShape.offset: unsupported shape type");
    return clone();
}

std::unique_ptr<TileShape> TileShape::get_instance(const Line& p_line)
{
    std::vector<Line> lines{p_line};
    return std::make_unique<Simplex>(Simplex::get_instance(lines));
}

IntOctagon TileShape::get_instance(int p_lx, int p_ly, int p_rx, int p_uy, int p_ulx, int p_lrx, int p_llx,
    int p_urx)
{
    IntOctagon oct(p_lx, p_ly, p_rx, p_uy, p_ulx, p_lrx, p_llx, p_urx);
    return oct.normalize();
}

IntOctagon TileShape::get_instance(int p_lower_left_x, int p_lower_left_y, int p_upper_right_x,
    int p_upper_right_y)
{
    IntBox box(p_lower_left_x, p_lower_left_y, p_upper_right_x, p_upper_right_y);
    return box.to_IntOctagon();
}

IntBox TileShape::get_instance(const IntPoint& p_point)
{
    return p_point.surrounding_box();
}

int TileShape::intersecting_border_line_no(const Point& p_from_point,
                                           const Direction& p_direction) const
{
    constexpr double PARALLEL_SENTINEL = 2147483647.0;
    FloatPoint from_point;
    if (const auto* int_point = dynamic_cast<const IntPoint*>(&p_from_point))
    {
        if (!contains(*int_point))
        {
            return -1;
        }
        from_point = int_point->to_float();
    }
    else
    {
        from_point = p_from_point.to_float();
        if (!contains(from_point))
        {
            return -1;
        }
    }
    // Line through p_from_point along p_direction (Java new Line(Point, Direction)).
    IntPoint origin = from_point.round();
    std::unique_ptr<Vector> dir_vec = p_direction.get_vector();
    std::unique_ptr<Point> second = dir_vec->add_to(origin);
    const IntPoint* second_int = dynamic_cast<const IntPoint*>(second.get());
    if (second_int == nullptr || *second_int == origin)
    {
        return -1;
    }
    Line intersection_line(origin, *second_int);
    const FloatPoint second_line_point = second_int->to_float();
    int result = -1;
    double min_distance = std::numeric_limits<double>::max();
    for (int i = 0; i < border_line_count(); ++i)
    {
        const Line curr_border_line = border_line(i);
        const FloatPoint curr_intersection =
            curr_border_line.intersection_approx(intersection_line);
        if (curr_intersection.x >= PARALLEL_SENTINEL)
        {
            continue; // lines are parallel
        }
        const double curr_distance = curr_intersection.distance_square(from_point);
        if (curr_distance < min_distance)
        {
            const bool direction_ok =
                curr_border_line.side_of(second_line_point) == Side::ON_THE_LEFT
                || second_line_point.distance_square(curr_intersection) < curr_distance;
            if (direction_ok)
            {
                result = i;
                min_distance = curr_distance;
            }
        }
    }
    return result;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting