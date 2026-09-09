#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace freerouting {
namespace geometry {
namespace planar {

// ============== IntBox Implementation ==============

const IntBox IntBox::EMPTY(0, 0, -1, -1);
const IntBox IntBox::INFINITE(-2147483647LL, -2147483647LL, 2147483647LL, 2147483647LL);

IntBox::IntBox()
    : ll(0, 0), ur(-1, -1)
{
}

IntBox::IntBox(int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y)
    : ll(p_ll_x, p_ll_y), ur(p_ur_x, p_ur_y)
{
}

IntBox::IntBox(const IntPoint& p_ll, const IntPoint& p_ur)
    : ll(p_ll), ur(p_ur)
{
}

bool IntBox::is_empty() const
{
    return ll.x > ur.x || ll.y > ur.y;
}

bool IntBox::is_bounded() const
{
    return true;
}

int IntBox::dimension() const
{
    if (is_empty())
    {
        return -1;
    }
    if (ll == ur)
    {
        return 0;
    }
    if (ur.x == ll.x || ll.y == ur.y)
    {
        return 1;
    }
    return 2;
}

IntPoint IntBox::corner(int p_no) const
{
    switch (p_no)
    {
    case 0: return ll;
    case 1: return IntPoint(ur.x, ll.y);
    case 2: return ur;
    case 3: return IntPoint(ll.x, ur.y);
    default: throw std::out_of_range("IntBox.corner: p_no out of range");
    }
}

FloatPoint IntBox::corner_approx(int p_no) const
{
    return corner(p_no).to_float();
}

std::vector<FloatPoint> IntBox::corner_approx_arr() const
{
    std::vector<FloatPoint> result;
    result.reserve(4);
    for (int i = 0; i < 4; i++)
    {
        result.push_back(corner_approx(i));
    }
    return result;
}

bool IntBox::contains(const IntPoint& p_point) const
{
    return p_point.x >= ll.x && p_point.y >= ll.y &&
           p_point.x <= ur.x && p_point.y <= ur.y;
}

bool IntBox::contains(const IntBox& p_box) const
{
    return contains(p_box.ll) && contains(p_box.ur);
}

bool IntBox::intersects(const IntBox& p_box) const
{
    return !(ur.x < p_box.ll.x || ll.x > p_box.ur.x ||
             ur.y < p_box.ll.y || ll.y > p_box.ur.y);
}

bool IntBox::intersects_approx(const IntBox& p_box, double p_precision) const
{
    double min_dx = std::abs(static_cast<double>(ur.x - p_box.ll.x));
    double min_dy = std::abs(static_cast<double>(ur.y - p_box.ll.y));
    return min_dx < p_precision || min_dy < p_precision;
}

bool IntBox::overlaps(const IntBox& p_box) const
{
    if (p_box.ll.x >= ur.x) return false;
    if (p_box.ll.y >= ur.y) return false;
    if (ll.x >= p_box.ur.x) return false;
    return ll.y < p_box.ur.y;
}

IntPoint IntBox::center() const
{
    return IntPoint((ll.x + ur.x) / 2, (ll.y + ur.y) / 2);
}

int64_t IntBox::width() const
{
    return ur.x - ll.x;
}

int64_t IntBox::height() const
{
    return ur.y - ll.y;
}

double IntBox::area() const
{
    return static_cast<double>(ur.x - ll.x) * static_cast<double>(ur.y - ll.y);
}

double IntBox::max_width() const
{
    return static_cast<double>(std::max(ur.x - ll.x, ur.y - ll.y));
}

double IntBox::min_width() const
{
    return static_cast<double>(std::min(ur.x - ll.x, ur.y - ll.y));
}

IntBox IntBox::intersection(const IntBox& p_box) const
{
    int64_t new_ll_x = std::max(ll.x, p_box.ll.x);
    int64_t new_ll_y = std::max(ll.y, p_box.ll.y);
    int64_t new_ur_x = std::min(ur.x, p_box.ur.x);
    int64_t new_ur_y = std::min(ur.y, p_box.ur.y);
    if (new_ll_x > new_ur_x || new_ll_y > new_ur_y)
    {
        return EMPTY;
    }
    return IntBox(new_ll_x, new_ll_y, new_ur_x, new_ur_y);
}

IntBox IntBox::union_with(const IntBox& p_box) const
{
    int64_t new_ll_x = std::min(ll.x, p_box.ll.x);
    int64_t new_ll_y = std::min(ll.y, p_box.ll.y);
    int64_t new_ur_x = std::max(ur.x, p_box.ur.x);
    int64_t new_ur_y = std::max(ur.y, p_box.ur.y);
    return IntBox(new_ll_x, new_ll_y, new_ur_x, new_ur_y);
}

bool IntBox::operator==(const IntBox& other) const
{
    return ll == other.ll && ur == other.ur;
}

FloatPoint IntBox::centre_of_gravity() const
{
    return FloatPoint((ll.x + ur.x) / 2.0, (ll.y + ur.y) / 2.0);
}

std::unique_ptr<TileShape> IntBox::simplify() const
{
    return std::make_unique<IntBox>(*this);
}

std::unique_ptr<TileShape> IntBox::clone() const
{
    return std::make_unique<IntBox>(*this);
}

IntBox IntBox::translate_by(const IntVector& p_rel) const
{
    if (p_rel.x == 0 && p_rel.y == 0)
    {
        return *this;
    }
    return IntBox(IntPoint(ll.x + p_rel.x, ll.y + p_rel.y),
                  IntPoint(ur.x + p_rel.x, ur.y + p_rel.y));
}

IntBox IntBox::offset(double p_dist) const
{
    if (p_dist == 0 || is_empty())
    {
        return *this;
    }
    int64_t dist = static_cast<int64_t>(std::round(p_dist));
    return IntBox(IntPoint(ll.x - dist, ll.y - dist),
                  IntPoint(ur.x + dist, ur.y + dist));
}

IntOctagon IntBox::enlarge(double p_offset) const
{
    return to_IntOctagon().offset(p_offset);
}

IntBox IntBox::horizontal_offset(double p_dist) const
{
    if (p_dist == 0 || is_empty())
    {
        return *this;
    }
    int64_t dist = static_cast<int64_t>(std::round(p_dist));
    return IntBox(IntPoint(ll.x - dist, ll.y), IntPoint(ur.x + dist, ur.y));
}

IntBox IntBox::vertical_offset(double p_dist) const
{
    if (p_dist == 0 || is_empty())
    {
        return *this;
    }
    int64_t dist = static_cast<int64_t>(std::round(p_dist));
    return IntBox(IntPoint(ll.x, ll.y - dist), IntPoint(ur.x, ur.y + dist));
}

IntBox IntBox::shrink(int p_width) const
{
    int64_t ll_x = ll.x;
    int64_t ur_x = ur.x;
    if (2 * p_width <= ur.x - ll.x)
    {
        ll_x = ll.x + p_width;
        ur_x = ur.x - p_width;
    }
    else
    {
        ll_x = (ll.x + ur.x) / 2;
        ur_x = ll_x;
    }
    int64_t ll_y = ll.y;
    int64_t ur_y = ur.y;
    if (2 * p_width <= ur.y - ll.y)
    {
        ll_y = ll.y + p_width;
        ur_y = ur.y - p_width;
    }
    else
    {
        ll_y = (ll.y + ur.y) / 2;
        ur_y = ll_y;
    }
    return IntBox(ll_x, ll_y, ur_x, ur_y);
}

IntBox IntBox::turn_90_degree(int p_factor, const IntPoint& p_pole) const
{
    int n = p_factor;
    while (n < 0) n += 4;
    while (n >= 4) n -= 4;
    auto rotate_around_pole = [p_pole, n](const IntPoint& p)
    {
        int64_t dx = p.x - p_pole.x;
        int64_t dy = p.y - p_pole.y;
        int64_t new_x, new_y;
        switch (n)
        {
        case 0: new_x = dx; new_y = dy; break;
        case 1: new_x = -dy; new_y = dx; break;
        case 2: new_x = -dx; new_y = -dy; break;
        default: new_x = dy; new_y = -dx; break;
        }
        return IntPoint(p_pole.x + new_x, p_pole.y + new_y);
    };
    IntPoint p1 = rotate_around_pole(ll);
    IntPoint p2 = rotate_around_pole(ur);
    int64_t llx = std::min(p1.x, p2.x);
    int64_t lly = std::min(p1.y, p2.y);
    int64_t urx = std::max(p1.x, p2.x);
    int64_t ury = std::max(p1.y, p2.y);
    return IntBox(llx, lly, urx, ury);
}

Line IntBox::border_line(int p_no) const
{
    int64_t a_x, a_y, b_x, b_y;
    switch (p_no)
    {
    case 0:
        a_x = 0; a_y = ll.y; b_x = 1; b_y = ll.y; break;
    case 1:
        a_x = ur.x; a_y = 0; b_x = ur.x; b_y = 1; break;
    case 2:
        a_x = 0; a_y = ur.y; b_x = -1; b_y = ur.y; break;
    case 3:
        a_x = ll.x; a_y = 0; b_x = ll.x; b_y = -1; break;
    default: throw std::out_of_range("IntBox.edge_line: p_no out of range");
    }
    return Line(IntPoint(a_x, a_y), IntPoint(b_x, b_y));
}

int IntBox::border_line_index(const Line& /*p_line*/) const
{
    return -1;
}

bool IntBox::intersects(const IntOctagon& p_oct) const
{
    return p_oct.intersects(*this);
}

bool IntBox::intersects(const Simplex& p_simplex) const
{
    return p_simplex.intersects(*this);
}

bool IntBox::is_contained_in(const IntBox& p_other) const
{
    if (is_empty() || *this == p_other)
    {
        return true;
    }
    return ll.x >= p_other.ll.x && ll.y >= p_other.ll.y &&
           ur.x <= p_other.ur.x && ur.y <= p_other.ur.y;
}

bool IntBox::contains_in_interior(const IntBox& p_other) const
{
    if (p_other.is_empty())
    {
        return true;
    }
    return p_other.ll.x > ll.x && p_other.ll.y > ll.y &&
           p_other.ur.x < ur.x && p_other.ur.y < ur.y;
}

IntBox IntBox::nearest_part(const IntBox& p_from_box) const
{
    int64_t ll_x = (p_from_box.ll.x >= ll.x) ? p_from_box.ll.x
                                             : std::min(p_from_box.ur.x, ll.x);
    int64_t ur_x = (p_from_box.ur.x <= ur.x) ? p_from_box.ur.x
                                             : std::max(p_from_box.ll.x, ur.x);
    int64_t ll_y = (p_from_box.ll.y >= ll.y) ? p_from_box.ll.y
                                             : std::min(p_from_box.ur.y, ll.y);
    int64_t ur_y = (p_from_box.ur.y <= ur.y) ? p_from_box.ur.y
                                             : std::max(p_from_box.ll.y, ur.y);
    return IntBox(ll_x, ll_y, ur_x, ur_y);
}

double IntBox::weighted_distance(const IntBox& p_other, double p_horizontal_weight, double p_vertical_weight) const
{
    double max_ll_x = std::max(static_cast<double>(ll.x), static_cast<double>(p_other.ll.x));
    double max_ll_y = std::max(static_cast<double>(ll.y), static_cast<double>(p_other.ll.y));
    double min_ur_x = std::min(static_cast<double>(ur.x), static_cast<double>(p_other.ur.x));
    double min_ur_y = std::min(static_cast<double>(ur.y), static_cast<double>(p_other.ur.y));

    double result;
    if (min_ur_x >= max_ll_x)
    {
        result = std::max(p_vertical_weight * (max_ll_y - min_ur_y), 0.0);
    }
    else if (min_ur_y >= max_ll_y)
    {
        result = std::max(p_horizontal_weight * (max_ll_x - min_ur_x), 0.0);
    }
    else
    {
        double delta_x = (max_ll_x - min_ur_x) * p_horizontal_weight;
        double delta_y = (max_ll_y - min_ur_y) * p_vertical_weight;
        result = std::sqrt(delta_x * delta_x + delta_y * delta_y);
    }
    return result;
}

Side IntBox::compare(const IntBox& p_other, int p_edge_no) const
{
    switch (p_edge_no)
    {
    case 0:
        if (ll.y > p_other.ll.y) return Side::ON_THE_LEFT;
        if (ll.y < p_other.ll.y) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 1:
        if (ur.x < p_other.ur.x) return Side::ON_THE_LEFT;
        if (ur.x > p_other.ur.x) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 2:
        if (ur.y < p_other.ur.y) return Side::ON_THE_LEFT;
        if (ur.y > p_other.ur.y) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 3:
        if (ll.x > p_other.ll.x) return Side::ON_THE_LEFT;
        if (ll.x < p_other.ll.x) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    default:
        throw std::out_of_range("IntBox.compare: p_edge_no out of range");
    }
}

Side IntBox::compare(const IntOctagon& p_other, int p_edge_no) const
{
    return to_IntOctagon().compare(p_other, p_edge_no);
}

bool IntBox::is_contained_in(const IntOctagon& p_other) const
{
    return to_IntOctagon().is_contained_in(p_other);
}

std::unique_ptr<IntOctagon> IntBox::union_with(const IntOctagon& p_other) const
{
    return std::make_unique<IntOctagon>(p_other.union_with(to_IntOctagon()));
}

Side IntBox::compare(const RegularTileShape& p_other, int p_edge_no) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return compare(*oct, p_edge_no);
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return compare(*box, p_edge_no);
    }
    logger::FRLogger::instance().warn("IntBox.compare: unsupported RegularTileShape operand");
    return Side::NO_SIDE;
}

std::unique_ptr<RegularTileShape> IntBox::union_with(const RegularTileShape& p_other) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return union_with(*oct);
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return std::make_unique<IntBox>(union_with(*box));
    }
    logger::FRLogger::instance().warn("IntBox.union_with: unsupported RegularTileShape operand");
    return nullptr;
}

bool IntBox::contains(const RegularTileShape& p_other) const
{
    return p_other.is_contained_in(*this);
}

bool IntBox::intersects(const RegularTileShape& p_other) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return intersects(*oct);
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return intersects(*box);
    }
    logger::FRLogger::instance().warn("IntBox.intersects: unsupported RegularTileShape operand");
    return false;
}

IntOctagon IntBox::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return p_dirs.bounds(*this);
}

std::vector<IntBox> IntBox::divide_into_sections(double p_max_section_width) const
{
    std::vector<IntBox> result;
    if (p_max_section_width <= 0 || is_empty())
    {
        return result;
    }
    double length = static_cast<double>(ur.x - ll.x);
    double height = static_cast<double>(ur.y - ll.y);
    int x_count = static_cast<int>(std::ceil(length / p_max_section_width));
    int y_count = static_cast<int>(std::ceil(height / p_max_section_width));
    int section_length_x = static_cast<int>(std::ceil(length / x_count));
    int section_length_y = static_cast<int>(std::ceil(height / y_count));
    result.reserve(x_count * y_count);
    for (int j = 0; j < y_count; j++)
    {
        int64_t curr_lly = ll.y + j * section_length_y;
        int64_t curr_ury = (j == y_count - 1) ? ur.y : curr_lly + section_length_y;
        for (int i = 0; i < x_count; i++)
        {
            int64_t curr_llx = ll.x + i * section_length_x;
            int64_t curr_urx = (i == x_count - 1) ? ur.x : curr_llx + section_length_x;
            result.emplace_back(curr_llx, curr_lly, curr_urx, curr_ury);
        }
    }
    return result;
}

std::vector<IntBox> IntBox::cutout_from(const IntBox& p_d) const
{
    IntBox c = intersection(p_d);
    if (is_empty() || c.dimension() < dimension())
    {
        // there is only an overlap at the border
        return {p_d};
    }

    std::vector<IntBox> result;
    result.resize(4);
    result[0] = IntBox(p_d.ll.x, p_d.ll.y, c.ur.x, c.ll.y);
    result[1] = IntBox(p_d.ll.x, c.ll.y, c.ll.x, p_d.ur.y);
    result[2] = IntBox(c.ur.x, p_d.ll.y, p_d.ur.x, c.ur.y);
    result[3] = IntBox(c.ll.x, c.ur.y, p_d.ur.x, p_d.ur.y);

    // optimize the division so that the cumulative circumference is minimal
    IntBox b;
    if (c.ll.x - p_d.ll.x > c.ll.y - p_d.ll.y)
    {
        // switch left dividing line to lower
        b = result[0];
        result[0] = IntBox(c.ll.x, b.ll.y, b.ur.x, b.ur.y);
        b = result[1];
        result[1] = IntBox(b.ll.x, p_d.ll.y, b.ur.x, b.ur.y);
    }
    if (p_d.ur.y - c.ur.y > c.ll.x - p_d.ll.x)
    {
        // switch upper dividing line to the left
        b = result[1];
        result[1] = IntBox(b.ll.x, b.ll.y, b.ur.x, c.ur.y);
        b = result[3];
        result[3] = IntBox(p_d.ll.x, b.ll.y, b.ur.x, b.ur.y);
    }
    if (p_d.ur.x - c.ur.x > p_d.ur.y - c.ur.y)
    {
        // switch right dividing line to upper
        b = result[2];
        result[2] = IntBox(b.ll.x, b.ll.y, b.ur.x, p_d.ur.y);
        b = result[3];
        result[3] = IntBox(b.ll.x, b.ll.y, c.ur.x, b.ur.y);
    }
    if (c.ll.y - p_d.ll.y > p_d.ur.x - c.ur.x)
    {
        // switch lower dividing line to the left
        b = result[0];
        result[0] = IntBox(b.ll.x, b.ll.y, p_d.ur.x, b.ur.y);
        b = result[2];
        result[2] = IntBox(b.ll.x, c.ll.y, b.ur.x, b.ur.y);
    }
    return result;
}

std::vector<Simplex> IntBox::cutout_from(const Simplex& p_simplex) const
{
    return to_Simplex().cutout_from(p_simplex);
}

std::vector<IntOctagon> IntBox::cutout_from(const IntOctagon& p_oct) const
{
    return to_IntOctagon().cutout_from(p_oct);
}

std::string IntBox::to_string() const
{
    std::ostringstream ss;
    ss << "IntBox[" << ll.to_string() << ", " << ur.to_string() << "]";
    return ss.str();
}

IntOctagon IntBox::bounding_octagon() const
{
    return to_IntOctagon();
}

IntOctagon IntBox::to_IntOctagon() const
{
    // Java parity: IntOctagon(lx, ly, rx, uy, ulx, lrx, llx, urx)
    return IntOctagon(ll.x, ll.y, ur.x, ur.y,
                      ll.x - ur.y,   // ulx = upper left diagonal x-intercept
                      ur.x - ll.y,   // lrx = lower right diagonal x-intercept
                      ll.x + ll.y,   // llx = lower left diagonal x-intercept
                      ur.x + ur.y);  // urx = upper right diagonal x-intercept
}

Simplex IntBox::to_Simplex() const
{
    std::vector<Line> line_arr;
    if (!is_empty())
    {
        line_arr.reserve(4);
        line_arr.emplace_back(IntPoint(ll.x, ll.y), IntPoint(ll.x + 1, ll.y));   // bottom-right
        line_arr.emplace_back(IntPoint(ur.x, ur.y), IntPoint(ur.x, ur.y + 1));   // right-up
        line_arr.emplace_back(IntPoint(ur.x, ur.y), IntPoint(ur.x - 1, ur.y));   // top-left
        line_arr.emplace_back(IntPoint(ll.x, ll.y), IntPoint(ll.x, ll.y - 1));   // left-down
    }
    return Simplex(line_arr);
}

FloatPoint IntBox::nearest_point(const FloatPoint& p_from_point) const
{
    double x;
    if (p_from_point.x <= static_cast<double>(ll.x))
    {
        x = static_cast<double>(ll.x);
    }
    else if (p_from_point.x >= static_cast<double>(ur.x))
    {
        x = static_cast<double>(ur.x);
    }
    else
    {
        x = p_from_point.x;
    }

    double y;
    if (p_from_point.y <= static_cast<double>(ll.y))
    {
        y = static_cast<double>(ll.y);
    }
    else if (p_from_point.y >= static_cast<double>(ur.y))
    {
        y = static_cast<double>(ur.y);
    }
    else
    {
        y = p_from_point.y;
    }

    return FloatPoint(x, y);
}

std::vector<IntPoint> IntBox::nearest_border_projections(const IntPoint& p_point, int p_max_result_points) const
{
    if (p_max_result_points <= 0)
    {
        return {};
    }
    p_max_result_points = std::min(p_max_result_points, 2);
    std::vector<IntPoint> result(p_max_result_points);

    int64_t lower_x_diff = p_point.x - ll.x;
    int64_t upper_x_diff = ur.x - p_point.x;
    int64_t lower_y_diff = p_point.y - ll.y;
    int64_t upper_y_diff = ur.y - p_point.y;

    int64_t min_diff;
    int64_t second_min_diff;

    int64_t nearest_projection_x = p_point.x;
    int64_t nearest_projection_y = p_point.y;
    int64_t second_nearest_projection_x = p_point.x;
    int64_t second_nearest_projection_y = p_point.y;
    if (lower_x_diff <= upper_x_diff)
    {
        min_diff = lower_x_diff;
        second_min_diff = upper_x_diff;
        nearest_projection_x = ll.x;
        second_nearest_projection_x = ur.x;
    }
    else
    {
        min_diff = upper_x_diff;
        second_min_diff = lower_x_diff;
        nearest_projection_x = ur.x;
        second_nearest_projection_x = ll.x;
    }
    if (lower_y_diff < min_diff)
    {
        second_min_diff = min_diff;
        min_diff = lower_y_diff;
        second_nearest_projection_x = nearest_projection_x;
        second_nearest_projection_y = nearest_projection_y;
        nearest_projection_x = p_point.x;
        nearest_projection_y = ll.y;
    }
    else if (lower_y_diff < second_min_diff)
    {
        second_min_diff = lower_y_diff;
        second_nearest_projection_x = p_point.x;
        second_nearest_projection_y = ll.y;
    }
    if (upper_y_diff < min_diff)
    {
        second_min_diff = min_diff;
        min_diff = upper_y_diff;
        second_nearest_projection_x = nearest_projection_x;
        second_nearest_projection_y = nearest_projection_y;
        nearest_projection_x = p_point.x;
        nearest_projection_y = ur.y;
    }
    else if (upper_y_diff < second_min_diff)
    {
        second_min_diff = upper_y_diff;
        second_nearest_projection_x = p_point.x;
        second_nearest_projection_y = ur.y;
    }
    result[0] = IntPoint(nearest_projection_x, nearest_projection_y);
    if (result.size() > 1)
    {
        result[1] = IntPoint(second_nearest_projection_x, second_nearest_projection_y);
    }

    return result;
}

double IntBox::distance(const FloatPoint& p_from_point) const
{
    return p_from_point.distance(nearest_point(p_from_point));
}

bool IntBox::intersects(const Circle& p_other) const
{
    return p_other.intersects(*this);
}

} // namespace planar
} // namespace geometry
} // namespace freerouting