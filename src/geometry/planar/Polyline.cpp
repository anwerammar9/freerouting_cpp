#include "geometry/planar/Polyline.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/LineSegment.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <numeric>

namespace freerouting {
namespace geometry {
namespace planar {

Polyline::Polyline(const std::vector<Line>& p_line_arr)
    : arr(remove_overlaps(remove_consecutive_parallel_lines(p_line_arr)))
{
}

Polyline::Polyline(const std::vector<IntPoint>& p_points)
{
    if (p_points.size() < 2) return;
    for (size_t i = 1; i < p_points.size(); ++i) {
        arr.emplace_back(p_points[i-1], p_points[i]);
    }
}

Polyline::Polyline(const IntPoint& p_from_corner, const IntPoint& p_to_corner)
{
    arr.emplace_back(p_from_corner, p_to_corner);
}

Polyline::Polyline(const Polyline& p_other)
    : arr(p_other.arr)
{
}

Polyline& Polyline::operator=(const Polyline& p_other)
{
    if (this != &p_other)
    {
        arr = p_other.arr;
        precalculated_float_corners.clear();
        precalculated_corners.clear();
        precalculated_bounding_box.reset();
    }
    return *this;
}

Polyline Polyline::from_corners(const std::vector<IntPoint>& p_points)
{
    Polyline result;
    if (p_points.size() < 2)
    {
        return result;
    }
    auto normalized_direction = [](const IntPoint& p_from, const IntPoint& p_to)
    {
        const int64_t dx = p_to.x - p_from.x;
        const int64_t dy = p_to.y - p_from.y;
        const int64_t divisor = std::gcd(std::llabs(dx), std::llabs(dy));
        if (divisor == 0)
        {
            return IntVector(0, 1);
        }
        return IntVector(dx / divisor, dy / divisor);
    };
    const IntVector first_direction = normalized_direction(p_points[0], p_points[1]);
    result.arr.emplace_back(p_points[0], IntPoint(p_points[0].x - first_direction.y,
                                                 p_points[0].y + first_direction.x));
    for (size_t i = 1; i < p_points.size(); ++i)
    {
        result.arr.emplace_back(p_points[i - 1], p_points[i]);
    }
    const IntVector last_direction =
        normalized_direction(p_points[p_points.size() - 2], p_points.back());
    result.arr.emplace_back(p_points.back(), IntPoint(p_points.back().x - last_direction.y,
                                                      p_points.back().y + last_direction.x));
    return result;
}

int Polyline::corner_count() const
{
    if (arr.size() < 2) return 0;
    return static_cast<int>(arr.size()) - 1;
}

bool Polyline::is_empty() const
{
    return arr.size() < 3;
}

bool Polyline::is_point() const
{
    if (arr.size() < 2) return true;
    const IntPoint first = corner(0);
    for (int i = 1; i < corner_count(); ++i) {
        if (!(corner(i) == first)) return false;
    }
    return true;
}

bool Polyline::is_orthogonal() const
{
    for (const auto& line : arr) {
        if (!(line.is_vertical() || line.is_horizontal())) return false;
    }
    return true;
}

bool Polyline::is_multiple_of_45_degree() const
{
    for (const auto& line : arr) {
        const IntVector d = line.direction();
        if (!(d.x == 0 || d.y == 0 || std::llabs(d.x) == std::llabs(d.y))) {
            return false;
        }
    }
    return true;
}

IntPoint Polyline::first_corner() const
{
    return corner(0);
}

IntPoint Polyline::last_corner() const
{
    return corner(corner_count() - 1);
}

std::vector<IntPoint> Polyline::corner_arr() const
{
    if (arr.size() < 2) return {};
    std::vector<IntPoint> corners;
    corners.reserve(arr.size() - 1);
    for (size_t i = 0; i + 1 < arr.size(); ++i) {
        corners.push_back(corner(static_cast<int>(i)));
    }
    return corners;
}

std::vector<FloatPoint> Polyline::corner_approx_arr() const
{
    if (arr.size() < 2) return {};
    std::vector<FloatPoint> corners;
    corners.reserve(arr.size() - 1);
    for (size_t i = 0; i + 1 < arr.size(); ++i) {
        corners.push_back(corner_approx(static_cast<int>(i)));
    }
    return corners;
}

FloatPoint Polyline::corner_approx(int p_no) const
{
    if (arr.size() < 2) return FloatPoint(0.0, 0.0);
    const int no = std::clamp(p_no, 0, static_cast<int>(arr.size()) - 2);
    return arr[no].intersection_approx(arr[no + 1]);
}

IntPoint Polyline::corner(int p_no) const
{
    if (arr.size() < 2) {
        return IntPoint(0, 0);
    }
    int no = std::clamp(p_no, 0, static_cast<int>(arr.size()) - 2);
    const FloatPoint approx = arr[no].intersection_approx(arr[no + 1]);
    return IntPoint(static_cast<int64_t>(std::llround(approx.x)),
                    static_cast<int64_t>(std::llround(approx.y)));
}

Polyline Polyline::reverse() const
{
    std::vector<Line> reversed;
    reversed.reserve(arr.size());
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
        reversed.emplace_back(it->b, it->a);
    }
    return Polyline(reversed);
}

double Polyline::length_approx(int p_from_corner, int p_to_corner) const
{
    if (arr.size() < 2) return 0.0;
    const int from = std::clamp(p_from_corner, 0, static_cast<int>(arr.size()) - 2);
    const int to = std::clamp(p_to_corner, 0, static_cast<int>(arr.size()) - 2);
    if (to <= from) return 0.0;

    double result = 0.0;
    for (int i = from; i < to; ++i) {
        result += corner_approx(i + 1).distance(corner_approx(i));
    }
    return result;
}

double Polyline::length_approx() const
{
    return length_approx(0, corner_count() - 1);
}

std::unique_ptr<Polyline> Polyline::translate_by(const IntVector& p_vector) const
{
    if (p_vector.is_zero()) {
        return std::make_unique<Polyline>(arr);
    }
    std::vector<Line> translated;
    translated.reserve(arr.size());
    for (const auto& line : arr) {
        translated.emplace_back(
            IntPoint(line.a.x + p_vector.x, line.a.y + p_vector.y),
            IntPoint(line.b.x + p_vector.x, line.b.y + p_vector.y));
    }
    return std::make_unique<Polyline>(translated);
}

IntBox Polyline::bounding_box(int p_from_corner_no, int p_to_corner_no) const
{
    if (arr.size() < 2) return IntBox::EMPTY;
    const int from = std::clamp(p_from_corner_no, 0, static_cast<int>(arr.size()) - 2);
    const int to = std::clamp(p_to_corner_no, 0, static_cast<int>(arr.size()) - 2);

    double llx = std::numeric_limits<double>::max();
    double lly = std::numeric_limits<double>::max();
    double urx = std::numeric_limits<double>::lowest();
    double ury = std::numeric_limits<double>::lowest();
    for (int i = from; i <= to; ++i) {
        const FloatPoint c = corner_approx(i);
        llx = std::min(llx, c.x);
        lly = std::min(lly, c.y);
        urx = std::max(urx, c.x);
        ury = std::max(ury, c.y);
    }
    return IntBox(
        static_cast<int64_t>(std::floor(llx)),
        static_cast<int64_t>(std::floor(lly)),
        static_cast<int64_t>(std::ceil(urx)),
        static_cast<int64_t>(std::ceil(ury))
    );
}

IntBox Polyline::bounding_box() const
{
    if (!precalculated_bounding_box) {
        precalculated_bounding_box = std::make_unique<IntBox>(bounding_box(0, corner_count() - 1));
    }
    return *precalculated_bounding_box;
}

LineSegment Polyline::projection_line(int p_no) const
{
    return LineSegment(*this, p_no);
}

std::optional<LineSegment> Polyline::projection_line(const IntPoint& p_from_point) const
{
    const FloatPoint from_point = p_from_point.to_float();
    double min_distance = std::numeric_limits<double>::max();
    std::optional<Line> result_line;
    std::optional<Line> nearest_line;
    for (int i = 1; i < static_cast<int>(arr.size()) - 1; i++)
    {
        const FloatPoint projection = from_point.projection_approx(arr[i]);
        const double curr_distance = projection.distance(from_point);
        if (curr_distance < min_distance)
        {
            const std::optional<IntDirection> direction_towards_line = arr[i].perpendicular_direction(p_from_point);
            if (!direction_towards_line.has_value())
            {
                continue;
            }
            const Line curr_result_line = Line::get_instance(p_from_point, *direction_towards_line);
            const IntPoint prev_corner = corner(i - 1);
            const IntPoint next_corner = corner(i);
            const Side prev_corner_side = curr_result_line.side_of(prev_corner);
            const Side next_corner_side = curr_result_line.side_of(next_corner);
            if (prev_corner_side == next_corner_side && prev_corner_side != Side::COLLINEAR)
            {
                // the projection point is outside the line segment
                continue;
            }
            nearest_line = arr[i];
            min_distance = curr_distance;
            result_line = curr_result_line;
        }
    }
    if (!nearest_line.has_value())
    {
        return std::nullopt;
    }
    const IntVector dir = nearest_line->direction();
    const Line start_line(p_from_point, IntPoint(p_from_point.x + dir.x, p_from_point.y + dir.y));
    return LineSegment(start_line, *result_line, *nearest_line);
}

Polyline Polyline::shorten(int p_new_line_count, double p_last_segment_length) const
{
    if (p_new_line_count < 3 || p_new_line_count > corner_count())
    {
        return Polyline(arr);
    }
    const FloatPoint last_corner = corner_approx(p_new_line_count - 2);
    const FloatPoint prev_last_corner = corner_approx(p_new_line_count - 3);
    const IntPoint new_last_corner = prev_last_corner.change_length(last_corner, p_last_segment_length).round();
    if (new_last_corner == corner(corner_count() - 2))
    {
        // skip the last line
        return skip_lines(p_new_line_count - 1, p_new_line_count - 1);
    }
    std::vector<Line> new_lines;
    new_lines.reserve(static_cast<size_t>(p_new_line_count));
    for (int i = 0; i < p_new_line_count - 2; i++)
    {
        new_lines.push_back(arr[i]);
    }
    // create the last 2 lines of the new polyline
    IntPoint first_line_point = arr[p_new_line_count - 2].a;
    if (first_line_point == new_last_corner)
    {
        first_line_point = arr[p_new_line_count - 2].b;
    }
    const Line new_prev_last_line(first_line_point, new_last_corner);
    new_lines.push_back(new_prev_last_line);
    const IntVector last_line_direction = new_prev_last_line.direction().turn_90_degree(3);
    const IntPoint last_line_end(new_last_corner.x + last_line_direction.x, new_last_corner.y + last_line_direction.y);
    new_lines.emplace_back(new_last_corner, last_line_end);
    return Polyline(new_lines);
}

double Polyline::distance(const FloatPoint& p_from_point) const
{
    if (arr.size() < 2) return 0.0;
    if (corner_count() == 1) {
        return corner_approx(0).distance(p_from_point);
    }

    auto distance_to_segment = [](const FloatPoint& p, const FloatPoint& a, const FloatPoint& b) -> double {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double len2 = dx * dx + dy * dy;
        if (len2 <= 1e-12) {
            return p.distance(a);
        }
        const double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
        const double clamped = std::clamp(t, 0.0, 1.0);
        const FloatPoint proj(a.x + clamped * dx, a.y + clamped * dy);
        return p.distance(proj);
    };

    double min_distance = std::numeric_limits<double>::max();
    double prev_x = corner_approx(0).x;
    double prev_y = corner_approx(0).y;
    for (int i = 1; i < corner_count(); ++i) {
        const FloatPoint curr = corner_approx(i);
        min_distance = std::min(
            min_distance,
            distance_to_segment(p_from_point, FloatPoint(prev_x, prev_y), curr));
        prev_x = curr.x;
        prev_y = curr.y;
    }
    return min_distance;
}

std::unique_ptr<Polyline> Polyline::combine(const Polyline& p_other) const
{
    if (p_other.arr.size() < 2 || arr.size() < 2) {
        return std::make_unique<Polyline>(arr);
    }

    bool combine_at_start = false;
    bool combine_other_at_start = false;
    if (first_corner() == p_other.first_corner()) {
        combine_at_start = true;
        combine_other_at_start = true;
    } else if (first_corner() == p_other.last_corner()) {
        combine_at_start = true;
        combine_other_at_start = false;
    } else if (last_corner() == p_other.first_corner()) {
        combine_at_start = false;
        combine_other_at_start = true;
    } else if (last_corner() == p_other.last_corner()) {
        combine_at_start = false;
        combine_other_at_start = false;
    } else {
        return std::make_unique<Polyline>(arr);
    }

    std::vector<Line> line_arr;
    line_arr.reserve(arr.size() + p_other.arr.size() - 2);
    if (combine_at_start) {
        if (combine_other_at_start) {
            for (size_t i = 0; i + 1 < p_other.arr.size(); ++i) {
                const auto& l = p_other.arr[p_other.arr.size() - i - 1];
                line_arr.emplace_back(l.b, l.a);
            }
        } else {
            for (size_t i = 0; i + 1 < p_other.arr.size(); ++i) {
                line_arr.push_back(p_other.arr[i]);
            }
        }
        for (size_t i = 1; i < arr.size(); ++i) {
            line_arr.push_back(arr[i]);
        }
    } else {
        for (size_t i = 0; i + 1 < arr.size(); ++i) {
            line_arr.push_back(arr[i]);
        }
        if (combine_other_at_start) {
            for (size_t i = 1; i < p_other.arr.size(); ++i) {
                line_arr.push_back(p_other.arr[i]);
            }
        } else {
            for (size_t i = 1; i < p_other.arr.size(); ++i) {
                const auto& l = p_other.arr[p_other.arr.size() - i - 1];
                line_arr.emplace_back(l.b, l.a);
            }
        }
    }
    return std::make_unique<Polyline>(line_arr);
}

Polyline Polyline::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    std::vector<Line> new_arr;
    new_arr.reserve(arr.size());
    for (const auto& line : arr) {
        new_arr.emplace_back(line.turn_90_degree(p_factor, p_pole));
    }
    return Polyline(new_arr);
}

Polyline Polyline::rotate_approx(double p_angle, const FloatPoint& p_pole) const
{
    if (p_angle == 0)
    {
        return Polyline(arr);
    }
    std::vector<IntPoint> new_corners;
    new_corners.reserve(arr.size() > 0 ? corner_count() : 0);
    for (int i = 0; i < corner_count(); ++i) {
        const FloatPoint rotated = corner_approx(i).rotate(p_angle, p_pole);
        new_corners.emplace_back(static_cast<int64_t>(std::llround(rotated.x)),
                                 static_cast<int64_t>(std::llround(rotated.y)));
    }
    return Polyline(new_corners);
}

Polyline Polyline::mirror_vertical(const IntPoint& p_pole) const
{
    std::vector<Line> mirrored;
    mirrored.reserve(arr.size());
    for (const auto& line : arr) {
        mirrored.push_back(line.mirror_vertical(p_pole));
    }
    return Polyline(mirrored);
}

Polyline Polyline::mirror_horizontal(const IntPoint& p_pole) const
{
    std::vector<Line> mirrored;
    mirrored.reserve(arr.size());
    for (const auto& line : arr) {
        mirrored.push_back(line.mirror_horizontal(p_pole));
    }
    return Polyline(mirrored);
}

IntOctagon Polyline::bounding_octagon(int p_from_corner_no, int p_to_corner_no) const
{
    if (arr.size() < 2) return IntOctagon::EMPTY;
    const int from = std::clamp(p_from_corner_no, 0, static_cast<int>(arr.size()) - 2);
    const int to = std::clamp(p_to_corner_no, 0, static_cast<int>(arr.size()) - 2);

    double lx = std::numeric_limits<double>::max();
    double ly = std::numeric_limits<double>::max();
    double rx = std::numeric_limits<double>::lowest();
    double uy = std::numeric_limits<double>::lowest();
    double ulx = std::numeric_limits<double>::max();
    double lrx = std::numeric_limits<double>::lowest();
    double llx = std::numeric_limits<double>::max();
    double urx = std::numeric_limits<double>::lowest();
    for (int i = from; i <= to; ++i) {
        const FloatPoint curr = corner_approx(i);
        lx = std::min(lx, curr.x);
        ly = std::min(ly, curr.y);
        rx = std::max(rx, curr.x);
        uy = std::max(uy, curr.y);
        ulx = std::min(ulx, curr.x - curr.y);
        lrx = std::max(lrx, curr.x - curr.y);
        llx = std::min(llx, curr.x + curr.y);
        urx = std::max(urx, curr.x + curr.y);
    }
    return IntOctagon(
        static_cast<int64_t>(std::floor(lx)),
        static_cast<int64_t>(std::floor(ly)),
        static_cast<int64_t>(std::ceil(rx)),
        static_cast<int64_t>(std::ceil(uy)),
        static_cast<int64_t>(std::floor(ulx)),
        static_cast<int64_t>(std::ceil(lrx)),
        static_cast<int64_t>(std::floor(llx)),
        static_cast<int64_t>(std::ceil(urx)));
}

FloatPoint Polyline::nearest_point_approx(const FloatPoint& p_from_point) const
{
    if (arr.size() < 2) return p_from_point;
    const std::vector<FloatPoint> corners = corner_approx_arr();
    double min_distance = std::numeric_limits<double>::max();
    FloatPoint nearest_point(0.0, 0.0);
    for (const auto& curr : corners) {
        const double curr_distance = curr.distance(p_from_point);
        if (curr_distance < min_distance) {
            min_distance = curr_distance;
            nearest_point = curr;
        }
    }
    const double c_tolerance = 1.0;
    for (size_t i = 1; i + 1 < arr.size(); ++i) {
        const FloatPoint projection = p_from_point.projection_approx(arr[i]);
        const double curr_distance = projection.distance(p_from_point);
        if (curr_distance < min_distance) {
            const double segment_length = corners[i].distance(corners[i - 1]);
            if (projection.distance(corners[i]) + projection.distance(corners[i - 1]) <
                segment_length + c_tolerance) {
                min_distance = curr_distance;
                nearest_point = projection;
            }
        }
    }
    return nearest_point;
}

bool Polyline::contains(const IntPoint& p_point) const
{
    if (arr.size() < 2) return false;
    for (size_t i = 1; i + 1 < arr.size(); ++i) {
        LineSegment curr_segment = LineSegment::from_endpoints(corner(static_cast<int>(i - 1)), corner(static_cast<int>(i)));
        if (curr_segment.contains(p_point)) return true;
    }
    return false;
}

std::vector<std::unique_ptr<Polyline>> Polyline::split(int p_line_no, const Line& p_end_line) const
{
    std::vector<std::unique_ptr<Polyline>> result;
    if (p_line_no < 1 || p_line_no > static_cast<int>(arr.size()) - 2) return result;
    if (arr[p_line_no].is_parallel(p_end_line)) return result;
    std::unique_ptr<Point> new_end_corner_ptr = arr[p_line_no].intersection(p_end_line);
    if (!new_end_corner_ptr) return result;
    const IntPoint new_end_corner = static_cast<const IntPoint&>(*new_end_corner_ptr);
    if ((p_line_no == 1 && new_end_corner == first_corner())
        || (p_line_no >= static_cast<int>(arr.size()) - 2 && new_end_corner == last_corner()))
    {
        return result;
    }

    std::vector<Line> first_piece;
    if (corner(p_line_no - 1) == new_end_corner) {
        first_piece.reserve(p_line_no + 1);
        for (int i = 0; i < p_line_no + 1; ++i) {
            first_piece.push_back(arr[i]);
        }
    } else {
        first_piece.reserve(p_line_no + 2);
        for (int i = 0; i < p_line_no + 1; ++i) {
            first_piece.push_back(arr[i]);
        }
        first_piece.push_back(p_end_line);
    }

    std::vector<Line> second_piece;
    const int arr_size = static_cast<int>(arr.size());
    if (corner(p_line_no) == new_end_corner) {
        second_piece.reserve(arr_size - p_line_no);
        for (int i = p_line_no; i < arr_size; ++i) {
            second_piece.push_back(arr[i]);
        }
    } else {
        second_piece.reserve(arr_size - p_line_no + 1);
        second_piece.push_back(p_end_line);
        for (int i = p_line_no; i < arr_size; ++i) {
            second_piece.push_back(arr[i]);
        }
    }

    std::unique_ptr<Polyline> poly0 = std::make_unique<Polyline>(first_piece);
    std::unique_ptr<Polyline> poly1 = std::make_unique<Polyline>(second_piece);
    if (poly0->is_point() || poly1->is_point()) return result;
    result.push_back(std::move(poly0));
    result.push_back(std::move(poly1));
    return result;
}

Polyline Polyline::skip_lines(int p_from_no, int p_to_no) const
{
    if (p_from_no < 0 || p_to_no >= static_cast<int>(arr.size()) || p_from_no > p_to_no)
    {
        return Polyline(arr);
    }
    std::vector<Line> new_lines;
    new_lines.reserve(arr.size() - static_cast<size_t>(p_to_no - p_from_no + 1));
    for (int i = 0; i < p_from_no; ++i) {
        new_lines.push_back(arr[i]);
    }
    for (int i = p_to_no + 1; i < static_cast<int>(arr.size()); ++i) {
        new_lines.push_back(arr[i]);
    }
    return Polyline(new_lines);
}

static const bool USE_BOUNDING_OCTAGON_FOR_OFFSET_SHAPES = true;

std::vector<std::unique_ptr<TileShape>> Polyline::offset_shapes(int p_half_width) const
{
    return offset_shapes(p_half_width, 0, static_cast<int>(arr.size()) - 1);
}

std::vector<std::unique_ptr<TileShape>> Polyline::offset_shapes(int p_half_width, int p_from_no, int p_to_no) const
{
    const int from_no = std::max(p_from_no, 0);
    const int to_no = std::min(p_to_no, static_cast<int>(arr.size()) - 1);
    const int shape_count = std::max(to_no - from_no - 1, 0);
    std::vector<std::unique_ptr<TileShape>> shape_arr(static_cast<size_t>(shape_count));
    if (shape_count == 0) return shape_arr;

    std::vector<IntVector> dirs;
    dirs.reserve(arr.size());
    for (const auto& line : arr) {
        dirs.emplace_back(line.direction());
    }

    for (int i = from_no + 1; i < to_no; ++i)
    {
        const IntVector& prev_dir = dirs[i - 1];
        const IntVector& curr_dir = dirs[i];
        const IntVector& next_dir = dirs[i + 1];
        std::vector<Line> lines;
        lines.reserve(4);

        lines.push_back(arr[i].translate(-p_half_width));
        const Side next_dir_from_curr_dir = next_dir.side_of(curr_dir);
        if (next_dir_from_curr_dir == Side::ON_THE_LEFT) {
            lines.push_back(arr[i + 1].translate(-p_half_width));
        } else {
            lines.push_back(arr[i + 1].opposite().translate(-p_half_width));
        }
        lines.push_back(arr[i].opposite().translate(-p_half_width));
        const Side curr_dir_from_prev_dir = curr_dir.side_of(prev_dir);
        if (curr_dir_from_prev_dir == Side::ON_THE_LEFT) {
            lines.push_back(arr[i - 1].translate(-p_half_width));
        } else {
            lines.push_back(arr[i - 1].opposite().translate(-p_half_width));
        }

        std::vector<Line> cut_dog_ear_lines;
        FloatPoint corner_to_check(0.0, 0.0);
        Line curr_line = lines[1];
        const Line check_line = (next_dir_from_curr_dir == Side::ON_THE_LEFT) ? lines[2] : lines[0];
        FloatPoint check_distance_corner = corner_approx(i);
        const double check_dist_square = 2.0 * static_cast<double>(p_half_width) * static_cast<double>(p_half_width);
        const IntVector* tmp_curr_dir = &next_dir;
        bool direction_changed = false;
        for (int j = i + 2; j < static_cast<int>(arr.size()) - 1; ++j)
        {
            if (corner_approx(j - 1).distance_square(check_distance_corner) > check_dist_square) break;
            if (!direction_changed) {
                corner_to_check = curr_line.intersection_approx(check_line);
            }
            const IntVector& tmp_next_dir = dirs[j];
            const Side tmp_next_dir_from_tmp_curr_dir = tmp_next_dir.side_of(*tmp_curr_dir);
            direction_changed = tmp_next_dir_from_tmp_curr_dir != next_dir_from_curr_dir;
            if (!direction_changed) {
                const Line next_border_line = (tmp_next_dir_from_tmp_curr_dir == Side::ON_THE_LEFT)
                    ? arr[j].translate(-p_half_width)
                    : arr[j].opposite().translate(-p_half_width);
                const IntPoint corner_i = corner(i);
                const IntPoint corner_i_1 = corner(i - 1);
                if (next_border_line.side_of(corner_to_check.round()) == Side::ON_THE_LEFT
                    && next_border_line.side_of(corner_i) == Side::ON_THE_RIGHT
                    && next_border_line.side_of(corner_i_1) == Side::ON_THE_RIGHT)
                {
                    cut_dog_ear_lines.push_back(next_border_line);
                }
                tmp_curr_dir = &dirs[j];
                curr_line = next_border_line;
            }
        }

        check_distance_corner = corner_approx(i - 1);
        const Line back_check_line = (curr_dir_from_prev_dir == Side::ON_THE_LEFT) ? lines[2] : lines[0];
        curr_line = lines[3];
        tmp_curr_dir = &prev_dir;
        direction_changed = false;
        for (int j = i - 2; j >= 1; --j)
        {
            if (corner_approx(j).distance_square(check_distance_corner) > check_dist_square) break;
            if (!direction_changed) {
                corner_to_check = curr_line.intersection_approx(back_check_line);
            }
            const IntVector& tmp_prev_dir = dirs[j];
            const Side tmp_curr_dir_from_tmp_prev_dir = tmp_curr_dir->side_of(tmp_prev_dir);
            direction_changed = tmp_curr_dir_from_tmp_prev_dir != curr_dir_from_prev_dir;
            if (!direction_changed) {
                const Line prev_border_line = (tmp_curr_dir->side_of(tmp_prev_dir) == Side::ON_THE_LEFT)
                    ? arr[j].translate(-p_half_width)
                    : arr[j].opposite().translate(-p_half_width);
                const IntPoint corner_i = corner(i);
                const IntPoint corner_i_1 = corner(i - 1);
                if (prev_border_line.side_of(corner_to_check.round()) == Side::ON_THE_LEFT
                    && prev_border_line.side_of(corner_i) == Side::ON_THE_RIGHT
                    && prev_border_line.side_of(corner_i_1) == Side::ON_THE_RIGHT)
                {
                    cut_dog_ear_lines.push_back(prev_border_line);
                }
                tmp_curr_dir = &dirs[j];
                curr_line = prev_border_line;
            }
        }

        Simplex s1 = Simplex::get_instance(lines);
        if (!cut_dog_ear_lines.empty()) {
            s1 = s1.intersection(Simplex::get_instance(cut_dog_ear_lines));
        }
        const int curr_shape_no = i - from_no - 1;
        if (USE_BOUNDING_OCTAGON_FOR_OFFSET_SHAPES) {
            const IntOctagon surr_oct = bounding_octagon(i - 1, i);
            shape_arr[static_cast<size_t>(curr_shape_no)] =
                std::make_unique<Simplex>(s1.intersection(surr_oct.offset(p_half_width)));
        } else {
            const IntBox surr_box = bounding_box(i - 1, i);
            shape_arr[static_cast<size_t>(curr_shape_no)] =
                std::make_unique<Simplex>(s1.intersection(surr_box.offset(p_half_width).to_Simplex()));
        }
        if (shape_arr[static_cast<size_t>(curr_shape_no)]->is_empty()) {
            logger::FRLogger::instance().warn("offset_shapes: shape is empty");
        }
    }
    return shape_arr;
}

std::unique_ptr<TileShape> Polyline::offset_shape(int p_half_width, int p_no) const
{
    if (p_no < 0 || p_no > static_cast<int>(arr.size()) - 3) {
        logger::FRLogger::instance().warn("Polyline.offset_shape: p_no out of range");
        return nullptr;
    }
    auto result = offset_shapes(p_half_width, p_no, p_no + 2);
    if (result.empty()) return nullptr;
    return std::move(result[0]);
}

IntBox Polyline::offset_box(int p_half_width, int p_no) const
{
    if (p_no < 0 || p_no + 1 >= static_cast<int>(arr.size()) - 1) {
        return IntBox::EMPTY;
    }
    const IntPoint start = corner(p_no);
    const IntPoint end = corner(p_no + 1);
    const LineSegment curr_line_segment = LineSegment::from_endpoints(start, end);
    return curr_line_segment.bounding_box().offset(p_half_width);
}

std::vector<Line> Polyline::remove_consecutive_parallel_lines(const std::vector<Line>& p_line_arr)
{
    if (p_line_arr.size() < 3)
    {
        // polyline must have at least 3 lines
        return p_line_arr;
    }
    std::vector<Line> tmp_arr;
    tmp_arr.reserve(p_line_arr.size());
    tmp_arr.push_back(p_line_arr[0]);
    for (size_t i = 1; i < p_line_arr.size(); ++i)
    {
        // skip multiple lines
        if (!tmp_arr.back().is_parallel(p_line_arr[i]))
        {
            tmp_arr.push_back(p_line_arr[i]);
        }
    }
    if (tmp_arr.size() == p_line_arr.size())
    {
        // nothing skipped
        return p_line_arr;
    }
    // at least 1 line is skipped, adjust the array
    if (tmp_arr.size() < 3)
    {
        return {};
    }
    return tmp_arr;
}

std::vector<Line> Polyline::remove_overlaps(const std::vector<Line>& p_line_arr)
{
    if (p_line_arr.size() < 4)
    {
        return p_line_arr;
    }
    int new_length = 0;
    std::vector<Line> tmp_arr;
    tmp_arr.reserve(p_line_arr.size());
    const auto set_line_at = [&tmp_arr](int p_index, const Line& p_line) {
        if (static_cast<size_t>(p_index) == tmp_arr.size())
        {
            tmp_arr.push_back(p_line);
        }
        else
        {
            tmp_arr[static_cast<size_t>(p_index)] = p_line;
        }
    };
    set_line_at(0, p_line_arr[0]);
    if (!p_line_arr[0].is_equal_or_opposite(p_line_arr[2]))
    {
        ++new_length;
    }
    // else  skip the first line
    set_line_at(new_length, p_line_arr[1]);
    ++new_length;
    for (size_t i = 2; i < p_line_arr.size() - 2; ++i)
    {
        if (tmp_arr[static_cast<size_t>(new_length) - 1].is_equal_or_opposite(p_line_arr[i + 1]))
        {
            // skip 2 lines
            --new_length;
        }
        else
        {
            set_line_at(new_length, p_line_arr[i]);
            ++new_length;
        }
    }
    set_line_at(new_length, p_line_arr[p_line_arr.size() - 2]);
    ++new_length;
    if (!p_line_arr[p_line_arr.size() - 1].is_equal_or_opposite(tmp_arr[static_cast<size_t>(new_length) - 2]))
    {
        set_line_at(new_length, p_line_arr[p_line_arr.size() - 1]);
        ++new_length;
    }
    // else skip the last line
    if (new_length == static_cast<int>(p_line_arr.size()))
    {
        // nothing skipped
        return p_line_arr;
    }
    // at least 1 line is skipped, adjust the array
    if (new_length < 3)
    {
        return {};
    }
    return tmp_arr;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting