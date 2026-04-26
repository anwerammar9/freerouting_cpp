#include "geometry/planar/Polyline.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Direction.h"
#include "logger/FRLogger.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace freerouting {
namespace geometry {
namespace planar {

Polyline::Polyline(const std::vector<Line>& p_line_arr) : arr(p_line_arr)
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

std::vector<Line> Polyline::remove_consecutive_parallel_lines(const std::vector<Line>& p_line_arr)
{
    return p_line_arr;
}

std::vector<Line> Polyline::remove_overlaps(const std::vector<Line>& p_line_arr)
{
    return p_line_arr;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting