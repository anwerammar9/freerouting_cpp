#include "board/ForcedViaAlgo.h"

#include <cmath>
#include <memory>

#include "board/FixedState.h"
#include "board/ForcedPadAlgo.h"
#include "board/Padstack.h"
#include "board/RoutingBoard.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Shape.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Vector.h"
#include "logger/FRLogger.h"
#include "rules/BoardRules.h"
#include "rules/ViaInfo.h"

namespace freerouting::board {

namespace {

bool is_90_degree_mode(RoutingBoard* p_board)
{
    return p_board->get_rules().get_trace_angle_restriction() == rules::AngleRestriction::RIGHT;
}

/**
 * Returns the bounding tile of p_shape (box for 90-degree mode, octagon
 * otherwise), moved by p_translate when that is not null. Mirrors the Java
 * curr_pad_shape.translate_by(...) + bounding_box/bounding_octagon sequence.
 *
 * The Java Shape interface has translate_by on every shape, but the C++ Shape
 * class only declares it on Area, so a per-shape translation would be silently
 * skipped for every other shape type. Translating a shape moves its bounding
 * tile by the same amount, so the tile is built first and moved afterwards,
 * which is equivalent for every convex shape. The hole clearance shape is
 * already built around the via location, so it is passed a null translation.
 */
std::unique_ptr<geometry::planar::TileShape> tile_shape_of(
    const geometry::planar::Shape* p_shape,
    const geometry::planar::IntVector* p_translate,
    bool p_is_90_degree)
{
    const bool translate = p_translate != nullptr;
    const geometry::planar::IntVector offset =
        translate ? *p_translate : geometry::planar::IntVector::ZERO;

    const auto* circle = dynamic_cast<const geometry::planar::Circle*>(p_shape);
    const auto* area = dynamic_cast<const geometry::planar::Area*>(p_shape);
    const geometry::planar::IntBox box = circle != nullptr
        ? circle->bounding_box()
        : (area != nullptr ? area->bounding_box() : p_shape->bounding_box());
    if (p_is_90_degree)
    {
        return std::make_unique<geometry::planar::IntBox>(
            translate ? box.translate_by(offset) : box);
    }
    const geometry::planar::IntOctagon octagon = circle != nullptr
        ? circle->bounding_octagon()
        : (area != nullptr ? area->bounding_octagon() : box.to_IntOctagon());
    return std::make_unique<geometry::planar::IntOctagon>(
        translate ? octagon.translate_by(offset) : octagon);
}

} // namespace

ForcedPadAlgo::CheckDrillResult ForcedViaAlgo::check_layer(double p_via_radius,
                                                          int p_cl_class,
                                                          bool p_attach_smd_allowed,
                                                          const geometry::planar::TileShape& p_room_shape,
                                                          const geometry::planar::Point& p_location,
                                                          int p_layer,
                                                          const std::vector<int>& p_net_no_arr,
                                                          int p_max_recursion_depth,
                                                          int p_max_via_recursion_depth,
                                                          RoutingBoard* p_board,
                                                          int p_trace_half_width,
                                                          int p_trace_clearance_class)
{
    if (p_via_radius <= 0.0)
    {
        return ForcedPadAlgo::CheckDrillResult::DRILLABLE;
    }
    ForcedPadAlgo forced_pad_algo(p_board);

    const auto* location_int = dynamic_cast<const geometry::planar::IntPoint*>(&p_location);
    if (location_int == nullptr)
    {
        return ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE;
    }
    geometry::planar::Circle via_shape(*location_int, static_cast<int64_t>(std::ceil(p_via_radius)));

    double check_radius = p_via_radius
        + 0.5 * p_board->clearance_value(p_cl_class, p_cl_class, p_layer)
        + p_board->get_rules().get_min_trace_half_width();

    const bool is_90_degree = is_90_degree_mode(p_board);
    std::unique_ptr<geometry::planar::TileShape> tile_shape;
    if (is_90_degree)
    {
        tile_shape = std::make_unique<geometry::planar::IntBox>(via_shape.bounding_box());
    }
    else
    {
        tile_shape = std::make_unique<geometry::planar::IntOctagon>(via_shape.bounding_octagon());
    }

    geometry::planar::Simplex room_simplex = p_room_shape.to_Simplex();
    CalcFromSide from_side =
        calculate_from_side(p_location.to_float(), *tile_shape, room_simplex, check_radius, is_90_degree);
    if (from_side.no < 0)
    {
        return ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE;
    }

    const std::vector<Item*> empty_ignore;
    ForcedPadAlgo::CheckDrillResult via_result = forced_pad_algo.check_forced_pad(*tile_shape, &from_side, p_layer,
        p_net_no_arr, p_cl_class, p_attach_smd_allowed, empty_ignore, p_max_recursion_depth, p_max_via_recursion_depth,
        false, nullptr);
    if (via_result == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
    {
        return via_result;
    }

    if (p_trace_half_width <= 0)
    {
        return via_result;
    }

    // Check that a trace of the trace width can start at the via location.
    geometry::planar::Circle start_trace_circle(*location_int, static_cast<int64_t>(p_trace_half_width));
    std::unique_ptr<geometry::planar::TileShape> start_trace_shape;
    if (is_90_degree)
    {
        start_trace_shape = std::make_unique<geometry::planar::IntBox>(start_trace_circle.bounding_box());
    }
    else
    {
        start_trace_shape = std::make_unique<geometry::planar::IntOctagon>(start_trace_circle.bounding_octagon());
    }

    ForcedPadAlgo::CheckDrillResult trace_result = forced_pad_algo.check_forced_pad(*start_trace_shape, &from_side,
        p_layer, p_net_no_arr, p_trace_clearance_class, true, empty_ignore, p_max_recursion_depth,
        p_max_via_recursion_depth, false, nullptr);
    if (trace_result == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
    {
        return trace_result;
    }
    if (via_result == ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD
        || trace_result == ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD)
    {
        return ForcedPadAlgo::CheckDrillResult::DRILLABLE_WITH_ATTACH_SMD;
    }
    return ForcedPadAlgo::CheckDrillResult::DRILLABLE;
}

bool ForcedViaAlgo::check(const std::shared_ptr<rules::ViaInfo>& p_via_info,
                          const geometry::planar::Point& p_location,
                          const std::vector<int>& p_net_no_arr,
                          int p_max_recursion_depth,
                          int p_max_via_recursion_depth,
                          RoutingBoard* p_board,
                          const std::vector<int>* p_trace_pen_halfwidth_arr,
                          int p_trace_clearance_class_index)
{
    if (p_via_info == nullptr || p_board == nullptr)
    {
        return false;
    }
    geometry::planar::IntPoint origin(0, 0);
    const std::unique_ptr<geometry::planar::Vector> translate_vector =
        p_location.difference_by(origin);
    const auto* translate_offset =
        dynamic_cast<const geometry::planar::IntVector*>(translate_vector.get());
    const int calc_from_side_offset = p_board->get_rules().get_min_trace_half_width();
    ForcedPadAlgo forced_pad_algo(p_board);

    auto via_padstack = p_via_info->get_padstack();
    if (via_padstack == nullptr)
    {
        logger::FRLogger::instance().warn("ForcedViaAlgo.check: via padstack is null");
        return false;
    }
    const bool is_90_degree = is_90_degree_mode(p_board);
    const std::vector<Item*> empty_ignore;
    const std::unique_ptr<geometry::planar::Circle> hole_shape =
        hole_check_shape(via_padstack.get(), p_location, p_board);
    for (int i = via_padstack->get_from_layer(); i <= via_padstack->get_to_layer(); ++i)
    {
        const geometry::planar::Shape* curr_pad_shape = via_padstack->get_shape(i);
        int curr_cl_class = p_via_info->get_clearance_class();
        bool is_hole_shape = false;
        if (curr_pad_shape == nullptr)
        {
            if (hole_shape == nullptr)
            {
                continue;
            }
            // The drill hole itself must keep hole clearance from copper on
            // this layer, so the hole shape is checked with the null clearance
            // class. It is already positioned at the via location.
            curr_pad_shape = hole_shape.get();
            curr_cl_class = 0;
            is_hole_shape = true;
        }
        std::unique_ptr<geometry::planar::TileShape> tile_shape = tile_shape_of(
            curr_pad_shape, is_hole_shape ? nullptr : translate_offset, is_90_degree);
        CalcFromSide from_side = forced_pad_algo.calc_from_side(
            *tile_shape, p_location, i, calc_from_side_offset, curr_cl_class);
        if (forced_pad_algo.check_forced_pad(*tile_shape, &from_side, i, p_net_no_arr,
                                             curr_cl_class,
                                             p_via_info->attach_smd_allowed(), empty_ignore,
                                             p_max_recursion_depth, p_max_via_recursion_depth, false,
                                             nullptr)
            == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
        {
            p_board->set_shove_failing_layer(i);
            return false;
        }

        // The drill hole must ALSO keep hole clearance from other-net copper on
        // layers where the pad exists: the pad check above only enforces the
        // (smaller) copper clearance.
        if (curr_cl_class != 0 && hole_shape != nullptr)
        {
            std::unique_ptr<geometry::planar::TileShape> hole_tile =
                tile_shape_of(hole_shape.get(), nullptr, is_90_degree);
            if (forced_pad_algo.check_forced_pad(
                    *hole_tile, &from_side, i, p_net_no_arr,
                    rules::BoardRules::clearance_class_none(),
                    p_via_info->attach_smd_allowed(), empty_ignore,
                    p_max_recursion_depth, p_max_via_recursion_depth, false, nullptr)
                == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
            {
                p_board->set_shove_failing_layer(i);
                return false;
            }
        }

        // Check that a trace of the routed trace width can start at the via
        // location. Port of Java ForcedViaInserter.check() lines 209-235.
        if (p_trace_pen_halfwidth_arr != nullptr
            && i < static_cast<int>(p_trace_pen_halfwidth_arr->size())
            && (*p_trace_pen_halfwidth_arr)[i] > 0)
        {
            const auto* trace_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_location);
            if (trace_point != nullptr)
            {
                geometry::planar::Circle start_trace_circle(*trace_point, (*p_trace_pen_halfwidth_arr)[i]);
                std::unique_ptr<geometry::planar::TileShape> start_trace_shape;
                if (is_90_degree)
                {
                    start_trace_shape =
                        std::make_unique<geometry::planar::IntBox>(start_trace_circle.bounding_box());
                }
                else
                {
                    start_trace_shape = std::make_unique<geometry::planar::IntOctagon>(
                        start_trace_circle.bounding_octagon());
                }
                if (forced_pad_algo.check_forced_pad(
                        *start_trace_shape, &from_side, i, p_net_no_arr, p_trace_clearance_class_index,
                        true, empty_ignore, p_max_recursion_depth, p_max_via_recursion_depth, false,
                        nullptr)
                    == ForcedPadAlgo::CheckDrillResult::NOT_DRILLABLE)
                {
                    p_board->set_shove_failing_layer(i);
                    return false;
                }
            }
        }
    }
    return true;
}

bool ForcedViaAlgo::insert(const std::shared_ptr<rules::ViaInfo>& p_via_info,
                           const geometry::planar::Point& p_location,
                           const std::vector<int>& p_net_no_arr,
                           int p_trace_clearance_class_no,
                           const std::vector<int>& p_trace_pen_halfwidth_arr,
                           int p_max_recursion_depth,
                           int p_max_via_recursion_depth,
                           RoutingBoard* p_board)
{
    if (p_via_info == nullptr || p_board == nullptr)
    {
        return false;
    }
    geometry::planar::IntPoint origin(0, 0);
    const std::unique_ptr<geometry::planar::Vector> translate_vector =
        p_location.difference_by(origin);
    const auto* translate_offset =
        dynamic_cast<const geometry::planar::IntVector*>(translate_vector.get());
    const int calc_from_side_offset = p_board->get_rules().get_min_trace_half_width();
    ForcedPadAlgo forced_pad_algo(p_board);

    auto via_padstack = p_via_info->get_padstack();
    if (via_padstack == nullptr)
    {
        logger::FRLogger::instance().warn("ForcedViaAlgo.insert: via padstack is null");
        return false;
    }
    const bool is_90_degree = is_90_degree_mode(p_board);
    const std::vector<Item*> empty_ignore;
    const auto* location_int = dynamic_cast<const geometry::planar::IntPoint*>(&p_location);
    const std::unique_ptr<geometry::planar::Circle> hole_shape =
        hole_check_shape(via_padstack.get(), p_location, p_board);
    for (int i = via_padstack->get_from_layer(); i <= via_padstack->get_to_layer(); ++i)
    {
        const geometry::planar::Shape* curr_pad_shape = via_padstack->get_shape(i);
        int curr_cl_class = p_via_info->get_clearance_class();
        bool is_hole_shape = false;
        if (curr_pad_shape == nullptr)
        {
            if (hole_shape == nullptr)
            {
                continue;
            }
            curr_pad_shape = hole_shape.get();
            curr_cl_class = rules::BoardRules::clearance_class_none();
            is_hole_shape = true;
        }
        std::unique_ptr<geometry::planar::TileShape> tile_shape = tile_shape_of(
            curr_pad_shape, is_hole_shape ? nullptr : translate_offset, is_90_degree);

        int trace_halfwidth = 0;
        if (i >= 0 && i < static_cast<int>(p_trace_pen_halfwidth_arr.size()))
        {
            trace_halfwidth = p_trace_pen_halfwidth_arr[i];
        }
        std::unique_ptr<geometry::planar::TileShape> start_trace_shape;
        if (trace_halfwidth > 0 && location_int != nullptr)
        {
            geometry::planar::Circle start_trace_circle(*location_int, static_cast<int64_t>(trace_halfwidth));
            if (is_90_degree)
            {
                start_trace_shape = std::make_unique<geometry::planar::IntBox>(start_trace_circle.bounding_box());
            }
            else
            {
                start_trace_shape = std::make_unique<geometry::planar::IntOctagon>(start_trace_circle.bounding_octagon());
            }
        }

        CalcFromSide from_side = forced_pad_algo.calc_from_side(
            *tile_shape, p_location, i, calc_from_side_offset, curr_cl_class);
        if (!forced_pad_algo.forced_pad(*tile_shape, &from_side, i, p_net_no_arr,
                                        curr_cl_class,
                                        p_via_info->attach_smd_allowed(), empty_ignore,
                                        p_max_recursion_depth, p_max_via_recursion_depth))
        {
            p_board->set_shove_failing_layer(i);
            return false;
        }
        if (curr_cl_class != rules::BoardRules::clearance_class_none() && hole_shape != nullptr)
        {
            std::unique_ptr<geometry::planar::TileShape> hole_tile =
                tile_shape_of(hole_shape.get(), nullptr, is_90_degree);
            if (!forced_pad_algo.forced_pad(*hole_tile, &from_side, i, p_net_no_arr,
                                            rules::BoardRules::clearance_class_none(),
                                            p_via_info->attach_smd_allowed(), empty_ignore,
                                            p_max_recursion_depth, p_max_via_recursion_depth))
            {
                p_board->set_shove_failing_layer(i);
                return false;
            }
        }
        if (start_trace_shape != nullptr)
        {
            // necessary in case start_trace_shape is bigger than tile_shape
            if (!forced_pad_algo.forced_pad(*start_trace_shape, &from_side, i, p_net_no_arr,
                                            p_trace_clearance_class_no, true, empty_ignore,
                                            p_max_recursion_depth, p_max_via_recursion_depth))
            {
                p_board->set_shove_failing_layer(i);
                return false;
            }
        }
    }
    p_board->insert_via(via_padstack.get(), p_location, p_net_no_arr, p_via_info->get_clearance_class(),
                        FixedState::NOT_FIXED, p_via_info->attach_smd_allowed());
    return true;
}

std::unique_ptr<geometry::planar::Circle> ForcedViaAlgo::hole_check_shape(
    const Padstack* p_padstack, const geometry::planar::Point& p_location, RoutingBoard* p_board)
{
    if (p_padstack == nullptr || p_board == nullptr)
    {
        return nullptr;
    }
    const int hole_clearance = p_board->mutable_rules().get_hole_clearance();
    const auto* center = dynamic_cast<const geometry::planar::IntPoint*>(&p_location);
    if (hole_clearance <= 0 || center == nullptr)
    {
        return nullptr;
    }
    const double drill_radius = p_padstack->get_drill_radius();
    if (drill_radius <= 0.0)
    {
        return nullptr;
    }
    // Inflate by the hole clearance itself and check with the null clearance
    // class (0), so the requirement is exact hole-to-copper spacing regardless
    // of the neighbor's class.
    return std::make_unique<geometry::planar::Circle>(
        *center, static_cast<int64_t>(std::ceil(drill_radius + hole_clearance + 10)));
}

CalcFromSide ForcedViaAlgo::calculate_from_side(const geometry::planar::FloatPoint& p_via_location,
                                                const geometry::planar::TileShape& p_via_shape,
                                                const geometry::planar::Simplex& p_room_shape,
                                                double p_dist,
                                                bool p_is_90_degree)
{
    geometry::planar::IntBox via_box = p_via_shape.bounding_box();
    for (int i = 0; i < 4; ++i)
    {
        geometry::planar::FloatPoint check_point;
        double border_x = 0.0;
        double border_y = 0.0;
        switch (i)
        {
            case 0:
                check_point = geometry::planar::FloatPoint(p_via_location.x, p_via_location.y - p_dist);
                border_x = p_via_location.x;
                border_y = static_cast<double>(via_box.ll.y);
                break;
            case 1:
                check_point = geometry::planar::FloatPoint(p_via_location.x + p_dist, p_via_location.y);
                border_x = static_cast<double>(via_box.ur.x);
                border_y = p_via_location.y;
                break;
            case 2:
                check_point = geometry::planar::FloatPoint(p_via_location.x, p_via_location.y + p_dist);
                border_x = p_via_location.x;
                border_y = static_cast<double>(via_box.ur.y);
                break;
            default: // i == 3
                check_point = geometry::planar::FloatPoint(p_via_location.x - p_dist, p_via_location.y);
                border_x = static_cast<double>(via_box.ll.x);
                border_y = p_via_location.y;
                break;
        }
        if (p_room_shape.contains(check_point))
        {
            int from_side_no = p_is_90_degree ? i : 2 * i;
            return CalcFromSide(from_side_no, geometry::planar::FloatPoint(border_x, border_y));
        }
    }
    if (p_is_90_degree)
    {
        return CalcFromSide::NOT_CALCULATED;
    }
    // try the diagonal directions
    const double dist = p_dist / geometry::planar::Limits::SQRT2;
    const double border_dist = via_box.max_width() / (2.0 * geometry::planar::Limits::SQRT2);
    for (int i = 0; i < 4; ++i)
    {
        geometry::planar::FloatPoint check_point;
        double border_x = 0.0;
        double border_y = 0.0;
        switch (i)
        {
            case 0:
                check_point = geometry::planar::FloatPoint(p_via_location.x + dist, p_via_location.y - dist);
                border_x = p_via_location.x + border_dist;
                border_y = p_via_location.y - border_dist;
                break;
            case 1:
                check_point = geometry::planar::FloatPoint(p_via_location.x + dist, p_via_location.y + dist);
                border_x = p_via_location.x + border_dist;
                border_y = p_via_location.y + border_dist;
                break;
            case 2:
                check_point = geometry::planar::FloatPoint(p_via_location.x - dist, p_via_location.y + dist);
                border_x = p_via_location.x - border_dist;
                border_y = p_via_location.y + border_dist;
                break;
            default: // i == 3
                check_point = geometry::planar::FloatPoint(p_via_location.x - dist, p_via_location.y - dist);
                border_x = p_via_location.x - border_dist;
                border_y = p_via_location.y - border_dist;
                break;
        }
        if (p_room_shape.contains(check_point))
        {
            return CalcFromSide(2 * i + 1, geometry::planar::FloatPoint(border_x, border_y));
        }
    }
    return CalcFromSide::NOT_CALCULATED;
}

} // namespace freerouting::board
