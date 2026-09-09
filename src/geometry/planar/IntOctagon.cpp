#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Direction.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace freerouting {
namespace geometry {
namespace planar {

const IntOctagon IntOctagon::EMPTY(
    0, 0, -1, -1,   // left, bottom, right, top (degenerate box)
    0, -1, 0, -1     // diagonals (degenerate)
);

IntOctagon::IntOctagon()
    : left(0), bottom(0), right(-1), top(-1),
      left_top(0), right_bottom(-1), left_bottom(0), right_top(-1)
{
}

IntOctagon::IntOctagon(int64_t p_left, int64_t p_bottom, int64_t p_right, int64_t p_top,
                       int64_t p_left_top, int64_t p_right_bottom,
                       int64_t p_left_bottom, int64_t p_right_top)
    : left(p_left), bottom(p_bottom), right(p_right), top(p_top),
      left_top(p_left_top), right_bottom(p_right_bottom),
      left_bottom(p_left_bottom), right_top(p_right_top)
{
}

bool IntOctagon::is_empty() const
{
    return right <= left || top <= bottom;
}

bool IntOctagon::is_bounded() const { return true; }

bool IntOctagon::is_IntBox() const
{
    return left_bottom == left + bottom &&
           right_bottom == right - bottom &&
           right_top == right + top &&
           left_top == left - top;
}

int IntOctagon::dimension() const
{
    if (is_empty()) return -1;
    if (right > left && top > bottom && right_bottom > left_top && right_top > left_bottom)
        return 2;
    if (right == left && top == bottom) return 0;
    return 1;
}

IntPoint IntOctagon::corner(int p_no) const
{
    int64_t x, y;
    switch (p_no)
    {
    case 0: x = left_bottom - bottom; y = bottom; break;
    case 1: x = right_bottom + bottom; y = bottom; break;
    case 2: x = right; y = right - right_bottom; break;
    case 3: x = right; y = right_top - right; break;
    case 4: x = right_top - top; y = top; break;
    case 5: x = left_top + top; y = top; break;
    case 6: x = left; y = left - left_top; break;
    case 7: x = left; y = left_bottom - left; break;
    default: throw std::out_of_range("IntOctagon.corner: p_no out of range");
    }
    return IntPoint(x, y);
}

FloatPoint IntOctagon::corner_approx(int p_no) const
{
    return corner(p_no).to_float();
}

std::vector<FloatPoint> IntOctagon::corner_approx_arr() const
{
    std::vector<FloatPoint> result;
    result.reserve(8);
    for (int i = 0; i < 8; i++)
    {
        result.push_back(corner_approx(i));
    }
    return result;
}

double IntOctagon::max_width() const
{
    double width_1 = std::max(static_cast<double>(right - left), static_cast<double>(top - bottom));
    double width_2 = std::max(static_cast<double>(right_top - left_bottom), static_cast<double>(right_bottom - left_top));
    return std::max(width_1, width_2 / Limits::SQRT2);
}

double IntOctagon::min_width() const
{
    double width_1 = std::min(static_cast<double>(right - left), static_cast<double>(top - bottom));
    double width_2 = std::min(static_cast<double>(right_top - left_bottom), static_cast<double>(right_bottom - left_top));
    return std::min(width_1, width_2 / Limits::SQRT2);
}

FloatPoint IntOctagon::centre_of_gravity() const
{
    int corner_count = border_line_count();
    double x = 0;
    double y = 0;
    for (int i = 0; i < corner_count; i++)
    {
        FloatPoint curr_point = corner_approx(i);
        x += curr_point.x;
        y += curr_point.y;
    }
    x /= corner_count;
    y /= corner_count;
    return FloatPoint(x, y);
}

double IntOctagon::circumference() const
{
    int corner_count = border_line_count();
    double result = 0;
    FloatPoint prev_corner = corner_approx(corner_count - 1);
    for (int i = 0; i < corner_count; i++)
    {
        FloatPoint curr_corner = corner_approx(i);
        result += curr_corner.distance(prev_corner);
        prev_corner = curr_corner;
    }
    return result;
}

int IntOctagon::border_line_index(const Line& /*p_line*/) const
{
    return -1;
}

Side IntOctagon::side_of_border_line(int p_x, int p_y, int p_border_line_no) const
{
    int64_t tmp;
    switch (p_border_line_no)
    {
    case 0: tmp = bottom - p_y; break;
    case 2: tmp = p_x - right; break;
    case 4: tmp = p_y - top; break;
    case 6: tmp = left - p_x; break;
    case 1: tmp = p_x - p_y - right_bottom; break;
    case 3: tmp = p_x + p_y - right_top; break;
    case 5: tmp = left_top + p_y - p_x; break;
    case 7: tmp = left_bottom - p_x - p_y; break;
    default: return Side::COLLINEAR;
    }
    if (tmp < 0) return Side::ON_THE_LEFT;
    if (tmp > 0) return Side::ON_THE_RIGHT;
    return Side::COLLINEAR;
}

IntOctagon IntOctagon::intersection_with_simplify(const IntOctagon& p_other) const
{
    IntOctagon result = intersection(p_other);
    return result;
}

std::unique_ptr<TileShape> IntOctagon::simplify() const
{
    if (this->is_IntBox())
    {
        return std::make_unique<IntBox>(this->bounding_box());
    }
    return std::make_unique<IntOctagon>(*this);
}

std::unique_ptr<TileShape> IntOctagon::clone() const
{
    return std::make_unique<IntOctagon>(*this);
}

int64_t IntOctagon::corner_x(int p_no) const
{
    switch (p_no)
    {
    case 0: return left_bottom - bottom;
    case 1: return right_bottom + bottom;
    case 2: case 3: return right;
    case 4: return right_top - top;
    case 5: return left_top + top;
    case 6: case 7: return left;
    default: throw std::out_of_range("IntOctagon.corner_x: p_no out of range");
    }
}

int64_t IntOctagon::corner_y(int p_no) const
{
    switch (p_no)
    {
    case 0: case 1: return bottom;
    case 2: return right - right_bottom;
    case 3: return right_top - right;
    case 4: case 5: return top;
    case 6: return left - left_top;
    case 7: return left_bottom - left;
    default: throw std::out_of_range("IntOctagon.corner_y: p_no out of range");
    }
}

double IntOctagon::area() const
{
    double result =
        static_cast<double>(left_bottom - bottom) * static_cast<double>(bottom - left_bottom + left)
      + static_cast<double>(right_bottom + bottom) * static_cast<double>(right - right_bottom - bottom)
      + static_cast<double>(right) * static_cast<double>(right_top - 2 * right - bottom + top + right_bottom)
      + static_cast<double>(right_top - top) * static_cast<double>(top - right_top + right)
      + static_cast<double>(left_top + top) * static_cast<double>(left - left_top - top)
      + static_cast<double>(left) * static_cast<double>(left_bottom - 2 * left - top + bottom + left_top);
    return 0.5 * std::abs(result);
}

Line IntOctagon::border_line(int p_no) const
{
    int64_t a_x, a_y, b_x, b_y;
    switch (p_no)
    {
    case 0: // lower boundary
        a_x = 0; a_y = bottom; b_x = 1; b_y = bottom; break;
    case 1: // lower right boundary
        a_x = right_bottom; a_y = 0; b_x = right_bottom + 1; b_y = 1; break;
    case 2: // right boundary
        a_x = right; a_y = 0; b_x = right; b_y = 1; break;
    case 3: // upper right boundary
        a_x = right_top; a_y = 0; b_x = right_top - 1; b_y = 1; break;
    case 4: // upper boundary
        a_x = 0; a_y = top; b_x = -1; b_y = top; break;
    case 5: // upper left boundary
        a_x = left_top; a_y = 0; b_x = left_top - 1; b_y = -1; break;
    case 6: // left boundary
        a_x = left; a_y = 0; b_x = left; b_y = -1; break;
    case 7: // lower left boundary
        a_x = left_bottom; a_y = 0; b_x = left_bottom + 1; b_y = -1; break;
    default: throw std::out_of_range("IntOctagon.border_line: p_no out of range");
    }
    return Line(IntPoint(a_x, a_y), IntPoint(b_x, b_y));
}

IntBox IntOctagon::bounding_box() const
{
    return IntBox(left, bottom, right, top);
}

bool IntOctagon::contains(const FloatPoint& p_point) const
{
    if (left > p_point.x || bottom > p_point.y || right < p_point.x || top < p_point.y)
        return false;
    double tmp1 = p_point.x - p_point.y;
    double tmp2 = p_point.x + p_point.y;
    return !(left_top > tmp1) && !(right_bottom < tmp1) &&
           !(left_bottom > tmp2) && !(right_top < tmp2);
}

bool IntOctagon::contains(const IntPoint& p_point) const
{
    return contains(FloatPoint(static_cast<double>(p_point.x), static_cast<double>(p_point.y)));
}

bool IntOctagon::is_contained_in(const IntBox& p_box) const
{
    return left >= p_box.ll.x && bottom >= p_box.ll.y &&
           right <= p_box.ur.x && top <= p_box.ur.y;
}

bool IntOctagon::is_contained_in(const IntOctagon& p_other) const
{
    return left >= p_other.left && bottom >= p_other.bottom &&
           right <= p_other.right && top <= p_other.top &&
           left_bottom >= p_other.left_bottom && left_top >= p_other.left_top &&
           right_bottom <= p_other.right_bottom && right_top <= p_other.right_top;
}

IntOctagon IntOctagon::intersection(const IntOctagon& p_other) const
{
    IntOctagon result(
        std::max(left, p_other.left),
        std::max(bottom, p_other.bottom),
        std::min(right, p_other.right),
        std::min(top, p_other.top),
        std::max(left_top, p_other.left_top),
        std::min(right_bottom, p_other.right_bottom),
        std::max(left_bottom, p_other.left_bottom),
        std::min(right_top, p_other.right_top));
    return result.normalize();
}

IntOctagon IntOctagon::intersection(const IntBox& p_other) const
{
    return intersection(p_other.to_IntOctagon());
}

IntOctagon IntOctagon::union_with(const IntOctagon& p_other) const
{
    return IntOctagon(
        std::min(left, p_other.left),
        std::min(bottom, p_other.bottom),
        std::max(right, p_other.right),
        std::max(top, p_other.top),
        std::min(left_top, p_other.left_top),
        std::max(right_bottom, p_other.right_bottom),
        std::min(left_bottom, p_other.left_bottom),
        std::max(right_top, p_other.right_top));
}

IntOctagon IntOctagon::union_with(const IntBox& p_other) const
{
    return union_with(p_other.to_IntOctagon());
}

bool IntOctagon::intersects(const IntOctagon& p_other) const
{
    if (std::max(p_other.left, left) > std::min(p_other.right, right)) return false;
    if (std::max(p_other.bottom, bottom) > std::min(p_other.top, top)) return false;
    if (std::max(p_other.left_bottom, left_bottom) > std::min(p_other.right_top, right_top)) return false;
    return std::max(p_other.left_top, left_top) <= std::min(p_other.right_bottom, right_bottom);
}

bool IntOctagon::intersects(const IntBox& p_other) const
{
    return intersects(p_other.to_IntOctagon());
}

bool IntOctagon::overlaps(const IntOctagon& p_other) const
{
    if (std::max(p_other.left, left) >= std::min(p_other.right, right)) return false;
    if (std::max(p_other.bottom, bottom) >= std::min(p_other.top, top)) return false;
    if (std::max(p_other.left_bottom, left_bottom) >= std::min(p_other.right_top, right_top)) return false;
    return std::max(p_other.left_top, left_top) < std::min(p_other.right_bottom, right_bottom);
}

IntOctagon IntOctagon::translate_by(const IntVector& p_rel) const
{
    return IntOctagon(
        left + p_rel.x, bottom + p_rel.y, right + p_rel.x, top + p_rel.y,
        left_top + p_rel.x - p_rel.y,
        right_bottom + p_rel.x - p_rel.y,
        left_bottom + p_rel.x + p_rel.y,
        right_top + p_rel.x + p_rel.y);
}

IntOctagon IntOctagon::offset(double p_distance) const
{
    int64_t width = static_cast<int64_t>(std::floor(p_distance + 0.5));
    if (width == 0) return *this;
    int64_t dia_width = static_cast<int64_t>(std::floor(Limits::SQRT2 * p_distance + 0.5));
    return IntOctagon(
        left - width, bottom - width, right + width, top + width,
        left_top - dia_width, right_bottom + dia_width,
        left_bottom - dia_width, right_top + dia_width).normalize();
}

IntOctagon IntOctagon::normalize() const
{
    if (left > right || bottom > top || left_bottom > right_top || left_top > right_bottom)
        return EMPTY;

    int64_t new_lx = left;
    int64_t new_rx = right;
    int64_t new_ly = bottom;
    int64_t new_uy = top;
    int64_t new_llx = left_bottom;
    int64_t new_ulx = left_top;
    int64_t new_lrx = right_bottom;
    int64_t new_urx = right_top;

    if (new_lx < new_llx - new_uy) new_lx = new_llx - new_uy;
    if (new_lx < new_ulx + new_ly) new_lx = new_ulx + new_ly;
    if (new_rx > new_urx - new_ly) new_rx = new_urx - new_ly;
    if (new_rx > new_lrx + new_uy) new_rx = new_lrx + new_uy;
    if (new_ly < new_lx - new_lrx) new_ly = new_lx - new_lrx;
    if (new_ly < new_llx - new_rx) new_ly = new_llx - new_rx;
    if (new_uy > new_urx - new_lx) new_uy = new_urx - new_lx;
    if (new_uy > new_rx - new_ulx) new_uy = new_rx - new_ulx;
    if (new_llx - new_lx < new_ly) new_llx = new_lx + new_ly;
    if (new_rx - new_lrx < new_ly) new_lrx = new_rx - new_ly;
    if (new_urx - new_rx > new_uy) new_urx = new_uy + new_rx;
    if (new_lx - new_ulx > new_uy) new_ulx = new_lx - new_uy;

    int64_t diag_upper_y = static_cast<int64_t>(std::ceil(static_cast<double>(new_urx - new_ulx) / 2.0));
    if (new_uy > diag_upper_y) new_uy = diag_upper_y;

    int64_t diag_lower_y = static_cast<int64_t>(std::floor(static_cast<double>(new_llx - new_lrx) / 2.0));
    if (new_ly < diag_lower_y) new_ly = diag_lower_y;

    int64_t diag_right_x = static_cast<int64_t>(std::ceil(static_cast<double>(new_urx + new_lrx) / 2.0));
    if (new_rx > diag_right_x) new_rx = diag_right_x;

    int64_t diag_left_x = static_cast<int64_t>(std::floor(static_cast<double>(new_llx + new_ulx) / 2.0));
    if (new_lx < diag_left_x) new_lx = diag_left_x;

    if (new_lx > new_rx || new_ly > new_uy || new_llx > new_urx || new_ulx > new_lrx)
        return EMPTY;

    return IntOctagon(new_lx, new_ly, new_rx, new_uy, new_ulx, new_lrx, new_llx, new_urx);
}

bool IntOctagon::is_normalized() const
{
    IntOctagon on = normalize();
    return left == on.left && bottom == on.bottom && right == on.right && top == on.top &&
           left_bottom == on.left_bottom && right_bottom == on.right_bottom &&
           left_top == on.left_top && right_top == on.right_top;
}

int64_t IntOctagon::left_x_value(int64_t p_y) const
{
    int64_t result = std::max(left, left_top + p_y);
    return std::max(result, left_bottom - p_y);
}

int64_t IntOctagon::right_x_value(int64_t p_y) const
{
    int64_t result = std::min(right, right_top - p_y);
    return std::min(result, right_bottom + p_y);
}

int64_t IntOctagon::lower_y_value(int64_t p_x) const
{
    int64_t result = std::max(bottom, left_bottom - p_x);
    return std::max(result, p_x - right_bottom);
}

int64_t IntOctagon::upper_y_value(int64_t p_x) const
{
    int64_t result = std::min(top, p_x - left_top);
    return std::min(result, right_top - p_x);
}

Side IntOctagon::compare(const IntOctagon& p_other, int p_edge_no) const
{
    switch (p_edge_no)
    {
    case 0:
        if (bottom > p_other.bottom) return Side::ON_THE_LEFT;
        if (bottom < p_other.bottom) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 1:
        if (right_bottom < p_other.right_bottom) return Side::ON_THE_LEFT;
        if (right_bottom > p_other.right_bottom) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 2:
        if (right < p_other.right) return Side::ON_THE_LEFT;
        if (right > p_other.right) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 3:
        if (right_top < p_other.right_top) return Side::ON_THE_LEFT;
        if (right_top > p_other.right_top) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 4:
        if (top < p_other.top) return Side::ON_THE_LEFT;
        if (top > p_other.top) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 5:
        if (left_top > p_other.left_top) return Side::ON_THE_LEFT;
        if (left_top < p_other.left_top) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 6:
        if (left > p_other.left) return Side::ON_THE_LEFT;
        if (left < p_other.left) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    case 7:
        if (left_bottom > p_other.left_bottom) return Side::ON_THE_LEFT;
        if (left_bottom < p_other.left_bottom) return Side::ON_THE_RIGHT;
        return Side::COLLINEAR;
    default:
        throw std::out_of_range("IntOctagon.compare: p_edge_no out of range");
    }
}

Side IntOctagon::compare(const IntBox& p_other, int p_edge_no) const
{
    return compare(p_other.to_IntOctagon(), p_edge_no);
}

Side IntOctagon::compare(const RegularTileShape& p_other, int p_edge_no) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return compare(*oct, p_edge_no);
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return compare(*box, p_edge_no);
    }
    logger::FRLogger::instance().warn("IntOctagon.compare: unsupported RegularTileShape operand");
    return Side::NO_SIDE;
}

std::unique_ptr<RegularTileShape> IntOctagon::union_with(const RegularTileShape& p_other) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return std::make_unique<IntOctagon>(union_with(*oct));
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return std::make_unique<IntOctagon>(union_with(*box));
    }
    logger::FRLogger::instance().warn("IntOctagon.union_with: unsupported RegularTileShape operand");
    return nullptr;
}

bool IntOctagon::contains(const RegularTileShape& p_other) const
{
    return p_other.is_contained_in(*this);
}

bool IntOctagon::intersects(const RegularTileShape& p_other) const
{
    if (const auto* oct = dynamic_cast<const IntOctagon*>(&p_other))
    {
        return intersects(*oct);
    }
    if (const auto* box = dynamic_cast<const IntBox*>(&p_other))
    {
        return intersects(*box);
    }
    logger::FRLogger::instance().warn("IntOctagon.intersects: unsupported RegularTileShape operand");
    return false;
}

IntOctagon IntOctagon::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return p_dirs.bounds(*this);
}

IntPoint IntOctagon::border_point(const IntPoint& p_point, FortyfiveDegreeDirection p_dir) const
{
    int64_t result_x, result_y;
    switch (p_dir)
    {
    case FortyfiveDegreeDirection::RIGHT:
        result_x = std::min(right, std::min(right_top - p_point.y, right_bottom + p_point.y));
        result_y = p_point.y;
        break;
    case FortyfiveDegreeDirection::LEFT:
        result_x = std::max(left, std::max(left_top + p_point.y, left_bottom - p_point.y));
        result_y = p_point.y;
        break;
    case FortyfiveDegreeDirection::UP:
        result_x = p_point.x;
        result_y = std::min(top, std::min(p_point.x - left_top, right_top - p_point.x));
        break;
    case FortyfiveDegreeDirection::DOWN:
        result_x = p_point.x;
        result_y = std::max(bottom, std::max(left_bottom - p_point.x, p_point.x - right_bottom));
        break;
    case FortyfiveDegreeDirection::RIGHT45:
        result_x = static_cast<int64_t>(std::ceil(0.5 * (p_point.x - p_point.y + right_top)));
        result_x = std::min(result_x, right);
        result_x = std::min(result_x, p_point.x - p_point.y + top);
        result_y = p_point.y - p_point.x + result_x;
        break;
    case FortyfiveDegreeDirection::UP45:
        result_x = static_cast<int64_t>(std::floor(0.5 * (p_point.x + p_point.y + left_top)));
        result_x = std::max(result_x, left);
        result_x = std::max(result_x, p_point.x + p_point.y - top);
        result_y = p_point.y + p_point.x - result_x;
        break;
    case FortyfiveDegreeDirection::LEFT45:
        result_x = static_cast<int64_t>(std::floor(0.5 * (p_point.x - p_point.y + left_bottom)));
        result_x = std::max(result_x, left);
        result_x = std::max(result_x, p_point.x - p_point.y + bottom);
        result_y = p_point.y - p_point.x + result_x;
        break;
    case FortyfiveDegreeDirection::DOWN45:
        result_x = static_cast<int64_t>(std::ceil(0.5 * (p_point.x + p_point.y + right_bottom)));
        result_x = std::min(result_x, right);
        result_x = std::min(result_x, p_point.x + p_point.y - bottom);
        result_y = p_point.y + p_point.x - result_x;
        break;
    default:
        result_x = 0;
        result_y = 0;
        break;
    }
    return IntPoint(result_x, result_y);
}

std::vector<IntOctagon> IntOctagon::cutout_from(const IntOctagon& p_d) const
{
    IntOctagon c = intersection(p_d);

    if (is_empty() || c.dimension() < dimension())
    {
        // there is only an overlap at the border
        return {p_d};
    }

    std::vector<IntOctagon> result;
    result.resize(8);

    int64_t tmp = c.left_bottom - c.left;
    result[0] = IntOctagon(p_d.left, tmp, c.left, c.left - c.left_top,
                           p_d.left_top, p_d.right_bottom, p_d.left_bottom, p_d.right_top);

    int64_t tmp2 = c.left_bottom - c.bottom;
    result[1] = IntOctagon(p_d.left, p_d.bottom, tmp2, tmp,
                           p_d.left_top, p_d.right_bottom, p_d.left_bottom, c.left_bottom);

    tmp = c.right_bottom + c.bottom;
    result[2] = IntOctagon(tmp2, p_d.bottom, tmp, c.bottom,
                           p_d.left_top, p_d.right_bottom, p_d.left_bottom, p_d.right_top);

    tmp2 = c.right - c.right_bottom;
    result[3] = IntOctagon(tmp, p_d.bottom, p_d.right, tmp2,
                           c.right_bottom, p_d.right_bottom, p_d.left_bottom, p_d.right_top);

    tmp = c.right_top - c.right;
    result[4] = IntOctagon(c.right, tmp2, p_d.right, tmp,
                           p_d.left_top, p_d.right_bottom, p_d.left_bottom, p_d.right_top);

    tmp2 = c.right_top - c.top;
    result[5] = IntOctagon(tmp2, tmp, p_d.right, p_d.top,
                           p_d.left_top, p_d.right_bottom, c.right_top, p_d.right_top);

    tmp = c.left_top + c.top;
    result[6] = IntOctagon(tmp, c.top, tmp2, p_d.top,
                           p_d.left_top, p_d.right_bottom, p_d.left_bottom, p_d.right_top);

    tmp2 = c.left - c.left_top;
    result[7] = IntOctagon(p_d.left, tmp2, tmp, p_d.top,
                           p_d.left_top, c.left_top, p_d.left_bottom, p_d.right_top);

    for (auto& piece : result)
    {
        piece = piece.normalize();
    }

    IntOctagon curr_1;
    IntOctagon curr_2;

    curr_1 = result[0];
    curr_2 = result[7];

    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_1.right - curr_1.left_x_value(curr_1.top) > curr_2.upper_y_value(curr_1.right) - curr_2.bottom)
    {
        // switch the horizontal upper left divide line to vertical
        curr_1 = IntOctagon(std::min(curr_1.left, curr_2.left), curr_1.bottom, curr_1.right, curr_2.top,
                            curr_2.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_2.right_top);
        curr_2 = IntOctagon(curr_1.right, curr_2.bottom, curr_2.right, curr_2.top,
                            curr_2.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_2.right_top);
        result[0] = curr_1.normalize();
        result[7] = curr_2.normalize();
    }
    curr_1 = result[7];
    curr_2 = result[6];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_2.upper_y_value(curr_1.right) - curr_2.bottom > curr_1.right - curr_1.left_x_value(curr_2.bottom))
    {
        // switch the vertical upper left divide line to horizontal
        curr_2 = IntOctagon(curr_1.left, curr_2.bottom, curr_2.right, std::max(curr_2.top, curr_1.top),
                            curr_1.left_top, curr_2.right_bottom, curr_1.left_bottom, curr_2.right_top);
        curr_1 = IntOctagon(curr_1.left, curr_1.bottom, curr_1.right, curr_2.bottom,
                            curr_1.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_1.right_top);
        result[7] = curr_1.normalize();
        result[6] = curr_2.normalize();
    }
    curr_1 = result[6];
    curr_2 = result[5];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_2.upper_y_value(curr_1.right) - curr_1.bottom > curr_2.right_x_value(curr_1.bottom) - curr_2.left)
    {
        // switch the vertical upper right divide line to horizontal
        curr_1 = IntOctagon(curr_1.left, curr_1.bottom, curr_2.right, std::max(curr_2.top, curr_1.top),
                            curr_1.left_top, curr_2.right_bottom, curr_1.left_bottom, curr_2.right_top);
        curr_2 = IntOctagon(curr_2.left, curr_2.bottom, curr_2.right, curr_1.bottom,
                            curr_2.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_2.right_top);
        result[6] = curr_1.normalize();
        result[5] = curr_2.normalize();
    }
    curr_1 = result[5];
    curr_2 = result[4];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_2.right_x_value(curr_2.top) - curr_2.left > curr_1.upper_y_value(curr_2.left) - curr_2.top)
    {
        // switch the horizontal upper right divide line to vertical
        curr_2 = IntOctagon(curr_2.left, curr_2.bottom, std::max(curr_2.right, curr_1.right), curr_1.top,
                            curr_1.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_1.right_top);
        curr_1 = IntOctagon(curr_1.left, curr_1.bottom, curr_2.left, curr_1.top,
                            curr_1.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_1.right_top);
        result[5] = curr_1.normalize();
        result[4] = curr_2.normalize();
    }
    curr_1 = result[4];
    curr_2 = result[3];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_1.right_x_value(curr_1.bottom) - curr_1.left > curr_1.bottom - curr_2.lower_y_value(curr_1.left))
    {
        // switch the horizontal lower right divide line to vertical
        curr_1 = IntOctagon(curr_1.left, curr_2.bottom, std::max(curr_2.right, curr_1.right), curr_1.top,
                            curr_1.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_1.right_top);
        curr_2 = IntOctagon(curr_2.left, curr_2.bottom, curr_1.left, curr_2.top,
                            curr_2.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_2.right_top);
        result[4] = curr_1.normalize();
        result[3] = curr_2.normalize();
    }
    curr_1 = result[3];
    curr_2 = result[2];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_2.top - curr_2.lower_y_value(curr_2.right) > curr_1.right_x_value(curr_2.top) - curr_2.right)
    {
        // switch the vertical lower right divide line to horizontal
        curr_2 = IntOctagon(curr_2.left, std::min(curr_1.bottom, curr_2.bottom), curr_1.right, curr_2.top,
                            curr_2.left_top, curr_1.right_bottom, curr_2.left_bottom, curr_1.right_top);
        curr_1 = IntOctagon(curr_1.left, curr_2.top, curr_1.right, curr_1.top,
                            curr_1.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_1.right_top);
        result[3] = curr_1.normalize();
        result[2] = curr_2.normalize();
    }
    curr_1 = result[2];
    curr_2 = result[1];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_1.top - curr_1.lower_y_value(curr_1.left) > curr_1.left - curr_2.left_x_value(curr_1.top))
    {
        // switch the vertical lower left divide line to horizontal
        curr_1 = IntOctagon(curr_2.left, std::min(curr_1.bottom, curr_2.bottom), curr_1.right, curr_1.top,
                            curr_2.left_top, curr_1.right_bottom, curr_2.left_bottom, curr_1.right_top);
        curr_2 = IntOctagon(curr_2.left, curr_1.top, curr_2.right, curr_2.top,
                            curr_2.left_top, curr_2.right_bottom, curr_2.left_bottom, curr_2.right_top);
        result[2] = curr_1.normalize();
        result[1] = curr_2.normalize();
    }
    curr_1 = result[1];
    curr_2 = result[0];
    if (!(curr_1.is_empty() || curr_2.is_empty()) &&
        curr_2.right - curr_2.left_x_value(curr_2.bottom) > curr_2.bottom - curr_1.lower_y_value(curr_2.right))
    {
        // switch the horizontal lower left divide line to vertical
        curr_2 = IntOctagon(std::min(curr_2.left, curr_1.left), curr_1.bottom, curr_2.right, curr_2.top,
                            curr_2.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_2.right_top);
        curr_1 = IntOctagon(curr_2.right, curr_1.bottom, curr_1.right, curr_1.top,
                            curr_1.left_top, curr_1.right_bottom, curr_1.left_bottom, curr_1.right_top);
        result[1] = curr_1.normalize();
        result[0] = curr_2.normalize();
    }

    return result;
}

std::vector<Simplex> IntOctagon::cutout_from(const Simplex& p_simplex) const
{
    return to_Simplex().cutout_from(p_simplex);
}

std::vector<IntOctagon> IntOctagon::cutout_from(const IntBox& p_d) const
{
    IntOctagon c = intersection(p_d);

    if (is_empty() || c.dimension() < dimension())
    {
        // there is only an overlap at the border
        return {p_d.to_IntOctagon()};
    }

    IntBox boxes[4];

    // construct left box
    boxes[0] = IntBox(p_d.ll.x, c.left_bottom - c.left, c.left, c.left - c.left_top);

    // construct right box
    boxes[1] = IntBox(c.right, c.right - c.right_bottom, p_d.ur.x, c.right_top - c.right);

    // construct lower box
    boxes[2] = IntBox(c.left_bottom - c.bottom, p_d.ll.y, c.right_bottom + c.bottom, c.bottom);

    // construct upper box
    boxes[3] = IntBox(c.left_top + c.top, c.top, c.right_top - c.top, p_d.ur.y);

    IntOctagon octagons[4];

    // construct upper left octagon
    IntOctagon curr_oct(p_d.ll.x, boxes[0].ur.y, boxes[3].ll.x, p_d.ur.y,
                        -Limits::CRIT_INT, c.left_top, -Limits::CRIT_INT, Limits::CRIT_INT);
    octagons[0] = curr_oct.normalize();

    // construct lower left octagon
    curr_oct = IntOctagon(p_d.ll.x, p_d.ll.y, boxes[2].ll.x, boxes[0].ll.y,
                          -Limits::CRIT_INT, Limits::CRIT_INT, -Limits::CRIT_INT, c.left_bottom);
    octagons[1] = curr_oct.normalize();

    // construct lower right octagon
    curr_oct = IntOctagon(boxes[2].ur.x, p_d.ll.y, p_d.ur.x, boxes[1].ll.y,
                          c.right_bottom, Limits::CRIT_INT, -Limits::CRIT_INT, Limits::CRIT_INT);
    octagons[2] = curr_oct.normalize();

    // construct upper right octagon
    curr_oct = IntOctagon(boxes[3].ur.x, boxes[1].ur.y, p_d.ur.x, p_d.ur.y,
                          -Limits::CRIT_INT, Limits::CRIT_INT, c.right_top, Limits::CRIT_INT);
    octagons[3] = curr_oct.normalize();

    // optimise the result to minimum cumulative circumference

    IntBox b = boxes[0];
    IntOctagon o = octagons[0];
    if (b.ur.x - b.ll.x > o.top - o.bottom)
    {
        // switch the horizontal upper left divide line to vertical
        boxes[0] = IntBox(b.ll.x, b.ll.y, b.ur.x, o.top);
        curr_oct = IntOctagon(b.ur.x, o.bottom, o.right, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[0] = curr_oct.normalize();
    }

    b = boxes[3];
    o = octagons[0];
    if (b.ur.y - b.ll.y > o.right - o.left)
    {
        // switch the vertical upper left divide line to horizontal
        boxes[3] = IntBox(o.left, b.ll.y, b.ur.x, b.ur.y);
        curr_oct = IntOctagon(o.left, o.bottom, o.right, b.ll.y,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[0] = curr_oct.normalize();
    }
    b = boxes[3];
    o = octagons[3];
    if (b.ur.y - b.ll.y > o.right - o.left)
    {
        // switch the vertical upper right divide line to horizontal
        boxes[3] = IntBox(b.ll.x, b.ll.y, o.right, b.ur.y);
        curr_oct = IntOctagon(o.left, o.bottom, o.right, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[3] = curr_oct.normalize();
    }
    b = boxes[1];
    o = octagons[3];
    if (b.ur.x - b.ll.x > o.top - o.bottom)
    {
        // switch the horizontal upper right divide line to vertical
        boxes[1] = IntBox(b.ll.x, b.ll.y, b.ur.x, o.top);
        curr_oct = IntOctagon(o.left, o.bottom, b.ll.x, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[3] = curr_oct.normalize();
    }
    b = boxes[1];
    o = octagons[2];
    if (b.ur.x - b.ll.x > o.top - o.bottom)
    {
        // switch the horizontal lower right divide line to vertical
        boxes[1] = IntBox(b.ll.x, o.bottom, b.ur.x, b.ur.y);
        curr_oct = IntOctagon(o.left, o.bottom, b.ll.x, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[2] = curr_oct.normalize();
    }
    b = boxes[2];
    o = octagons[2];
    if (b.ur.y - b.ll.y > o.right - o.left)
    {
        // switch the vertical lower right divide line to horizontal
        boxes[2] = IntBox(b.ll.x, b.ll.y, o.right, b.ur.y);
        curr_oct = IntOctagon(o.left, b.ur.y, o.right, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[2] = curr_oct.normalize();
    }
    b = boxes[2];
    o = octagons[1];
    if (b.ur.y - b.ll.y > o.right - o.left)
    {
        // switch the vertical lower left divide line to horizontal
        boxes[2] = IntBox(o.left, b.ll.y, b.ur.x, b.ur.y);
        curr_oct = IntOctagon(o.left, b.ur.y, o.right, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[1] = curr_oct.normalize();
    }
    b = boxes[0];
    o = octagons[1];
    if (b.ur.x - b.ll.x > o.top - o.bottom)
    {
        // switch the horizontal lower left divide line to vertical
        boxes[0] = IntBox(b.ll.x, o.bottom, b.ur.x, b.ur.y);
        curr_oct = IntOctagon(b.ur.x, o.bottom, o.right, o.top,
                              o.left_top, o.right_bottom, o.left_bottom, o.right_top);
        octagons[1] = curr_oct.normalize();
    }

    std::vector<IntOctagon> result;
    result.resize(8);

    // add the 4 boxes to the result
    for (int i = 0; i < 4; ++i)
    {
        result[i] = boxes[i].to_IntOctagon();
    }

    // add the 4 octagons to the result
    for (int i = 0; i < 4; ++i)
    {
        result[4 + i] = octagons[i];
    }
    return result;
}

bool IntOctagon::operator==(const IntOctagon& other) const
{
    return left == other.left && bottom == other.bottom && right == other.right &&
           top == other.top && left_top == other.left_top &&
           right_bottom == other.right_bottom && left_bottom == other.left_bottom &&
           right_top == other.right_top;
}

std::string IntOctagon::to_string() const
{
    std::ostringstream ss;
    ss << "IntOctagon[l=" << left << ",b=" << bottom << ",r=" << right << ",t=" << top
       << ",ul=" << left_top << ",lr=" << right_bottom
       << ",ll=" << left_bottom << ",ur=" << right_top << "]";
    return ss.str();
}

Simplex IntOctagon::to_Simplex() const
{
    if (is_empty())
    {
        return Simplex::EMPTY;
    }
    std::vector<Line> line_arr;
    line_arr.reserve(8);
    for (int i = 0; i < 8; i++)
    {
        line_arr.push_back(border_line(i));
    }
    Simplex curr_simplex(line_arr);
    return curr_simplex.remove_redundant_lines();
}

std::vector<IntPoint> IntOctagon::nearest_border_projections(const IntPoint& p_point, int p_max_result_points) const
{
    if (!contains(p_point) || p_max_result_points <= 0)
    {
        return {};
    }
    p_max_result_points = std::min(p_max_result_points, 8);
    std::vector<IntPoint> result(p_max_result_points);
    std::vector<double> min_dist(p_max_result_points, std::numeric_limits<double>::max());

    const FloatPoint inside_point = p_point.to_float();
    static const std::array<FortyfiveDegreeDirection, 8> directions = {
        FortyfiveDegreeDirection::RIGHT, FortyfiveDegreeDirection::RIGHT45,
        FortyfiveDegreeDirection::UP, FortyfiveDegreeDirection::UP45,
        FortyfiveDegreeDirection::LEFT, FortyfiveDegreeDirection::LEFT45,
        FortyfiveDegreeDirection::DOWN, FortyfiveDegreeDirection::DOWN45};

    for (const FortyfiveDegreeDirection curr_dir : directions)
    {
        const IntPoint curr_border_point = border_point(p_point, curr_dir);
        const double curr_dist = inside_point.distance_square(curr_border_point.to_float());
        for (int i = 0; i < p_max_result_points; i++)
        {
            if (curr_dist < min_dist[i])
            {
                for (int k = p_max_result_points - 1; k > i; k--)
                {
                    min_dist[k] = min_dist[k - 1];
                    result[k] = result[k - 1];
                }
                min_dist[i] = curr_dist;
                result[i] = curr_border_point;
                break;
            }
        }
    }

    return result;
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
