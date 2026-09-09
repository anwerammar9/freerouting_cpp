#include "board/CalcShapeAndFromSide.h"

#include <optional>
#include <utility>

#include "board/CalcFromSide.h"
#include "board/PolylineTrace.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::board
{

namespace
{

/**
 * Returns the number of the border line of p_simplex equal or opposite to
 * p_line, or -1 if no such border line exists. Mirrors Java Line.equals which
 * is direction insensitive after normalization.
 */
int border_line_index_eq_or_opposite(const geometry::planar::Simplex& p_simplex,
                                     const geometry::planar::Line& p_line)
{
    const int border_count = p_simplex.border_line_count();
    for (int i = 0; i < border_count; ++i)
    {
        if (p_simplex.border_line(i).is_equal_or_opposite(p_line))
        {
            return i;
        }
    }
    return -1;
}

std::optional<geometry::planar::Line> calc_cutline_at_end(int p_index,
                                                          const PolylineTrace& p_trace)
{
    const geometry::planar::Polyline& trace_lines = p_trace.polyline();
    const ShapeSearchTree* search_tree =
        p_trace.get_board()->get_search_tree_manager()->get_default_tree();
    const int line_count = static_cast<int>(trace_lines.arr.size());
    if (p_index == line_count - 3
        || trace_lines.corner_approx(line_count - 2)
                   .distance(trace_lines.corner_approx(p_index + 1))
               < p_trace.get_compensated_half_width(*search_tree))
    {
        const geometry::planar::Line& curr_line = trace_lines.arr[line_count - 1];
        geometry::planar::FloatPoint is = trace_lines.corner_approx(line_count - 3);
        if (curr_line.side_of(is) == geometry::planar::Side::ON_THE_LEFT)
        {
            return curr_line.opposite();
        }
        return curr_line;
    }
    return std::nullopt;
}

std::optional<geometry::planar::Line> calc_cutline_at_start(int p_index,
                                                            const PolylineTrace& p_trace)
{
    const geometry::planar::Polyline& trace_lines = p_trace.polyline();
    const ShapeSearchTree* search_tree =
        p_trace.get_board()->get_search_tree_manager()->get_default_tree();
    if (p_index == 0
        || trace_lines.corner_approx(0).distance(trace_lines.corner_approx(p_index))
               < p_trace.get_compensated_half_width(*search_tree))
    {
        const geometry::planar::Line& curr_line = trace_lines.arr[0];
        geometry::planar::FloatPoint is = trace_lines.corner_approx(1);
        if (curr_line.side_of(is) == geometry::planar::Side::ON_THE_LEFT)
        {
            return curr_line.opposite();
        }
        return curr_line;
    }
    return std::nullopt;
}

} // namespace

CalcShapeAndFromSide::CalcShapeAndFromSide(const PolylineTrace& p_trace, int p_index,
                                           bool p_orthogonal, bool p_in_shove_check)
{
    const ShapeSearchTree* search_tree =
        p_trace.get_board()->get_search_tree_manager()->get_default_tree();
    const geometry::planar::TileShape* shape_orig = p_trace.get_tree_shape(*search_tree, p_index);
    if (shape_orig == nullptr)
    {
        shape_ = std::make_shared<geometry::planar::IntBox>();
        from_side_ = nullptr;
        return;
    }

    std::shared_ptr<const geometry::planar::TileShape> curr_shape;
    std::unique_ptr<CalcFromSide> curr_from_side;
    bool cut_off_at_start = false;
    bool cut_off_at_end = false;

    if (p_orthogonal)
    {
        curr_shape = std::make_shared<geometry::planar::IntBox>(shape_orig->bounding_box());
    }
    else
    {
        // prevent dog ears at the start and the end of the substitute trace
        geometry::planar::Simplex curr_simplex = shape_orig->to_Simplex();
        std::optional<geometry::planar::Line> end_cutline = calc_cutline_at_end(p_index, p_trace);
        if (end_cutline.has_value())
        {
            geometry::planar::Simplex cut_plane =
                geometry::planar::Simplex::get_instance({*end_cutline});
            geometry::planar::Simplex tmp_shape = curr_simplex.intersection(cut_plane);
            if (!tmp_shape.is_empty())
            {
                curr_simplex = tmp_shape;
                cut_off_at_end = true;
            }
        }
        std::optional<geometry::planar::Line> start_cutline =
            calc_cutline_at_start(p_index, p_trace);
        if (start_cutline.has_value())
        {
            geometry::planar::Simplex cut_plane =
                geometry::planar::Simplex::get_instance({*start_cutline});
            geometry::planar::Simplex tmp_shape = curr_simplex.intersection(cut_plane);
            if (!tmp_shape.is_empty())
            {
                curr_simplex = tmp_shape;
                cut_off_at_start = true;
            }
        }

        int from_side_no = -1;
        const geometry::planar::Line* curr_cut_line = nullptr;
        if (cut_off_at_start)
        {
            curr_cut_line = &(*start_cutline);
            from_side_no = border_line_index_eq_or_opposite(curr_simplex, *curr_cut_line);
        }
        if (from_side_no < 0 && cut_off_at_end)
        {
            curr_cut_line = &(*end_cutline);
            from_side_no = border_line_index_eq_or_opposite(curr_simplex, *curr_cut_line);
        }
        if (from_side_no >= 0)
        {
            geometry::planar::FloatPoint border_intersection =
                curr_cut_line->intersection_approx(curr_simplex.border_line(from_side_no));
            curr_from_side = std::make_unique<CalcFromSide>(from_side_no, border_intersection);
        }
        curr_shape = std::make_shared<geometry::planar::Simplex>(curr_simplex);
    }

    if (curr_from_side == nullptr && !p_in_shove_check)
    {
        // In p_in_shove_check, using this calculation may produce an undesired
        // stack_level > 1 in ShapeTraceEntries.
        curr_from_side = std::make_unique<CalcFromSide>(p_trace.polyline(), p_index, *curr_shape);
    }

    shape_ = std::move(curr_shape);
    from_side_ = std::move(curr_from_side);
}

} // namespace freerouting::board