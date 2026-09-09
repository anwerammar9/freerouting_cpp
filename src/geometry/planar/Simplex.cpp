#include "geometry/planar/Simplex.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Limits.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace freerouting {
namespace geometry {
namespace planar {

namespace {
// Java parity: Integer.MAX_VALUE is used as a sentinel for unbounded corners
// and as an "infinite" distance in calc_division_lines.
constexpr int64_t INTEGER_MAX_VALUE = 2147483647;
}

const Simplex Simplex::EMPTY{std::vector<Line>()};

Simplex::Simplex(const std::vector<Line>& p_lines)
    : lines(p_lines)
{
}

Simplex Simplex::get_instance(std::vector<Line> p_line_arr)
{
    if (p_line_arr.empty()) return EMPTY;
    std::sort(p_line_arr.begin(), p_line_arr.end(),
        [](const Line& a, const Line& b) { return a.compare_by_direction(b) < 0; });
    Simplex curr(p_line_arr);
    return curr.remove_redundant_lines();
}

bool Simplex::is_empty() const { return lines.empty(); }

bool Simplex::corner_is_bounded(int p_no) const
{
    if (p_no < 0 || p_no >= static_cast<int>(lines.size())) return false;
    if (lines.size() == 1) return false;
    int prev_no = (p_no == 0) ? static_cast<int>(lines.size()) - 1 : p_no - 1;
    IntVector prev_dir = lines[prev_no].direction();
    IntVector curr_dir = lines[p_no].direction();
    return prev_dir.determinant(curr_dir) > 0;
}

bool Simplex::is_bounded() const
{
    if (lines.empty()) return true;
    if (lines.size() < 3) return false;
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
        if (!corner_is_bounded(i)) return false;
    return true;
}

int Simplex::dimension() const
{
    if (lines.empty()) return -1;
    if (lines.size() > 4) return 2;
    if (lines.size() == 1) return 2; // half plane
    if (lines.size() == 2)
    {
        if (lines[0].overlaps(lines[1])) return 1;
        return 2;
    }
    if (lines.size() == 3)
    {
        if (lines[0].overlaps(lines[1]) || lines[0].overlaps(lines[2]) || lines[1].overlaps(lines[2]))
            return 1;
        auto intersection = lines[1].intersection(lines[2]);
        if (!intersection) return -1;
        const IntPoint* ip = dynamic_cast<const IntPoint*>(intersection.get());
        const FloatPoint* fp = dynamic_cast<const FloatPoint*>(intersection.get());
        Side side;
        if (ip) side = lines[0].side_of(*ip);
        else if (fp) side = lines[0].side_of(*fp);
        else return -1;
        if (side == Side::ON_THE_RIGHT) return 2;
        if (side == Side::ON_THE_LEFT) return -1;
        return 0;
    }
    // 4 edge lines
    bool collinear_0_2 = lines[0].overlaps(lines[2]);
    bool collinear_1_3 = lines[1].overlaps(lines[3]);
    if (collinear_0_2 && collinear_1_3) return 0;
    if (collinear_0_2 || collinear_1_3) return 1;
    return 2;
}

Line Simplex::border_line(int p_no) const
{
    if (lines.empty()) throw std::runtime_error("Simplex.border_line: simplex is empty");
    int no = std::clamp(p_no, 0, static_cast<int>(lines.size()) - 1);
    return lines[no];
}

Line& Simplex::mutable_border_line(int p_no)
{
    return const_cast<Line&>(lines[p_no]);
}

IntPoint Simplex::corner(int p_no) const
{
    if (lines.empty()) throw std::runtime_error("Simplex.corner: simplex is empty");
    int no = std::clamp(p_no, 0, static_cast<int>(lines.size()) - 1);
    int prev_no = (no == 0) ? static_cast<int>(lines.size()) - 1 : no - 1;
    auto result = lines[no].intersection(lines[prev_no]);
    if (!result) return IntPoint(INTEGER_MAX_VALUE, INTEGER_MAX_VALUE);
    const IntPoint* ip = dynamic_cast<const IntPoint*>(result.get());
    if (ip) return *ip;
    const FloatPoint* fp = dynamic_cast<const FloatPoint*>(result.get());
    if (fp) return fp->round();
    return IntPoint(INTEGER_MAX_VALUE, INTEGER_MAX_VALUE);
}

FloatPoint Simplex::corner_approx(int p_no) const
{
    if (lines.empty()) return FloatPoint(0, 0);
    int no = std::clamp(p_no, 0, static_cast<int>(lines.size()) - 1);
    int prev_no = (no == 0) ? static_cast<int>(lines.size()) - 1 : no - 1;
    return lines[no].intersection_approx(lines[prev_no]);
}

std::vector<FloatPoint> Simplex::corner_approx_arr() const
{
    std::vector<FloatPoint> result;
    result.reserve(lines.size());
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
        result.push_back(corner_approx(i));
    return result;
}

bool Simplex::contains(const IntPoint& p_point) const
{
    return !is_outside(p_point);
}

FloatPoint Simplex::centre_of_gravity() const
{
    FloatPoint result(0, 0);
    int corner_count = border_line_count();
    if (corner_count == 0) return result;
    for (int i = 0; i < corner_count; i++)
    {
        FloatPoint curr_point = corner_approx(i);
        result.x += curr_point.x;
        result.y += curr_point.y;
    }
    result.x /= corner_count;
    result.y /= corner_count;
    return result;
}

double Simplex::circumference() const
{
    if (!is_bounded())
    {
        return static_cast<double>(std::numeric_limits<int>::max());
    }
    int corner_count = border_line_count();
    if (corner_count <= 1) return 0;
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

std::unique_ptr<TileShape> Simplex::simplify() const
{
    if (this->is_empty())
    {
        return std::make_unique<Simplex>(Simplex::EMPTY);
    }
    if (this->is_IntBox())
    {
        return std::make_unique<IntBox>(this->bounding_box());
    }
    if (this->is_IntOctagon())
    {
        return std::make_unique<IntOctagon>(this->to_IntOctagon());
    }
    return std::make_unique<Simplex>(*this);
}

std::unique_ptr<TileShape> Simplex::clone() const
{
    return std::make_unique<Simplex>(*this);
}

bool Simplex::is_IntBox() const
{
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        if (!lines[i].is_orthogonal()) return false;
        if (!corner_is_bounded(i)) return false;
    }
    return true;
}

bool Simplex::is_IntOctagon() const
{
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        if (!lines[i].is_multiple_of_45_degree()) return false;
        if (!corner_is_bounded(i)) return false;
    }
    return true;
}

IntBox Simplex::bounding_box() const
{
    if (lines.empty()) return IntBox::EMPTY;
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        FloatPoint curr = corner_approx(i);
        min_x = std::min(min_x, curr.x);
        min_y = std::min(min_y, curr.y);
        max_x = std::max(max_x, curr.x);
        max_y = std::max(max_y, curr.y);
    }
    IntPoint lower_left(static_cast<int64_t>(std::floor(min_x)), static_cast<int64_t>(std::floor(min_y)));
    IntPoint upper_right(static_cast<int64_t>(std::ceil(max_x)), static_cast<int64_t>(std::ceil(max_y)));
    return IntBox(lower_left, upper_right);
}

IntOctagon Simplex::bounding_octagon() const
{
    if (lines.empty()) return IntOctagon::EMPTY;
    double lx = std::numeric_limits<double>::max();
    double ly = std::numeric_limits<double>::max();
    double rx = std::numeric_limits<double>::lowest();
    double uy = std::numeric_limits<double>::lowest();
    double ulx = std::numeric_limits<double>::max();
    double lrx = std::numeric_limits<double>::lowest();
    double llx = std::numeric_limits<double>::max();
    double urx = std::numeric_limits<double>::lowest();
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        FloatPoint curr = corner_approx(i);
        lx = std::min(lx, curr.x);
        ly = std::min(ly, curr.y);
        rx = std::max(rx, curr.x);
        uy = std::max(uy, curr.y);
        double tmp = curr.x - curr.y;
        ulx = std::min(ulx, tmp);
        lrx = std::max(lrx, tmp);
        tmp = curr.x + curr.y;
        llx = std::min(llx, tmp);
        urx = std::max(urx, tmp);
    }
    if (std::min(lx, ly) < -Limits::CRIT_INT || std::max(rx, uy) > Limits::CRIT_INT ||
        std::min(ulx, llx) < -Limits::CRIT_INT || std::max(lrx, urx) > Limits::CRIT_INT)
        return IntOctagon::EMPTY;
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

IntOctagon Simplex::to_IntOctagon() const
{
    if (!is_IntOctagon()) return IntOctagon::EMPTY;
    if (is_empty()) return IntOctagon::EMPTY;

    int64_t rx = Limits::CRIT_INT;
    int64_t uy = Limits::CRIT_INT;
    int64_t lrx = Limits::CRIT_INT;
    int64_t urx = Limits::CRIT_INT;
    int64_t lx = -Limits::CRIT_INT;
    int64_t ly = -Limits::CRIT_INT;
    int64_t llx = -Limits::CRIT_INT;
    int64_t ulx = -Limits::CRIT_INT;

    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        const Line& curr = lines[i];
        // Lines must be between IntPoints for is_IntOctagon
        const IntPoint& a = curr.a;
        const IntPoint& b = curr.b;
        if (a.y == b.y)
        {
            if (b.x >= a.x) ly = a.y;   // lower boundary
            if (b.x <= a.x) uy = a.y;   // upper boundary
        }
        if (a.x == b.x)
        {
            if (b.y >= a.y) rx = a.x;   // right boundary
            if (b.y <= a.y) lx = a.x;   // left boundary
        }
        if (a.y < b.y)
        {
            if (a.x < b.x)     lrx = a.x - a.y;   // lower right
            else if (a.x > b.x) urx = a.x + a.y;   // upper right
        }
        else if (a.y > b.y)
        {
            if (a.x < b.x)     llx = a.x + a.y;   // lower left
            else if (a.x > b.x) ulx = a.x - a.y;   // upper left
        }
    }
    return IntOctagon(lx, ly, rx, uy, ulx, lrx, llx, urx).normalize();
}

double Simplex::max_width() const
{
    if (!is_bounded()) return static_cast<double>(INTEGER_MAX_VALUE);
    FloatPoint gravity = corner_approx(0);
    double max_dist = std::numeric_limits<double>::lowest();
    double max_dist2 = std::numeric_limits<double>::lowest();
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        double d = std::abs(lines[i].signed_distance(gravity));
        if (d > max_dist) { max_dist2 = max_dist; max_dist = d; }
        else if (d > max_dist2) { max_dist2 = d; }
    }
    return max_dist + max_dist2;
}

double Simplex::min_width() const
{
    if (!is_bounded()) return static_cast<double>(INTEGER_MAX_VALUE);
    FloatPoint gravity = corner_approx(0);
    double min_dist = std::numeric_limits<double>::max();
    double min_dist2 = std::numeric_limits<double>::max();
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        double d = std::abs(lines[i].signed_distance(gravity));
        if (d < min_dist) { min_dist2 = min_dist; min_dist = d; }
        else if (d < min_dist2) { min_dist2 = d; }
    }
    return min_dist + min_dist2;
}

Simplex Simplex::translate_by(const IntVector& p_vector) const
{
    if (p_vector.x == 0 && p_vector.y == 0) return *this;
    std::vector<Line> new_lines;
    new_lines.reserve(lines.size());
    for (auto& l : lines)
        new_lines.push_back(l.translate_by(p_vector));
    return Simplex(new_lines);
}

Simplex Simplex::offset(double p_width) const
{
    if (p_width == 0.0) return *this;
    std::vector<Line> new_lines;
    new_lines.reserve(lines.size());
    for (auto& l : lines)
        new_lines.push_back(l.translate(-p_width));
    Simplex result(new_lines);
    if (p_width < 0.0)
        result = result.remove_redundant_lines();
    return result;
}

Simplex Simplex::enlarge(double p_offset) const
{
    if (p_offset == 0.0) return *this;
    Simplex offset_simplex = offset(p_offset);
    IntOctagon bounding_oct = bounding_octagon();
    if (bounding_oct.is_empty()) return EMPTY;
    IntOctagon offset_oct = bounding_oct.offset(p_offset);
    return offset_simplex.intersection(offset_oct.to_Simplex());
}

bool Simplex::intersects(const Simplex& p_other) const
{
    Simplex isect = intersection(p_other);
    return !isect.is_empty();
}

bool Simplex::intersects(const IntBox& p_box) const
{
    return intersects(p_box.to_Simplex());
}

bool Simplex::intersects(const IntOctagon& p_oct) const
{
    return intersects(p_oct.to_Simplex());
}

Simplex Simplex::intersection(const IntBox& p_box) const
{
    return intersection(p_box.to_Simplex());
}

Simplex Simplex::intersection(const Simplex& p_other) const
{
    if (is_empty() || p_other.is_empty()) return EMPTY;
    std::vector<Line> new_lines;
    new_lines.reserve(lines.size() + p_other.lines.size());
    for (auto& l : lines) new_lines.push_back(l);
    for (auto& l : p_other.lines) new_lines.push_back(l);
    std::sort(new_lines.begin(), new_lines.end(),
        [](const Line& a, const Line& b) { return a.compare_by_direction(b) < 0; });
    Simplex result(new_lines);
    return result.remove_redundant_lines();
}

Simplex Simplex::intersection(const IntOctagon& p_other) const
{
    return intersection(p_other.to_Simplex());
}

int Simplex::border_line_index(const Line& p_line) const
{
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
        if (p_line.fast_equals(lines[i])) return i;
    return -1;
}

Simplex Simplex::remove_border_line(int p_no) const
{
    if (p_no < 0 || p_no >= static_cast<int>(lines.size())) return *this;
    std::vector<Line> new_lines;
    new_lines.reserve(lines.size() - 1);
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
        if (i != p_no) new_lines.push_back(lines[i]);
    return Simplex(new_lines);
}

int Simplex::index_of_right_most_corner(const IntPoint& p_from_point) const
{
    int result = 0;
    IntPoint right_most = corner(0);
    for (int i = 1; i < static_cast<int>(lines.size()); i++)
    {
        IntPoint curr = corner(i);
        if (curr.side_of(p_from_point, right_most) == Side::ON_THE_RIGHT)
        {
            right_most = curr;
            result = i;
        }
    }
    return result;
}

IntOctagon Simplex::bounding_shape(const ShapeBoundingDirections& p_dirs) const
{
    return p_dirs.bounds(*this);
}

std::vector<Simplex> Simplex::cutout_from(const IntOctagon& p_oct) const
{
    return cutout_from(p_oct.to_Simplex());
}

std::vector<Simplex> Simplex::cutout_from(const IntBox& p_box) const
{
    return cutout_from(p_box.to_Simplex());
}

std::vector<Simplex> Simplex::cutout_from(const Simplex& p_outer_simplex) const
{
    if (dimension() < 2)
    {
        logger::FRLogger::instance().warn("Simplex.cutout_from only implemented for 2-dim simplex");
        return std::vector<Simplex>{};
    }
    Simplex inner_simplex = intersection(p_outer_simplex);
    if (inner_simplex.dimension() < 2)
    {
        // nothing to cut out from p_outer_simplex
        return std::vector<Simplex>{Simplex(p_outer_simplex)};
    }
    const int inner_corner_count = static_cast<int>(inner_simplex.lines.size());
    std::vector<std::vector<Line>> division_line_arr;
    division_line_arr.reserve(inner_corner_count);
    for (int inner_corner_no = 0; inner_corner_no < inner_corner_count; inner_corner_no++)
    {
        std::vector<Line> curr_lines = inner_simplex.calc_division_lines(inner_corner_no, p_outer_simplex);
        if (curr_lines.empty())
        {
            logger::FRLogger::instance().warn("Simplex.cutout_from: division line missing");
            return std::vector<Simplex>{Simplex(p_outer_simplex)};
        }
        division_line_arr.push_back(std::move(curr_lines));
    }
    bool check_cross_first_line = false;
    bool have_prev_division_line = false;
    Line prev_division_line(IntPoint(0, 0), IntPoint(0, 0));
    const Line& first_division_line = division_line_arr[0][0];
    IntVector first_dir_vec = first_division_line.direction();
    IntDirection first_direction(first_dir_vec.x, first_dir_vec.y);
    std::vector<Simplex> result_list;

    for (int inner_corner_no = 0; inner_corner_no < inner_corner_count; inner_corner_no++)
    {
        Line next_division_line = (inner_corner_no == inner_corner_count - 1)
            ? division_line_arr[0][0]
            : division_line_arr[inner_corner_no + 1][0];
        const std::vector<Line>& curr_division_lines = division_line_arr[inner_corner_no];
        if (curr_division_lines.size() == 2)
        {
            // 2 division lines are necessary (sharp corner).
            // Construct an unbounded simplex from
            // curr_division_lines[1] and curr_division_lines[0]
            // and intersect it with the outer simplex
            IntVector curr_vec = curr_division_lines[0].direction();
            IntDirection curr_dir(curr_vec.x, curr_vec.y);
            bool merge_prev_division_line = false;
            bool merge_first_division_line = false;
            if (have_prev_division_line)
            {
                IntVector prev_vec = prev_division_line.direction();
                IntDirection prev_dir(prev_vec.x, prev_vec.y);
                if (curr_dir.determinant(prev_dir) > 0)
                {
                    // the previous division line may intersect
                    // curr_division_lines[0] inside p_divide_simplex
                    merge_prev_division_line = true;
                }
            }
            if (!check_cross_first_line)
            {
                check_cross_first_line = inner_corner_no > 0 && curr_dir.determinant(first_direction) > 0;
            }
            if (check_cross_first_line)
            {
                IntVector curr_vec2 = curr_division_lines[1].direction();
                IntDirection curr_dir2(curr_vec2.x, curr_vec2.y);
                if (curr_dir2.determinant(first_direction) < 0)
                {
                    // The current piece has an intersection area with the first
                    // piece. Add a line to tmp_polyline to prevent this
                    merge_first_division_line = true;
                }
            }
            int piece_line_count = 2;
            if (merge_prev_division_line)
            {
                ++piece_line_count;
            }
            if (merge_first_division_line)
            {
                ++piece_line_count;
            }
            std::vector<Line> piece_lines;
            piece_lines.reserve(piece_line_count);
            piece_lines.emplace_back(curr_division_lines[1].b, curr_division_lines[1].a);
            piece_lines.push_back(curr_division_lines[0]);
            if (merge_prev_division_line)
            {
                piece_lines.push_back(prev_division_line);
            }
            if (merge_first_division_line)
            {
                piece_lines.emplace_back(first_division_line.b, first_division_line.a);
            }
            Simplex curr_piece(piece_lines);
            result_list.push_back(curr_piece.intersection(p_outer_simplex));
        }
        // construct an unbounded simplex from next_division_line,
        // inner_simplex.line[inner_corner_no] and the last current division line
        // and intersect it with the outer simplex
        bool merge_next_division_line = !(next_division_line.b == next_division_line.a);
        const Line& last_curr_division_line = curr_division_lines[curr_division_lines.size() - 1];
        IntVector last_curr_vec = last_curr_division_line.direction();
        IntDirection last_curr_dir(last_curr_vec.x, last_curr_vec.y);
        bool merge_last_curr_division_line = !(last_curr_division_line.b == last_curr_division_line.a);
        bool merge_prev_division_line = false;
        bool merge_first_division_line = false;
        if (have_prev_division_line)
        {
            IntVector prev_vec = prev_division_line.direction();
            IntDirection prev_dir(prev_vec.x, prev_vec.y);
            if (last_curr_dir.determinant(prev_dir) > 0)
            {
                // the previous division line may intersect
                // the last current division line inside p_divide_simplex
                merge_prev_division_line = true;
            }
        }
        if (!check_cross_first_line)
        {
            check_cross_first_line = inner_corner_no > 0 && last_curr_dir.determinant(first_direction) > 0 &&
                                     last_curr_dir.x * first_direction.x + last_curr_dir.y * first_direction.y < 0;
            // scalar_product checked to ignore backcrossing at
            // small inner_corner_no
        }
        if (check_cross_first_line)
        {
            IntVector next_vec = next_division_line.direction();
            IntDirection next_dir(next_vec.x, next_vec.y);
            if (next_dir.determinant(first_direction) < 0)
            {
                // The current piece has an intersection area with the first piece.
                // Add a line to tmp_polyline to prevent this
                merge_first_division_line = true;
            }
        }
        int piece_line_count = 1;
        if (merge_next_division_line)
        {
            ++piece_line_count;
        }
        if (merge_last_curr_division_line)
        {
            ++piece_line_count;
        }
        if (merge_prev_division_line)
        {
            ++piece_line_count;
        }
        if (merge_first_division_line)
        {
            ++piece_line_count;
        }
        std::vector<Line> piece_lines;
        piece_lines.reserve(piece_line_count);
        const Line& curr_line = inner_simplex.lines[inner_corner_no];
        piece_lines.emplace_back(curr_line.b, curr_line.a);
        if (merge_next_division_line)
        {
            piece_lines.emplace_back(next_division_line.b, next_division_line.a);
        }
        if (merge_last_curr_division_line)
        {
            piece_lines.push_back(last_curr_division_line);
        }
        if (merge_prev_division_line)
        {
            piece_lines.push_back(prev_division_line);
        }
        if (merge_first_division_line)
        {
            piece_lines.emplace_back(first_division_line.b, first_division_line.a);
        }
        Simplex curr_piece(piece_lines);
        result_list.push_back(curr_piece.intersection(p_outer_simplex));
        // Java parity: Simplex.java never assigns prev_division_line (it stays
        // null), so the merge_prev_division_line branches above are inert.
    }
    return result_list;
}

std::vector<Line> Simplex::calc_division_lines(int p_inner_corner_no, const Simplex& p_outer_simplex) const
{
    const Line& curr_inner_line = lines[p_inner_corner_no];
    const Line& prev_inner_line = (p_inner_corner_no != 0) ? lines[p_inner_corner_no - 1] : lines[lines.size() - 1];
    FloatPoint intersection = curr_inner_line.intersection_approx(prev_inner_line);
    if (intersection.x >= INTEGER_MAX_VALUE)
    {
        logger::FRLogger::instance().warn("Simplex.calc_division_lines: intersection expected");
        return std::vector<Line>{};
    }
    IntPoint inner_corner = intersection.round();
    const double c_tolerance = 0.0001;
    bool is_exact = std::abs(static_cast<double>(inner_corner.x) - intersection.x) < c_tolerance &&
                    std::abs(static_cast<double>(inner_corner.y) - intersection.y) < c_tolerance;

    if (!is_exact)
    {
        // it is assumed that the corners of the original inner simplex are
        // exact and the not exact corners come from the intersection of
        // the inner simplex with the outer simplex. Because these corners
        // lie on the border of the outer simplex, no division is necessary
        return std::vector<Line>{prev_inner_line};
    }
    std::optional<IntDirection> first_projection_dir;
    std::optional<IntDirection> second_projection_dir;
    IntVector prev_vec = prev_inner_line.direction();
    IntDirection prev_inner_dir(-prev_vec.x, -prev_vec.y);   // direction().opposite()
    IntVector next_vec = curr_inner_line.direction();
    IntDirection next_inner_dir(next_vec.x, next_vec.y);
    int outer_line_no = 0;

    // search the first outer line, so that the perpendicular projection of the
    // inner corner onto this line is visible from inner_corner to the left of
    // prev_inner_line.

    double min_distance = INTEGER_MAX_VALUE;

    for (int ind = 0; ind < static_cast<int>(p_outer_simplex.lines.size()); ind++)
    {
        const Line& outer_line = p_outer_simplex.lines[outer_line_no];
        IntDirection curr_projection_dir = inner_corner.perpendicular_direction(outer_line);
        if (curr_projection_dir.equals(IntDirection::NULL_DIR))
        {
            return std::vector<Line>{Line(inner_corner, inner_corner)};
        }
        bool projection_visible = prev_inner_dir.determinant(curr_projection_dir) >= 0;
        if (projection_visible)
        {
            double curr_distance = std::abs(outer_line.signed_distance(inner_corner.to_float()));
            bool second_division_necessary = curr_projection_dir.determinant(next_inner_dir) < 0;
            // may occur at a sharp angle
            std::optional<IntDirection> curr_second_projection_dir = curr_projection_dir;

            if (second_division_necessary)
            {
                // search the first projection_dir between curr_projection_dir
                // and next_inner_dir, that is visible from next_inner_line
                bool second_projection_visible = false;
                int tmp_outer_line_no = outer_line_no;
                while (!second_projection_visible)
                {
                    if (tmp_outer_line_no == static_cast<int>(p_outer_simplex.lines.size()) - 1)
                    {
                        tmp_outer_line_no = 0;
                    }
                    else
                    {
                        ++tmp_outer_line_no;
                    }
                    curr_second_projection_dir.emplace(inner_corner.perpendicular_direction(p_outer_simplex.lines[tmp_outer_line_no]));

                    if (curr_second_projection_dir->equals(IntDirection::NULL_DIR))
                    // inner corner is on outer_line
                    {
                        return std::vector<Line>{Line(inner_corner, inner_corner)};
                    }
                    if (curr_projection_dir.determinant(*curr_second_projection_dir) < 0)
                    {
                        // curr_second_projection_dir not found;
                        // the angle between curr_projection_dir and
                        // curr_second_projection_dir would be already bigger
                        // than 180 degree
                        curr_distance = INTEGER_MAX_VALUE;
                        break;
                    }

                    second_projection_visible = curr_second_projection_dir->determinant(next_inner_dir) >= 0;
                }
                curr_distance += std::abs(p_outer_simplex.lines[tmp_outer_line_no].signed_distance(inner_corner.to_float()));
            }
            if (curr_distance < min_distance)
            {
                min_distance = curr_distance;
                first_projection_dir.emplace(curr_projection_dir);
                second_projection_dir.emplace(*curr_second_projection_dir);
            }
        }
        if (outer_line_no == static_cast<int>(p_outer_simplex.lines.size()) - 1)
        {
            outer_line_no = 0;
        }
        else
        {
            ++outer_line_no;
        }
    }
    if (min_distance == static_cast<double>(INTEGER_MAX_VALUE))
    {
        logger::FRLogger::instance().warn("Simplex.calc_division_lines: division not found");
        return std::vector<Line>{};
    }
    std::vector<Line> result;
    if (first_projection_dir->equals(*second_projection_dir))
    {
        result.emplace_back(Line::get_instance(inner_corner, *first_projection_dir));
    }
    else
    {
        result.emplace_back(Line::get_instance(inner_corner, *first_projection_dir));
        result.emplace_back(Line::get_instance(inner_corner, *second_projection_dir));
    }
    return result;
}

Simplex Simplex::remove_redundant_lines() const
{
    const int orig_len = static_cast<int>(lines.size());
    if (orig_len == 0) return *this;

    std::vector<Line> line_arr;
    line_arr.reserve(orig_len);
    line_arr.push_back(lines[0]);
    for (int i = 1; i < orig_len; i++)
    {
        if (!lines[i].fast_equals(line_arr.back()))
            line_arr.push_back(lines[i]);
    }
    int new_length = static_cast<int>(line_arr.size());

    std::vector<Side> intersection_sides(new_length, Side::NO_SIDE);

    bool try_again = new_length > 2;
    int index_of_last_removed = new_length;

    while (try_again)
    {
        try_again = false;
        int prev_ind = new_length - 1;
        Line prev_line = line_arr[prev_ind];
        Line curr_line = line_arr[0];

        for (int ind = 0; ind < new_length; ind++)
        {
            int next_ind = (ind == new_length - 1) ? 0 : ind + 1;
            Line next_line = line_arr[next_ind];

            bool remove_line = false;
            IntVector prev_dir = prev_line.direction();
            IntVector next_dir = next_line.direction();
            double det = prev_dir.determinant(next_dir);

            if (det != 0.0)
            {
                if (intersection_sides[ind] == Side::NO_SIDE)
                {
                    intersection_sides[ind] = curr_line.side_of_intersection(prev_line, next_line);
                }
                if (det > 0)
                {
                    remove_line = intersection_sides[ind] != Side::ON_THE_LEFT;
                }
                else
                {
                    if (intersection_sides[ind] == Side::ON_THE_LEFT)
                    {
                        IntVector curr_dir = curr_line.direction();
                        if (prev_dir.determinant(curr_dir) > 0)
                        {
                            new_length = 0;
                            try_again = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (prev_line.side_of(next_line.a) == Side::ON_THE_LEFT)
                {
                    new_length = 0;
                    try_again = false;
                    break;
                }
            }

            if (remove_line)
            {
                try_again = true;
                --new_length;
                for (int i = ind; i < new_length; i++)
                {
                    line_arr[i] = line_arr[i + 1];
                    intersection_sides[i] = intersection_sides[i + 1];
                }
                if (new_length < 3) { try_again = false; break; }
                prev_ind = (ind == 0) ? new_length - 1 : ind - 1;
                intersection_sides[prev_ind] = Side::NO_SIDE;
                int ni = (ind >= new_length) ? 0 : ind;
                intersection_sides[ni] = Side::NO_SIDE;
                --ind;
                index_of_last_removed = ind;
            }
            else
            {
                prev_line = curr_line;
                prev_ind = ind;
            }
            curr_line = next_line;

            if (!try_again && ind >= index_of_last_removed) break;
        }
    }

    if (new_length == 2)
    {
        if (line_arr[0].is_parallel(line_arr[1]))
        {
            IntVector d0 = line_arr[0].direction();
            IntVector d1 = line_arr[1].direction();
            if (d0.x == d1.x && d0.y == d1.y)
            {
                if (line_arr[1].side_of(line_arr[0].a) == Side::ON_THE_LEFT)
                    line_arr[0] = line_arr[1];
                --new_length;
            }
            else
            {
                if (line_arr[1].side_of(line_arr[0].a) == Side::ON_THE_LEFT)
                    new_length = 0;
            }
        }
    }

    if (new_length == orig_len) return *this;
    if (new_length == 0) return EMPTY;
    std::vector<Line> result(line_arr.begin(), line_arr.begin() + new_length);
    return Simplex(result);
}

std::string Simplex::to_string() const
{
    std::ostringstream ss;
    ss << "Simplex[" << lines.size() << " lines]";
    return ss.str();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
