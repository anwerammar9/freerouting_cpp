#include "board/BasicBoard.h"
#include "board/ChangedArea.h"
#include "board/Item.h"
#include "board/ItemSelectionFilter.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/SearchTreeManager.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Polyline.h"
#include "geometry/planar/TileShape.h"
#include "rules/BoardRules.h"

#include <algorithm>

namespace freerouting::board {

namespace {

geometry::planar::IntPoint to_int_point(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return *int_point;
    }
    return p_point.to_float().round();
}

} // namespace

BasicBoard::BasicBoard(const LayerStructure& p_layer_structure,
                       std::unique_ptr<rules::BoardRules> p_rules)
    : layer_structure_(p_layer_structure)
    , rules_(std::move(p_rules))
    , search_tree_manager_(std::make_unique<SearchTreeManager>(this))
{
}

BasicBoard::~BasicBoard() = default;

geometry::planar::IntBox BasicBoard::get_bounding_box() const
{
    return geometry::planar::IntBox::EMPTY;
}

std::set<SearchTreeObject*, TreeStorableLess> BasicBoard::overlapping_objects(
    const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer) const
{
    return search_tree_manager_->get_default_tree()->overlapping_objects(p_shape, p_layer);
}

std::set<Item*, TreeStorableLess> BasicBoard::overlapping_items_with_clearance(
    const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer,
    const std::vector<int>& p_ignore_net_nos, int p_clearance_class) const
{
    return search_tree_manager_->get_default_tree()->overlapping_items_with_clearance(
        p_shape, p_layer, p_ignore_net_nos, p_clearance_class);
}

std::set<Item*> BasicBoard::pick_items(const geometry::planar::Point& p_location, int p_layer,
                                       const ItemSelectionFilter* p_filter) const
{
    using namespace geometry::planar;
    IntPoint location;
    if (const auto* int_location = dynamic_cast<const IntPoint*>(&p_location))
    {
        location = *int_location;
    }
    else
    {
        location = p_location.to_float().round();
    }
    geometry::planar::IntBox point_shape =
        geometry::planar::TileShape::get_instance(location);
    std::set<SearchTreeObject*, TreeStorableLess> overlaps = overlapping_objects(point_shape, p_layer);
    std::set<Item*> result;
    for (SearchTreeObject* curr : overlaps)
    {
        if (auto* item = dynamic_cast<Item*>(curr))
        {
            result.insert(item);
        }
    }
    if (p_filter != nullptr)
    {
        return p_filter->filter(result);
    }
    return result;
}

bool BasicBoard::check_trace_shape(const geometry::planar::TileShape& p_shape,
                                     int p_layer,
                                     const std::vector<int>& p_net_no_arr,
                                     int p_cl_class,
                                     const std::set<Pin*>* p_contact_pins) const
{
    if (const auto* routing_board = dynamic_cast<const RoutingBoard*>(this))
    {
        if (!p_shape.bounding_box().is_contained_in(routing_board->get_bounding_box()))
        {
            return false;
        }
    }
    ShapeSearchTree* default_tree = search_tree_manager_->get_default_tree();
    std::vector<int> ignore_net_nos;
    std::vector<datastructures::ShapeTree::TreeEntry> tree_entries;
    if (default_tree->is_clearance_compensation_used())
    {
        default_tree->overlapping_tree_entries(p_shape, p_layer, ignore_net_nos, tree_entries);
    }
    else
    {
        tree_entries = default_tree->overlapping_tree_entries_with_clearance(
            p_shape, p_layer, ignore_net_nos, p_cl_class);
    }
    for (const auto& curr_entry : tree_entries)
    {
        auto* curr_item = dynamic_cast<Item*>(curr_entry.object);
        if (curr_item == nullptr)
        {
            continue;
        }
        if (p_contact_pins != nullptr)
        {
            auto* curr_pin = dynamic_cast<Pin*>(curr_item);
            if (curr_pin != nullptr)
            {
                if (p_contact_pins->count(curr_pin) > 0)
                {
                    continue;
                }
                // The contact pins of the trace should be exempted.
                // Other pins are handled as obstacles to avoid acid traps.
                return false;
            }
        }
        bool is_obstacle = true;
        for (int net_no : p_net_no_arr)
        {
            if (!curr_item->is_trace_obstacle(net_no))
            {
                is_obstacle = false;
                break;
            }
        }
        // TODO: tie-pin exception for foreign-net traces fully inside a
        // multi-net contact pin shape (needs Pin tile shapes on layer).
        if (is_obstacle)
        {
            return false;
        }
    }
    return true;
}

PolylineTrace* BasicBoard::insert_trace_without_cleaning(
    const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
    int p_layer,
    int p_half_width,
    const std::vector<int>& p_net_no_arr,
    int p_clearance_class,
    FixedState p_fixed_state)
{
    if (p_polyline == nullptr || p_polyline->corner_count() < 2)
    {
        return nullptr;
    }
    auto* new_trace = new PolylineTrace(p_polyline, p_layer, p_half_width, p_net_no_arr,
                                        p_clearance_class, generate_item_id(), 0, p_fixed_state);
    if (new_trace->first_corner().to_float() == new_trace->last_corner().to_float()
        && static_cast<int>(p_fixed_state) < static_cast<int>(FixedState::USER_FIXED))
    {
        delete new_trace;
        return nullptr;
    }
    new_trace->set_board(this);
    search_tree_manager_->insert(new_trace);
    // Java insert_item registers the trace in the item list as well; keep the
    // list in sync when the board maintains one. (add_item callers remain
    // list-only until the tree/list duality is unified.)
    if (auto* routing_board = dynamic_cast<RoutingBoard*>(this))
    {
        routing_board->add_item(new_trace);
    }
    if (new_trace->nets_normal())
    {
        max_trace_half_width_ = std::max(max_trace_half_width_, p_half_width);
        min_trace_half_width_ = std::min(min_trace_half_width_, p_half_width);
    }
    return new_trace;
}

void BasicBoard::insert_trace(const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
                              int p_layer,
                              int p_half_width,
                              const std::vector<int>& p_net_no_arr,
                              int p_clearance_class,
                              FixedState p_fixed_state)
{
    // Port of Java BasicBoard.insertTrace(Polyline, int, int, int[], int, FixedState).
    PolylineTrace* new_trace = insert_trace_without_cleaning(p_polyline, p_layer, p_half_width,
                                                             p_net_no_arr, p_clearance_class,
                                                             p_fixed_state);
    if (new_trace == nullptr)
    {
        return;
    }
    geometry::planar::IntOctagon clip_shape;
    bool has_clip_shape = false;
    if (auto* routing_board = dynamic_cast<const RoutingBoard*>(this))
    {
        if (const ChangedArea* changed_area = routing_board->changed_area())
        {
            clip_shape = changed_area->get_area(p_layer);
            has_clip_shape = true;
        }
    }
    new_trace->normalize(has_clip_shape ? &clip_shape : nullptr);
}

bool BasicBoard::check_polyline_trace(const geometry::planar::Polyline& p_polyline,
                                      int p_layer,
                                      int p_pen_half_width,
                                      const std::vector<int>& p_net_no_arr,
                                      int p_clearance_class) const
{
    // Port of Java BasicBoard.checkPolylineTrace(): a temporary trace is built
    // from the polyline to determine the pins at its end corners, which are the
    // contact pins exempted from the clearance check.
    PolylineTrace tmp_trace(std::make_shared<geometry::planar::Polyline>(p_polyline), p_layer,
                            p_pen_half_width, p_net_no_arr, p_clearance_class, 0, 0,
                            FixedState::NOT_FIXED);
    std::set<Pin*> contact_pins = tmp_trace.touching_pins_at_end_corners();
    for (int i = 0; i < tmp_trace.tile_shape_count(); i++)
    {
        if (!check_trace_shape(tmp_trace.get_tile_shape(i), p_layer, p_net_no_arr,
                               p_clearance_class, &contact_pins))
        {
            return false;
        }
    }
    return true;
}

bool BasicBoard::split_traces(const geometry::planar::Point& p_location, int p_layer, int p_net_no)
{
    ItemSelectionFilter filter(ItemSelectionFilter::SelectableChoices::TRACES);
    std::set<Item*> picked_items = pick_items(p_location, p_layer, &filter);
    geometry::planar::IntBox location_box =
        geometry::planar::TileShape::get_instance(to_int_point(p_location));
    geometry::planar::IntOctagon location_shape = location_box.bounding_octagon();
    bool trace_split = false;
    for (Item* curr_item : picked_items)
    {
        auto* curr_trace = dynamic_cast<Trace*>(curr_item);
        if (curr_trace != nullptr && curr_trace->contains_net(p_net_no))
        {
            std::vector<PolylineTrace*> split_pieces = curr_trace->split(&location_shape);
            if (split_pieces.size() != 1)
            {
                trace_split = true;
            }
        }
    }
    return trace_split;
}

bool BasicBoard::remove_if_cycle(Trace* p_trace)
{
    if (p_trace == nullptr || !p_trace->is_on_board() || !p_trace->is_cycle())
    {
        return false;
    }
    auto* routing_board = dynamic_cast<RoutingBoard*>(this);
    if (routing_board == nullptr)
    {
        return false;
    }
    const int curr_layer = p_trace->get_layer();
    const std::vector<int>& curr_net_no_arr = p_trace->get_net_nos();
    const geometry::planar::Point* end_corners[2] = {&p_trace->first_corner(),
                                                     &p_trace->last_corner()};
    bool tail_at_endpoint_before[2] = {false, false};
    for (int i = 0; i < 2; ++i)
    {
        tail_at_endpoint_before[i] =
            routing_board->get_trace_tail(*end_corners[i], curr_layer, curr_net_no_arr)
            != nullptr;
    }
    std::vector<Item*> connection_items =
        p_trace->get_connection_items(Item::StopConnectionOption::NONE);
    routing_board->remove_items(connection_items);
    for (Item* curr_item : connection_items)
    {
        search_tree_manager_->remove(curr_item);
    }
    for (int i = 0; i < 2; ++i)
    {
        if (!tail_at_endpoint_before[i])
        {
            Trace* tail =
                routing_board->get_trace_tail(*end_corners[i], curr_layer, curr_net_no_arr);
            if (tail != nullptr)
            {
                std::vector<Item*> tail_items =
                    tail->get_connection_items(Item::StopConnectionOption::NONE);
                routing_board->remove_items(tail_items);
                for (Item* curr_item : tail_items)
                {
                    search_tree_manager_->remove(curr_item);
                }
            }
        }
    }
    return true;
}



} // namespace freerouting::board