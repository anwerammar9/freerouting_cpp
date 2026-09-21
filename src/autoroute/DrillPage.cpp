#include "autoroute/DrillPage.h"

#include <optional>
#include <utility>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "board/Item.h"
#include "board/Pin.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "datastructures/ShapeTree.h"
#include "geometry/planar/Simplex.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

namespace {

using geometry::planar::IntBox;
using geometry::planar::Simplex;
using geometry::planar::TileShape;

/**
 * Calculates the intersection of the two convex tile shapes p_a and p_b.
 * In the common all-IntBox case this is exact box intersection; otherwise the
 * shapes are intersected as Simplices and the result is simplified to the
 * simplest physical representation. Mirrors the polymorphic Java
 * TileShape.intersection() for the shape combinations occurring in drill page
 * calculation.
 */
std::unique_ptr<TileShape> tile_intersection(const TileShape& p_a, const TileShape& p_b)
{
    if (const auto* box_a = dynamic_cast<const IntBox*>(&p_a))
    {
        if (const auto* box_b = dynamic_cast<const IntBox*>(&p_b))
        {
            return std::make_unique<IntBox>(box_a->intersection(*box_b));
        }
    }
    Simplex intersection = p_a.to_Simplex().intersection(p_b.to_Simplex());
    return intersection.simplify();
}

/**
 * Returns true if the convex shape p_inner is geometrically contained in the
 * convex shape p_outer (every bounded corner of p_inner is inside p_outer).
 * Mirrors the Java TileShape.contains(TileShape) check used to avoid multiple
 * cutouts of equal obstacle shapes (vias with the same shape on all layers).
 */
bool tile_contains(const TileShape& p_outer, const TileShape& p_inner)
{
    int corner_count = p_inner.border_line_count();
    for (int i = 0; i < corner_count; ++i)
    {
        if (!p_inner.corner_is_bounded(i))
        {
            continue;
        }
        if (!p_outer.contains(p_inner.corner(i)))
        {
            return false;
        }
    }
    return true;
}

/**
 * Splits the convex base shape into convex pieces with the holes cut out.
 * Replicates the C++ PolylineArea::split_to_convex() algorithm, which is itself
 * a faithful port of the Java PolylineArea.splitToConvex() used by
 * DrillPage.getDrills(). Holes that cover a whole piece remove it (dimension < 2
 * pieces are dropped).
 */
std::vector<std::unique_ptr<TileShape>> split_to_convex_with_holes(
    const TileShape& p_base, const std::vector<std::unique_ptr<TileShape>>& p_holes)
{
    std::vector<std::unique_ptr<TileShape>> result;
    const std::vector<std::unique_ptr<TileShape>>& base_pieces = p_base.split_to_convex();
    result.reserve(base_pieces.size());
    for (const auto& base_piece : base_pieces)
    {
        result.push_back(base_piece->clone());
    }

    for (const auto& hole : p_holes)
    {
        if (hole == nullptr || hole->dimension() < 2)
        {
            continue;
        }
        const std::vector<std::unique_ptr<TileShape>>& hole_pieces = hole->split_to_convex();
        if (hole_pieces.empty())
        {
            return result;
        }
        for (const auto& hole_piece : hole_pieces)
        {
            std::vector<std::unique_ptr<TileShape>> new_piece_list;
            for (const auto& divide_piece : result)
            {
                std::vector<std::unique_ptr<TileShape>> cut_pieces = divide_piece->cutout(*hole_piece);
                for (auto& piece : cut_pieces)
                {
                    if (piece->dimension() == 2)
                    {
                        new_piece_list.push_back(std::move(piece));
                    }
                }
            }
            result = std::move(new_piece_list);
            if (result.empty())
            {
                return result;
            }
        }
    }
    return result;
}

/**
 * Looks if p_drill_shape contains the center of a drillable Pin on p_layer.
 * Returns nullopt if no such Pin was found. Mirrors Java
 * DrillPage.calcPinCenterInDrill().
 */
std::optional<geometry::planar::IntPoint> calc_pin_center_in_drill(
    const TileShape& p_drill_shape, int p_layer, board::RoutingBoard* p_board)
{
    std::optional<geometry::planar::IntPoint> result;
    if (p_board == nullptr)
    {
        return result;
    }
    for (auto* current_object : p_board->overlapping_objects(p_drill_shape, p_layer))
    {
        auto* current_pin = dynamic_cast<board::Pin*>(current_object);
        if (current_pin == nullptr || !current_pin->drill_allowed())
        {
            continue;
        }
        const geometry::planar::IntPoint& center = current_pin->get_center();
        if (p_drill_shape.contains_inside(center))
        {
            result = center;
        }
    }
    return result;
}

} // namespace

DrillPage::DrillPage(const geometry::planar::IntBox& p_shape, board::RoutingBoard* p_board)
    : shape(p_shape), board_(p_board)
{
    if (board_ != nullptr)
    {
        maze_search_elements_.resize(board_->get_layer_count());
    }
}

int DrillPage::maze_search_element_count() const
{
    return static_cast<int>(maze_search_elements_.size());
}

MazeSearchElement* DrillPage::get_maze_search_element(int p_no)
{
    if (p_no >= 0 && p_no < static_cast<int>(maze_search_elements_.size()))
    {
        return &maze_search_elements_[p_no];
    }
    return nullptr;
}

void DrillPage::reset()
{
    for (auto& drill : drills_)
    {
        drill->reset();
    }
    for (auto& element : maze_search_elements_)
    {
        element.reset();
    }
}

std::vector<ExpansionDrill*> DrillPage::get_drills(AutorouteEngine* p_autoroute_engine,
                                                   bool p_attach_smd)
{
    if (p_autoroute_engine == nullptr || board_ == nullptr)
    {
        return {};
    }
    if (!drills_.empty() && p_autoroute_engine->get_net_no() == net_no_)
    {
        std::vector<ExpansionDrill*> existing;
        existing.reserve(drills_.size());
        for (auto& drill : drills_)
        {
            existing.push_back(drill.get());
        }
        return existing;
    }

    net_no_ = p_autoroute_engine->get_net_no();
    drills_.clear();

    board::ShapeSearchTree* search_tree = p_autoroute_engine->get_autoroute_search_tree();
    if (search_tree == nullptr)
    {
        return {};
    }

    // Collect the convex shapes of all obstacles overlapping this page.
    std::vector<board::ShapeSearchTree::TreeEntry> overlaps;
    search_tree->overlapping_tree_entries(shape, -1, overlaps);

    std::vector<std::unique_ptr<TileShape>> cutout_shapes;
    std::unique_ptr<TileShape> prev_obstacle_shape = std::make_unique<IntBox>(IntBox::EMPTY);
    for (const auto& current_entry : overlaps)
    {
        auto* current_item = dynamic_cast<board::Item*>(current_entry.object);
        if (current_item == nullptr)
        {
            continue;
        }
        if (current_item->is_drillable(net_no_))
        {
            continue;
        }
        if (auto* current_pin = dynamic_cast<board::Pin*>(current_item))
        {
            if (p_attach_smd && current_pin->drill_allowed())
            {
                // destination smd pins are not obstacles when attaching smd
                continue;
            }
        }
        const TileShape* current_obstacle_shape =
            current_item->get_tree_shape(*search_tree, current_entry.shape_index_in_object);
        if (current_obstacle_shape == nullptr)
        {
            continue;
        }
        // Avoid multiple cutouts for equal obstacle shapes (vias with the same
        // shape on all layers).
        if (!tile_contains(*prev_obstacle_shape, *current_obstacle_shape))
        {
            std::unique_ptr<TileShape> current_cutout_shape =
                tile_intersection(*current_obstacle_shape, shape);
            if (current_cutout_shape != nullptr && current_cutout_shape->dimension() == 2)
            {
                cutout_shapes.push_back(std::move(current_cutout_shape));
            }
        }
        prev_obstacle_shape = current_obstacle_shape->clone();
    }

    std::vector<std::unique_ptr<TileShape>> drill_shapes =
        split_to_convex_with_holes(shape, cutout_shapes);

    const int drill_first_layer = 0;
    const int drill_last_layer = board_->get_layer_count() - 1;

    for (auto& current_drill_shape : drill_shapes)
    {
        std::optional<geometry::planar::IntPoint> current_drill_location;
        if (p_attach_smd)
        {
            current_drill_location =
                calc_pin_center_in_drill(*current_drill_shape, drill_first_layer, board_);
            if (!current_drill_location.has_value())
            {
                current_drill_location =
                    calc_pin_center_in_drill(*current_drill_shape, drill_last_layer, board_);
            }
        }
        if (!current_drill_location.has_value())
        {
            current_drill_location = current_drill_shape->centre_of_gravity().round();
        }

        // Note (WP-9): the C++ autoroute room model is IntBox based. The exact
        // drill shape is only needed for the drill location above; the
        // ExpansionDrill stores its bounding box as a documented simplification.
        auto new_drill = std::make_unique<ExpansionDrill>(
            current_drill_shape->bounding_box(), *current_drill_location,
            drill_first_layer, drill_last_layer);
        if (new_drill->calculate_expansion_rooms(p_autoroute_engine))
        {
            drills_.push_back(std::move(new_drill));
        }
    }

    std::vector<ExpansionDrill*> result;
    result.reserve(drills_.size());
    for (auto& drill : drills_)
    {
        result.push_back(drill.get());
    }
    return result;
}

} // namespace freerouting::autoroute