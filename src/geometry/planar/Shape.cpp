#include "geometry/planar/Shape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/PolygonShape.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <algorithm>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== Shape interface surface (port of Java Shape.java) ==============

double Shape::circumference() const
{
    FR_LOG_WARN("Shape::circumference not implemented for this shape");
    return 0.0;
}

double Shape::area() const
{
    FR_LOG_WARN("Shape::area not implemented for this shape");
    return 0.0;
}

FloatPoint Shape::centre_of_gravity() const
{
    FR_LOG_WARN("Shape::centre_of_gravity not implemented for this shape");
    return FloatPoint();
}

bool Shape::is_outside(const IntPoint& p_point) const
{
    return !contains(p_point);
}

bool Shape::is_outside(const FloatPoint& p_point) const
{
    return !contains(p_point);
}

bool Shape::contains_inside(const IntPoint& p_point) const
{
    return contains(p_point) && !contains_on_border(p_point);
}

bool Shape::contains_inside(const FloatPoint& p_point) const
{
    return contains(p_point) && !contains_on_border(p_point);
}

bool Shape::contains_on_border(const IntPoint& /*p_point*/) const
{
    return false;
}

bool Shape::contains_on_border(const FloatPoint& /*p_point*/) const
{
    return false;
}

bool Shape::contains(const FloatPoint& /*p_point*/) const
{
    FR_LOG_WARN("Shape::contains(FloatPoint) not implemented for this shape");
    return false;
}

double Shape::distance(const FloatPoint& /*p_point*/) const
{
    FR_LOG_WARN("Shape::distance not implemented for this shape");
    return 0.0;
}

double Shape::border_distance(const FloatPoint& /*p_point*/) const
{
    FR_LOG_WARN("Shape::border_distance not implemented for this shape");
    return 0.0;
}

double Shape::smallest_radius() const
{
    FR_LOG_WARN("Shape::smallest_radius not implemented for this shape");
    return 0.0;
}

std::unique_ptr<TileShape> Shape::bounding_tile() const
{
    return std::make_unique<IntBox>(bounding_box());
}

IntOctagon Shape::bounding_shape(const ShapeBoundingDirections& /*p_dirs*/) const
{
    return bounding_box().bounding_octagon();
}

std::unique_ptr<Shape> Shape::enlarge(double /*p_offset*/) const
{
    FR_LOG_WARN("Shape::enlarge not implemented for this shape");
    return nullptr;
}

bool Shape::intersects(const IntBox& p_box) const
{
    return bounding_box().intersects(p_box);
}

bool Shape::intersects(const IntOctagon& p_oct) const
{
    return bounding_box().intersects(p_oct);
}

bool Shape::intersects(const Simplex& p_simplex) const
{
    return bounding_box().intersects(p_simplex);
}

bool Shape::intersects(const Circle& p_circle) const
{
    return bounding_box().intersects(p_circle);
}

std::vector<std::unique_ptr<Polyline>> Shape::cutout(const Polyline& /*p_polyline*/) const
{
    FR_LOG_WARN("Shape::cutout not implemented for this shape");
    return {};
}

// ============== Circle Implementation ==============

Circle::Circle(const IntPoint& p_center, int64_t p_radius)
    : center(p_center), radius(p_radius < 0 ? -p_radius : p_radius)
{
    if (p_radius < 0)
    {
        FR_LOG_WARN("Circle constructor with negative radius");
    }
}

IntBox Circle::bounding_box() const
{
    return IntBox(center.x - radius, center.y - radius,
                  center.x + radius, center.y + radius);
}

IntOctagon Circle::bounding_octagon() const
{
    int64_t lx = center.x - radius;
    int64_t rx = center.x + radius;
    int64_t ly = center.y - radius;
    int64_t uy = center.y + radius;

    const double sqrt2_minus_1 = std::sqrt(2.0) - 1.0;
    const int64_t ceil_corner_value = static_cast<int64_t>(std::ceil(sqrt2_minus_1 * static_cast<double>(radius)));
    const int64_t floor_corner_value = static_cast<int64_t>(std::floor(sqrt2_minus_1 * static_cast<double>(radius)));

    int64_t ulx = lx - (center.y + floor_corner_value);
    int64_t lrx = rx - (center.y - ceil_corner_value);
    int64_t llx = lx + (center.y - floor_corner_value);
    int64_t urx = rx + (center.y + ceil_corner_value);
    return IntOctagon(lx, ly, rx, uy, ulx, lrx, llx, urx);
}

bool Circle::intersects(const Shape& other) const
{
    const Circle* other_circle = dynamic_cast<const Circle*>(&other);
    if (other_circle)
    {
        int64_t dx = center.x - other_circle->center.x;
        int64_t dy = center.y - other_circle->center.y;
        int64_t dist_sq = dx*dx + dy*dy;
        int64_t rad_sum = radius + other_circle->radius;
        return dist_sq <= rad_sum * rad_sum;
    }
    return false;
}

bool Circle::contains(const IntPoint& point) const
{
    int64_t dx = point.x - center.x;
    int64_t dy = point.y - center.y;
    return dx*dx + dy*dy <= radius * radius;
}

bool Circle::is_empty() const
{
    return false;
}

bool Circle::is_bounded() const
{
    return true;
}

int Circle::dimension() const
{
    if (radius == 0)
    {
        return 0;
    }
    return 2;
}

double Circle::circumference() const
{
    return 2.0 * M_PI * static_cast<double>(radius);
}

double Circle::area() const
{
    double r = static_cast<double>(radius);
    return M_PI * r * r;
}

FloatPoint Circle::centre_of_gravity() const
{
    return center.to_float();
}

bool Circle::is_outside(const IntPoint& p_point) const
{
    return is_outside(p_point.to_float());
}

bool Circle::is_outside(const FloatPoint& p_point) const
{
    return p_point.distance_square(center.to_float()) > static_cast<double>(radius) * radius;
}

bool Circle::contains(const FloatPoint& p_point) const
{
    return p_point.distance_square(center.to_float()) <= static_cast<double>(radius) * radius;
}

bool Circle::contains_inside(const IntPoint& p_point) const
{
    return center.distance_square(p_point) < static_cast<double>(radius) * radius;
}

bool Circle::contains_inside(const FloatPoint& p_point) const
{
    return p_point.distance_square(center.to_float()) < static_cast<double>(radius) * radius;
}

bool Circle::contains_on_border(const IntPoint& p_point) const
{
    return center.distance_square(p_point) == static_cast<double>(radius) * radius;
}

bool Circle::contains_on_border(const FloatPoint& p_point) const
{
    return p_point.distance_square(center.to_float()) == static_cast<double>(radius) * radius;
}

double Circle::distance(const FloatPoint& p_point) const
{
    double d = p_point.distance(center.to_float()) - static_cast<double>(radius);
    return std::max(d, 0.0);
}

double Circle::border_distance(const FloatPoint& p_point) const
{
    double d = p_point.distance(center.to_float()) - static_cast<double>(radius);
    return std::abs(d);
}

double Circle::smallest_radius() const
{
    return static_cast<double>(radius);
}

double Circle::max_width() const
{
    return 2.0 * static_cast<double>(radius);
}

double Circle::min_width() const
{
    return 2.0 * static_cast<double>(radius);
}

std::unique_ptr<TileShape> Circle::bounding_tile() const
{
    return std::make_unique<IntOctagon>(bounding_octagon());
}

std::unique_ptr<TileShape> Circle::bounding_tile(int p_max_segment_length) const
{
    int quadrant_division_count = static_cast<int>(radius / p_max_segment_length) + 1;
    if (quadrant_division_count <= 2)
    {
        return std::make_unique<IntOctagon>(bounding_octagon());
    }
    std::vector<Line> tangent_line_arr;
    tangent_line_arr.reserve(static_cast<size_t>(quadrant_division_count) * 4);
    for (int i = 0; i < quadrant_division_count; i++)
    {
        const double curr_angle = static_cast<double>(i) * M_PI / (2.0 * quadrant_division_count);
        const int64_t curr_a_x = (i == 0) ? center.x + radius
                                          : center.x + static_cast<int64_t>(std::ceil(
                                                          std::sin(curr_angle) * static_cast<double>(radius)));
        const int64_t curr_a_y = (i == 0) ? center.y
                                          : center.y + static_cast<int64_t>(std::ceil(
                                                          std::cos(curr_angle) * static_cast<double>(radius)));
        const IntPoint curr_a(curr_a_x, curr_a_y);
        const IntPoint curr_b = curr_a.turn_90_degree(1, center);
        const Line curr_tangent(curr_a, curr_b);
        tangent_line_arr.push_back(curr_tangent.turn_90_degree(3, center));
        tangent_line_arr.push_back(curr_tangent);
        tangent_line_arr.push_back(curr_tangent.turn_90_degree(1, center));
        tangent_line_arr.push_back(curr_tangent.turn_90_degree(2, center));
    }
    return TileShape::get_instance(tangent_line_arr);
}

std::vector<std::unique_ptr<TileShape>> Circle::split_to_convex() const
{
    std::vector<std::unique_ptr<TileShape>> result;
    result.push_back(bounding_tile());
    return result;
}

bool Circle::is_contained_in(const IntBox& p_box) const
{
    if (p_box.ll.x > center.x - radius)
    {
        return false;
    }
    if (p_box.ll.y > center.y - radius)
    {
        return false;
    }
    if (p_box.ur.x < center.x + radius)
    {
        return false;
    }
    return p_box.ur.y >= center.y + radius;
}

bool Circle::intersects(const Circle& p_other) const
{
    double d_square = static_cast<double>(radius + p_other.radius);
    d_square *= d_square;
    int64_t dx = center.x - p_other.center.x;
    int64_t dy = center.y - p_other.center.y;
    return static_cast<double>(dx*dx + dy*dy) <= d_square;
}

bool Circle::intersects(const IntBox& p_box) const
{
    return p_box.distance(center.to_float()) <= static_cast<double>(radius);
}

bool Circle::intersects(const IntOctagon& p_oct) const
{
    return p_oct.distance(center.to_float()) <= static_cast<double>(radius);
}

bool Circle::intersects(const Simplex& p_simplex) const
{
    return p_simplex.distance(center.to_float()) <= static_cast<double>(radius);
}

Circle Circle::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    int n = p_factor % 4;
    if (n < 0)
    {
        n += 4;
    }
    int64_t dx = center.x - p_pole.x;
    int64_t dy = center.y - p_pole.y;
    int64_t new_dx;
    int64_t new_dy;
    switch (n)
    {
        case 0:
            new_dx = dx;
            new_dy = dy;
            break;
        case 1:
            new_dx = -dy;
            new_dy = dx;
            break;
        case 2:
            new_dx = -dx;
            new_dy = -dy;
            break;
        case 3:
            new_dx = dy;
            new_dy = -dx;
            break;
        default:
            new_dx = 0;
            new_dy = 0;
            break;
    }
    return Circle(IntPoint(p_pole.x + new_dx, p_pole.y + new_dy), radius);
}

Circle Circle::mirror_vertical(const IntPoint& p_pole) const
{
    return Circle(IntPoint(2 * p_pole.x - center.x, center.y), radius);
}

Circle Circle::mirror_horizontal(const IntPoint& p_pole) const
{
    return Circle(IntPoint(center.x, 2 * p_pole.y - center.y), radius);
}

Circle Circle::offset(double p_offset) const
{
    double new_radius = static_cast<double>(radius) + p_offset;
    int64_t r = static_cast<int64_t>(std::round(new_radius));
    return Circle(center, r);
}

Circle Circle::shrink(double p_offset) const
{
    double new_radius = static_cast<double>(radius) - p_offset;
    int64_t r = std::max(static_cast<int64_t>(std::round(new_radius)), static_cast<int64_t>(1));
    return Circle(center, r);
}

std::unique_ptr<Shape> Circle::enlarge(double p_offset) const
{
    if (p_offset == 0)
    {
        return std::make_unique<Circle>(*this);
    }
    int64_t new_radius = radius + static_cast<int64_t>(std::round(p_offset));
    return std::make_unique<Circle>(center, new_radius);
}

Circle Circle::translate_by(const Vector& p_vector) const
{
    if (p_vector.is_zero())
    {
        return *this;
    }
    const IntVector* int_vector = dynamic_cast<const IntVector*>(&p_vector);
    if (int_vector == nullptr)
    {
        return *this;
    }
    return Circle(IntPoint(center.x + int_vector->x, center.y + int_vector->y), radius);
}

Circle Circle::rotate_approx(double p_angle, const FloatPoint& p_pole) const
{
    IntPoint new_center = center.to_float().rotate(p_angle, p_pole).round();
    return Circle(new_center, radius);
}

FloatPoint Circle::nearest_point_approx(const FloatPoint& p_point) const
{
    FloatPoint c = center.to_float();
    double dist = p_point.distance(c);
    if (dist == 0.0)
    {
        return FloatPoint(c.x + static_cast<double>(radius), c.y);
    }
    double factor = static_cast<double>(radius) / dist;
    return FloatPoint(c.x + (p_point.x - c.x) * factor, c.y + (p_point.y - c.y) * factor);
}

IntOctagon Circle::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return p_dirs.bounds(*this);
}

Circle Circle::get_border() const
{
    return *this;
}

std::vector<std::unique_ptr<Shape>> Circle::get_holes() const
{
    return {};
}

std::vector<FloatPoint> Circle::corner_approx_arr() const
{
    return {};
}

std::vector<std::unique_ptr<Polyline>> Circle::cutout(const Polyline& /*p_polyline*/) const
{
    FR_LOG_WARN("Circle.cutout not yet implemented");
    return {};
}

std::string Circle::to_string() const
{
    std::ostringstream ss;
    ss << "Circle[" << center.to_string() << ", r=" << radius << "]";
    return ss.str();
}

// ============== PolygonArea Implementation ==============

PolygonArea::PolygonArea(const std::vector<IntPoint>& p_corners)
    : corners(p_corners)
{
}

IntBox PolygonArea::bounding_box() const
{
    if (corners.empty())
        return IntBox::EMPTY;

    int64_t min_x = corners[0].x;
    int64_t max_x = corners[0].x;
    int64_t min_y = corners[0].y;
    int64_t max_y = corners[0].y;

    for (const auto& pt : corners)
    {
        min_x = std::min(min_x, pt.x);
        max_x = std::max(max_x, pt.x);
        min_y = std::min(min_y, pt.y);
        max_y = std::max(max_y, pt.y);
    }

    return IntBox(min_x, min_y, max_x, max_y);
}

IntOctagon PolygonArea::bounding_octagon() const
{
    IntBox box = bounding_box();
    return IntOctagon(box.ur.x, box.ur.y, box.ll.x, box.ll.y,
                     box.ll.x - box.ur.y, box.ur.x - box.ll.y,
                     box.ur.x + box.ur.y, box.ll.x + box.ll.y);
}

bool PolygonArea::is_empty() const
{
    return corners.empty();
}

std::unique_ptr<Area> PolygonArea::clone() const
{
    return std::make_unique<PolygonArea>(corners);
}

bool PolygonArea::contains(const IntPoint& point) const
{
    if (corners.empty())
        return false;

    int n = static_cast<int>(corners.size());
    bool inside = false;

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((corners[i].y > point.y) != (corners[j].y > point.y)) &&
            (point.x < (corners[j].x - corners[i].x) * (point.y - corners[i].y) /
                           (corners[j].y - corners[i].y) + corners[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

std::string PolygonArea::to_string() const
{
    std::ostringstream ss;
    ss << "PolygonArea[";
    for (size_t i = 0; i < corners.size(); ++i)
    {
        if (i > 0) ss << ", ";
        ss << corners[i].to_string();
    }
    ss << "]";
    return ss.str();
}

bool PolygonArea::intersects(const Shape& other) const
{
    if (is_empty())
        return false;

    IntBox this_box = bounding_box();
    IntBox other_box = other.bounding_box();

    if (!this_box.intersects(other_box))
        return false;

    for (const auto& corner : corners)
    {
        if (other.contains(corner))
            return true;
    }
    return false;
}

bool PolygonArea::is_bounded() const
{
    return !corners.empty();
}

int PolygonArea::dimension() const
{
    return corners.empty() ? -1 : 2;
}

bool PolygonArea::contains(const FloatPoint& p_point) const
{
    if (corners.empty())
    {
        return false;
    }
    return PolygonShape(corners).contains(p_point);
}

bool PolygonArea::is_contained_in(const IntBox& p_box) const
{
    return !corners.empty() && p_box.contains(bounding_box());
}

std::optional<FloatPoint> PolygonArea::nearest_point_approx(const FloatPoint& p_from_point) const
{
    if (corners.empty())
    {
        return std::nullopt;
    }
    return PolygonShape(corners).nearest_point_approx(p_from_point);
}

std::vector<FloatPoint> PolygonArea::corner_approx_arr() const
{
    return PolygonShape(corners).corner_approx_arr();
}

const std::vector<std::unique_ptr<TileShape>>& PolygonArea::split_to_convex() const
{
    if (!precalculated_convex_pieces_)
    {
        const PolygonShape polygon_shape(corners);
        const auto& pieces = polygon_shape.split_to_convex();
        auto result = std::make_shared<std::vector<std::unique_ptr<TileShape>>>();
        result->reserve(pieces.size());
        for (const auto& piece : pieces)
        {
            result->push_back(piece->clone());
        }
        precalculated_convex_pieces_ = std::move(result);
    }
    return *precalculated_convex_pieces_;
}

std::unique_ptr<Area> PolygonArea::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& corner : corners)
    {
        new_corners.push_back(corner.turn_90_degree(p_factor, p_pole));
    }
    return std::make_unique<PolygonArea>(std::move(new_corners));
}

std::unique_ptr<Area> PolygonArea::rotate_approx(double p_angle, const FloatPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& corner : corners)
    {
        new_corners.push_back(corner.to_float().rotate(p_angle, p_pole).round());
    }
    return std::make_unique<PolygonArea>(std::move(new_corners));
}

std::unique_ptr<Area> PolygonArea::translate_by(const Vector& p_vector) const
{
    if (p_vector.is_zero())
    {
        return std::make_unique<PolygonArea>(*this);
    }
    const auto* int_vector = dynamic_cast<const IntVector*>(&p_vector);
    if (int_vector == nullptr)
    {
        FR_LOG_WARN("PolygonArea.translate_by: only IntVector translation supported");
        return nullptr;
    }
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& corner : corners)
    {
        new_corners.push_back(IntPoint(corner.x + int_vector->x, corner.y + int_vector->y));
    }
    return std::make_unique<PolygonArea>(std::move(new_corners));
}

std::unique_ptr<Area> PolygonArea::mirror_horizontal(const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& corner : corners)
    {
        new_corners.push_back(corner.mirror_horizontal(p_pole));
    }
    return std::make_unique<PolygonArea>(std::move(new_corners));
}

std::unique_ptr<Area> PolygonArea::mirror_vertical(const IntPoint& p_pole) const
{
    std::vector<IntPoint> new_corners;
    new_corners.reserve(corners.size());
    for (const auto& corner : corners)
    {
        new_corners.push_back(corner.mirror_vertical(p_pole));
    }
    return std::make_unique<PolygonArea>(std::move(new_corners));
}

} // namespace planar
} // namespace geometry
} // namespace freerouting