#include "geometry/planar/LineSegment.h"

#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/PolylineShape.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace freerouting {
namespace geometry {
namespace planar {

namespace
{
    const Line& degenerate_line()
    {
        static const Line result(IntPoint(0, 0), IntPoint(1, 0));
        return result;
    }
} // namespace

LineSegment::LineSegment(const Line& p_start_line, const Line& p_middle_line, const Line& p_end_line)
    : start_(p_start_line)
    , middle_(p_middle_line)
    , end_(p_end_line)
{
}

LineSegment::LineSegment(const Polyline& p_polyline, int p_no)
    : start_(degenerate_line())
    , middle_(degenerate_line())
    , end_(degenerate_line())
{
    if (p_no <= 0 || p_no >= static_cast<int>(p_polyline.arr.size()) - 1)
    {
        logger::FRLogger::instance().warn("LineSegment from Polyline: p_no out of range");
        return;
    }
    start_ = p_polyline.arr[p_no - 1];
    middle_ = p_polyline.arr[p_no];
    end_ = p_polyline.arr[p_no + 1];
}

LineSegment::LineSegment(const PolylineShape& p_shape, int p_no)
    : start_(degenerate_line())
    , middle_(degenerate_line())
    , end_(degenerate_line())
{
    const int line_count = p_shape.border_line_count();
    if (p_no < 0 || p_no >= line_count)
    {
        logger::FRLogger::instance().warn("LineSegment from TileShape: p_no out of range");
        return;
    }
    start_ = (p_no == 0) ? p_shape.border_line(line_count - 1) : p_shape.border_line(p_no - 1);
    middle_ = p_shape.border_line(p_no);
    end_ = (p_no == line_count - 1) ? p_shape.border_line(0) : p_shape.border_line(p_no + 1);
}

LineSegment::LineSegment(const TileShape& p_shape, int p_no)
    : start_(degenerate_line())
    , middle_(degenerate_line())
    , end_(degenerate_line())
{
    const int line_count = p_shape.border_line_count();
    if (p_no < 0 || p_no >= line_count)
    {
        logger::FRLogger::instance().warn("LineSegment from TileShape: p_no out of range");
        return;
    }
    start_ = (p_no == 0) ? p_shape.border_line(line_count - 1) : p_shape.border_line(p_no - 1);
    middle_ = p_shape.border_line(p_no);
    end_ = (p_no == line_count - 1) ? p_shape.border_line(0) : p_shape.border_line(p_no + 1);
}

IntDirection LineSegment::normalized_direction(const IntPoint& p_from, const IntPoint& p_to)
{
    const int64_t dx = p_to.x - p_from.x;
    const int64_t dy = p_to.y - p_from.y;
    const int64_t g = std::gcd(std::llabs(dx), std::llabs(dy));
    if (g == 0)
    {
        return IntDirection(0, 1);
    }
    return IntDirection(dx / g, dy / g);
}

LineSegment LineSegment::from_endpoints(const IntPoint& p_start, const IntPoint& p_end)
{
    const IntDirection dir = normalized_direction(p_start, p_end);
    const Line start(p_start, IntPoint(p_start.x - dir.y, p_start.y + dir.x));
    const Line middle(p_start, p_end);
    const Line end(p_end, IntPoint(p_end.x - dir.y, p_end.y + dir.x));
    return LineSegment(start, middle, end);
}

IntPoint LineSegment::start_point() const
{
    if (!precalculated_start_point_.has_value())
    {
        std::unique_ptr<Point> is = middle_.intersection(start_);
        if (!is)
        {
            logger::FRLogger::instance().warn("LineSegment.start_point: lines are parallel");
            precalculated_start_point_ = IntPoint(0, 0);
        }
        else if (const IntPoint* int_point = dynamic_cast<const IntPoint*>(is.get()))
        {
            precalculated_start_point_ = *int_point;
        }
        else if (const FloatPoint* float_point = dynamic_cast<const FloatPoint*>(is.get()))
        {
            precalculated_start_point_ = float_point->round();
        }
    }
    return *precalculated_start_point_;
}

IntPoint LineSegment::end_point() const
{
    if (!precalculated_end_point_.has_value())
    {
        std::unique_ptr<Point> is = middle_.intersection(end_);
        if (!is)
        {
            logger::FRLogger::instance().warn("LineSegment.end_point: lines are parallel");
            precalculated_end_point_ = IntPoint(0, 0);
        }
        else if (const IntPoint* int_point = dynamic_cast<const IntPoint*>(is.get()))
        {
            precalculated_end_point_ = *int_point;
        }
        else if (const FloatPoint* float_point = dynamic_cast<const FloatPoint*>(is.get()))
        {
            precalculated_end_point_ = float_point->round();
        }
    }
    return *precalculated_end_point_;
}

FloatPoint LineSegment::start_point_approx() const
{
    if (precalculated_start_point_.has_value())
    {
        return precalculated_start_point_->to_float();
    }
    return start_.intersection_approx(middle_);
}

FloatPoint LineSegment::end_point_approx() const
{
    if (precalculated_end_point_.has_value())
    {
        return precalculated_end_point_->to_float();
    }
    return end_.intersection_approx(middle_);
}

LineSegment LineSegment::opposite() const
{
    return LineSegment(end_.opposite(), middle_.opposite(), start_.opposite());
}

Polyline LineSegment::to_polyline() const
{
    std::vector<Line> lines;
    lines.push_back(start_);
    lines.push_back(middle_);
    lines.push_back(end_);
    return Polyline(lines);
}

std::unique_ptr<Simplex> LineSegment::to_simplex() const
{
    std::vector<Line> line_arr;
    line_arr.reserve(4);
    if (start_.side_of(end_point()) == Side::ON_THE_RIGHT)
    {
        line_arr.push_back(start_.opposite());
    }
    else
    {
        line_arr.push_back(start_);
    }
    line_arr.push_back(middle_);
    line_arr.push_back(middle_.opposite());
    if (end_.side_of(start_point()) == Side::ON_THE_RIGHT)
    {
        line_arr.push_back(end_.opposite());
    }
    else
    {
        line_arr.push_back(end_);
    }
    return std::make_unique<Simplex>(Simplex::get_instance(std::move(line_arr)));
}

bool LineSegment::contains(const IntPoint& p_point) const
{
    if (middle_.side_of(p_point) != Side::COLLINEAR)
    {
        return false;
    }
    // create a perpendicular line at p_point and check, that the two
    // endpoints of this segment are on different sides of that line.
    const IntDirection perpendicular_direction(middle_.direction().y, -middle_.direction().x);
    const Line perpendicular_line = Line::get_instance(p_point, perpendicular_direction);
    const Side start_point_side = perpendicular_line.side_of(start_point());
    const Side end_point_side = perpendicular_line.side_of(end_point());
    return start_point_side != end_point_side || start_point_side == Side::COLLINEAR;
}

IntBox LineSegment::bounding_box() const
{
    const FloatPoint start_corner = middle_.intersection_approx(start_);
    const FloatPoint end_corner = middle_.intersection_approx(end_);
    const double llx = std::min(start_corner.x, end_corner.x);
    const double lly = std::min(start_corner.y, end_corner.y);
    const double urx = std::max(start_corner.x, end_corner.x);
    const double ury = std::max(start_corner.y, end_corner.y);
    return IntBox(
        static_cast<int64_t>(std::floor(llx)),
        static_cast<int64_t>(std::floor(lly)),
        static_cast<int64_t>(std::ceil(urx)),
        static_cast<int64_t>(std::ceil(ury))
    );
}

IntOctagon LineSegment::bounding_octagon() const
{
    const FloatPoint start_corner = middle_.intersection_approx(start_);
    const FloatPoint end_corner = middle_.intersection_approx(end_);
    const int64_t lx = static_cast<int64_t>(std::floor(std::min(start_corner.x, end_corner.x)));
    const int64_t ly = static_cast<int64_t>(std::floor(std::min(start_corner.y, end_corner.y)));
    const int64_t rx = static_cast<int64_t>(std::ceil(std::max(start_corner.x, end_corner.x)));
    const int64_t uy = static_cast<int64_t>(std::ceil(std::max(start_corner.y, end_corner.y)));
    const double start_x_minus_y = start_corner.x - start_corner.y;
    const double end_x_minus_y = end_corner.x - end_corner.y;
    const int64_t ulx = static_cast<int64_t>(std::floor(std::min(start_x_minus_y, end_x_minus_y)));
    const int64_t lrx = static_cast<int64_t>(std::ceil(std::max(start_x_minus_y, end_x_minus_y)));
    const double start_x_plus_y = start_corner.x + start_corner.y;
    const double end_x_plus_y = end_corner.x + end_corner.y;
    const int64_t llx = static_cast<int64_t>(std::floor(std::min(start_x_plus_y, end_x_plus_y)));
    const int64_t urx = static_cast<int64_t>(std::ceil(std::max(start_x_plus_y, end_x_plus_y)));
    return IntOctagon(lx, ly, rx, uy, ulx, lrx, llx, urx).normalize();
}

LineSegment LineSegment::change_length_approx(double p_new_length) const
{
    const FloatPoint new_end_point = start_point_approx().change_length(end_point_approx(), p_new_length);
    const IntDirection perpendicular_direction(middle_.direction().y, -middle_.direction().x);
    const Line new_end_line = Line::get_instance(new_end_point.round(), perpendicular_direction);
    return LineSegment(start_, middle_, new_end_line);
}

std::vector<Line> LineSegment::intersection(const LineSegment& p_other) const
{
    std::vector<Line> result;
    if (!bounding_box().intersects(p_other.bounding_box()))
    {
        return result;
    }
    const Side start_point_side = p_other.middle_.side_of(start_point());
    const Side end_point_side = p_other.middle_.side_of(end_point());
    if (start_point_side == Side::COLLINEAR && end_point_side == Side::COLLINEAR)
    {
        // there may be an overlap
        const LineSegment this_sorted = sort_endpoints_in_x_y();
        const LineSegment other_sorted = p_other.sort_endpoints_in_x_y();
        const LineSegment& left_line = (this_sorted.start_point().compare_x_y(other_sorted.start_point()) <= 0) ? this_sorted : other_sorted;
        const LineSegment& right_line = (this_sorted.start_point().compare_x_y(other_sorted.start_point()) <= 0) ? other_sorted : this_sorted;
        const int cmp = left_line.end_point().compare_x_y(right_line.start_point());
        if (cmp < 0)
        {
            // end point of the left line is to the left of the start point of the right line
            return result;
        }
        if (cmp == 0)
        {
            // end point of the left line is equal to the start point of the right line
            result.push_back(left_line.end_);
            return result;
        }
        // now there is a real overlap
        result.push_back(right_line.start_);
        if (right_line.end_point().compare_x_y(left_line.end_point()) >= 0)
        {
            result.push_back(left_line.end_);
        }
        else
        {
            result.push_back(right_line.end_);
        }
        return result;
    }
    if (start_point_side == end_point_side || middle_.side_of(p_other.start_point()) == middle_.side_of(p_other.end_point()))
    {
        // no intersection possible
        return result;
    }
    // now both start points and both end points are on different sides of the middle
    // line of the other segment.
    result.push_back(p_other.middle_);
    return result;
}

bool LineSegment::intersects(const LineSegment& p_other) const
{
    return !intersection(p_other).empty();
}

bool LineSegment::overlaps(const LineSegment& p_other) const
{
    return intersection(p_other).size() > 1;
}

LineSegment LineSegment::sort_endpoints_in_x_y() const
{
    if (start_point().compare_x_y(end_point()) > 0)
    {
        LineSegment result(end_, middle_, start_);
        result.precalculated_start_point_ = precalculated_end_point_;
        result.precalculated_end_point_ = precalculated_start_point_;
        return result;
    }
    return *this;
}

std::vector<IntPoint> LineSegment::stair_approximation(double p_width, bool p_to_the_right) const
{
    IntPoint sp = start_point().to_float().round();
    IntPoint ep = end_point().to_float().round();
    if (sp == ep)
    {
        return {};
    }

    if (sp.x == ep.x || sp.y == ep.y)
    {
        return {sp, ep};
    }

    const int64_t dx = ep.x - sp.x;
    const int64_t dy = ep.y - sp.y;
    const int64_t abs_dx = std::abs(dx);
    const int64_t abs_dy = std::abs(dy);
    const bool function_of_x = abs_dx >= abs_dy;

    int64_t stair_width;
    int64_t stair_count;

    if (function_of_x)
    {
        stair_width = static_cast<int64_t>(std::lround((p_width * static_cast<double>(abs_dx)) / static_cast<double>(abs_dy)));
        stair_count = (abs_dx - 1) / stair_width + 1;
        if (ep.x < sp.x)
        {
            stair_width = -stair_width;
        }
    }
    else
    {
        stair_width = static_cast<int64_t>(std::lround((p_width * static_cast<double>(abs_dy)) / static_cast<double>(abs_dx)));
        stair_count = (abs_dy - 1) / stair_width + 1;
        if (ep.y < sp.y)
        {
            stair_width = -stair_width;
        }
    }

    std::vector<IntPoint> result(2 * stair_count + 1);
    result[0] = sp;
    const double det = static_cast<double>(dx) * static_cast<double>(dy);
    const bool change_x_first = (p_to_the_right && det > 0) || (!p_to_the_right && det < 0);
    size_t curr_index = 0;

    int64_t prev_line_point_x = sp.x;
    int64_t prev_line_point_y = sp.y;
    for (int64_t i = 1; i < stair_count; ++i)
    {
        int64_t curr_line_point_x;
        int64_t curr_line_point_y;
        if (function_of_x)
        {
            curr_line_point_x = sp.x + i * stair_width;
            curr_line_point_y = static_cast<int64_t>(std::lround(middle_.function_value_approx(static_cast<double>(curr_line_point_x))));
        }
        else
        {
            curr_line_point_y = sp.y + i * stair_width;
            curr_line_point_x = static_cast<int64_t>(std::lround(middle_.function_in_y_value_approx(static_cast<double>(curr_line_point_y))));
        }
        ++curr_index;
        if (change_x_first)
        {
            result[curr_index] = IntPoint(curr_line_point_x, prev_line_point_y);
        }
        else
        {
            result[curr_index] = IntPoint(prev_line_point_x, curr_line_point_y);
        }
        ++curr_index;
        result[curr_index] = IntPoint(curr_line_point_x, curr_line_point_y);
        prev_line_point_x = curr_line_point_x;
        prev_line_point_y = curr_line_point_y;
    }
    ++curr_index;
    if (change_x_first)
    {
        result[curr_index] = IntPoint(ep.x, prev_line_point_y);
    }
    else
    {
        result[curr_index] = IntPoint(prev_line_point_x, ep.y);
    }
    ++curr_index;
    result[curr_index] = ep;
    return result;
}

std::vector<IntPoint> LineSegment::stair_approximation_45(double p_width, bool p_to_the_right) const
{
    IntPoint sp = start_point().to_float().round();
    IntPoint ep = end_point().to_float().round();
    if (sp == ep)
    {
        return {};
    }
    IntVector delta(ep.x - sp.x, ep.y - sp.y);
    if (delta.is_multiple_of_45_degree())
    {
        return {sp, ep};
    }
    IntVector abs_delta(std::abs(delta.x), std::abs(delta.y));
    const bool function_of_x = abs_delta.x >= abs_delta.y;
    const double det = static_cast<double>(delta.x) * static_cast<double>(delta.y);
    int64_t stair_width;
    int64_t stair_count;
    if (function_of_x)
    {
        stair_width = static_cast<int64_t>(std::lround((p_width * static_cast<double>(abs_delta.x)) / static_cast<double>(abs_delta.y)));
        stair_count = (abs_delta.x - 1) / stair_width + 1;
        if (ep.x < sp.x)
        {
            stair_width = -stair_width;
        }
    }
    else
    {
        stair_width = static_cast<int64_t>(std::lround((p_width * static_cast<double>(abs_delta.y)) / static_cast<double>(abs_delta.x)));
        stair_count = (abs_delta.y - 1) / stair_width + 1;
        if (ep.y < sp.y)
        {
            stair_width = -stair_width;
        }
    }
    std::vector<IntPoint> result(2 * stair_count + 1);
    result[0] = sp;
    IntPoint prev_line_point = sp;
    size_t curr_index = 0;
    const int64_t sign_stair_width = (stair_width > 0) ? 1 : ((stair_width < 0) ? -1 : 0);

    for (int64_t i = 1; i <= stair_count; ++i)
    {
        IntPoint curr_line_point;
        int64_t curr_x;
        int64_t curr_y;
        if (i == stair_count)
        {
            curr_line_point = ep;
        }
        else
        {
            if (function_of_x)
            {
                curr_x = sp.x + i * stair_width;
                curr_y = static_cast<int64_t>(std::lround(middle_.function_value_approx(static_cast<double>(curr_x))));
            }
            else
            {
                curr_y = sp.y + i * stair_width;
                curr_x = static_cast<int64_t>(std::lround(middle_.function_in_y_value_approx(static_cast<double>(curr_y))));
            }
            curr_line_point = IntPoint(curr_x, curr_y);
        }
        if (function_of_x)
        {
            const bool diagonal_first = (p_to_the_right && det < 0) || (!p_to_the_right && det > 0);
            if (diagonal_first)
            {
                curr_x = prev_line_point.x + sign_stair_width * std::abs(curr_line_point.y - prev_line_point.y);
                curr_y = curr_line_point.y;
            }
            else
            {
                curr_x = curr_line_point.x - sign_stair_width * std::abs(curr_line_point.y - prev_line_point.y);
                curr_y = prev_line_point.y;
            }
        }
        else
        {
            const bool diagonal_first = (p_to_the_right && det > 0) || (!p_to_the_right && det < 0);
            if (diagonal_first)
            {
                curr_x = curr_line_point.x;
                curr_y = prev_line_point.y + sign_stair_width * std::abs(curr_line_point.x - prev_line_point.x);
            }
            else
            {
                curr_x = prev_line_point.x;
                curr_y = curr_line_point.y - sign_stair_width * std::abs(curr_line_point.x - prev_line_point.x);
            }
        }
        ++curr_index;
        result[curr_index] = IntPoint(curr_x, curr_y);
        ++curr_index;
        result[curr_index] = curr_line_point;
        prev_line_point = curr_line_point;
    }
    return result;
}

std::vector<int> LineSegment::border_intersections(const TileShape& p_shape) const
{
    if (!bounding_box().intersects(p_shape.bounding_box()))
    {
        return {};
    }

    const int edge_count = p_shape.border_line_count();
    Line prev_line = p_shape.border_line(edge_count - 1);
    Line curr_line = p_shape.border_line(0);
    std::vector<int> result;
    std::vector<std::unique_ptr<Point>> intersection_pts;
    IntPoint line_start = start_point();
    IntPoint line_end = end_point();

    for (int edge_line_no = 0; edge_line_no < edge_count; ++edge_line_no)
    {
        Line next_line = (edge_line_no == edge_count - 1) ? p_shape.border_line(0) : p_shape.border_line(edge_line_no + 1);

        Side start_point_side = curr_line.side_of(line_start);
        Side end_point_side = curr_line.side_of(line_end);
        if (start_point_side == Side::ON_THE_LEFT && end_point_side == Side::ON_THE_LEFT)
        {
            return {};
        }

        if (start_point_side == Side::COLLINEAR)
        {
            if (end_point_side != Side::ON_THE_RIGHT)
            {
                return {};
            }
        }

        if (end_point_side == Side::COLLINEAR)
        {
            if (start_point_side != Side::ON_THE_RIGHT)
            {
                return {};
            }
        }

        if (start_point_side != Side::ON_THE_RIGHT || end_point_side != Side::ON_THE_RIGHT)
        {
            std::unique_ptr<Point> is = middle_.intersection(curr_line);
            if (is)
            {
                Side prev_line_side_of_is = prev_line.side_of(*is);
                Side next_line_side_of_is = next_line.side_of(*is);
                if (prev_line_side_of_is != Side::ON_THE_LEFT && next_line_side_of_is != Side::ON_THE_LEFT)
                {
                    if (prev_line_side_of_is == Side::COLLINEAR)
                    {
                        IntPoint prev_prev_corner = (edge_line_no == 0) ? p_shape.corner(edge_count - 1) : p_shape.corner(edge_line_no - 1);
                        IntPoint next_corner = (edge_line_no == edge_count - 1) ? p_shape.corner(0) : p_shape.corner(edge_line_no + 1);
                        Side prev_prev_corner_side = middle_.side_of(prev_prev_corner);
                        Side next_corner_side = middle_.side_of(next_corner);
                        if (prev_prev_corner_side == Side::COLLINEAR || next_corner_side == Side::COLLINEAR || prev_prev_corner_side == next_corner_side)
                        {
                            return {};
                        }
                    }
                    if (next_line_side_of_is == Side::COLLINEAR)
                    {
                        IntPoint prev_corner = p_shape.corner(edge_line_no);
                        IntPoint next_next_corner;
                        if (edge_line_no == edge_count - 2)
                        {
                            next_next_corner = p_shape.corner(0);
                        }
                        else if (edge_line_no == edge_count - 1)
                        {
                            next_next_corner = p_shape.corner(1);
                        }
                        else
                        {
                            next_next_corner = p_shape.corner(edge_line_no + 2);
                        }
                        Side prev_corner_side = middle_.side_of(prev_corner);
                        Side next_next_corner_side = middle_.side_of(next_next_corner);
                        if (prev_corner_side == Side::COLLINEAR || next_next_corner_side == Side::COLLINEAR || prev_corner_side == next_next_corner_side)
                        {
                            return {};
                        }
                    }
                    bool intersection_already_handled = false;
                    for (const auto& existing_is : intersection_pts)
                    {
                        if (is->equals(existing_is.get()))
                        {
                            intersection_already_handled = true;
                            break;
                        }
                    }
                    if (!intersection_already_handled)
                    {
                        result.push_back(edge_line_no);
                        intersection_pts.push_back(std::move(is));
                    }
                }
            }
        }

        prev_line = curr_line;
        curr_line = next_line;
    }

    if (result.empty())
    {
        return {};
    }

    if (result.size() == 2)
    {
        FloatPoint is0 = intersection_pts[0]->to_float();
        FloatPoint is1 = intersection_pts[1]->to_float();
        FloatPoint curr_start = line_start.to_float();
        if (curr_start.distance_square(is1) < curr_start.distance_square(is0))
        {
            std::swap(result[0], result[1]);
        }
        return result;
    }

    return result;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting