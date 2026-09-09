#include "board/MoveComponent.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

#include "board/Component.h"
#include "board/ComponentOutline.h"
#include "board/Item.h"
#include "board/MoveDrillItemAlgo.h"
#include "board/ObstacleArea.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "datastructures/TimeLimit.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting::board {

namespace {

constexpr int64_t PULL_TIGHT_TIME_LIMIT = 1000;
constexpr int64_t CHECK_TIME_LIMIT = 3000;

geometry::planar::IntVector to_int_vector(const geometry::planar::Vector& p_vector)
{
    geometry::planar::FloatPoint f = p_vector.to_float();
    return geometry::planar::IntVector(static_cast<int64_t>(std::llround(f.x)),
                                       static_cast<int64_t>(std::llround(f.y)));
}

/**
 * Simplified port of Java RoutingBoard.check_move_item: checks that the item
 * can be translated without clearance violations against items not in
 * p_ignore_items. Multi-net items and connected traces are rejected, mirroring
 * the Java implementation.
 */
bool check_move_item(RoutingBoard* p_board,
                     Item* p_item,
                     const geometry::planar::IntVector& p_vector,
                     std::vector<Item*>& p_ignore_items)
{
    if (p_board == nullptr || p_item == nullptr)
    {
        return false;
    }
    if (p_item->get_net_nos().size() > 1)
    {
        return false; // not yet implemented (mirrors Java)
    }
    if (const auto* trace = dynamic_cast<const Trace*>(p_item))
    {
        if (!trace->get_normal_contacts().empty())
        {
            return false;
        }
    }
    p_ignore_items.push_back(p_item);
    const geometry::planar::IntBox board_box = p_board->get_bounding_box();
    const int shape_count = p_item->tile_shape_count();
    for (int i = 0; i < shape_count; ++i)
    {
        geometry::planar::IntOctagon moved_shape = p_item->get_tile_shape(i).translate_by(p_vector);
        if (!moved_shape.bounding_box().is_contained_in(board_box))
        {
            return false;
        }
        std::set<Item*, TreeStorableLess> obstacles = p_board->overlapping_items_with_clearance(
            moved_shape, p_item->shape_layer(i), p_item->get_net_nos(), p_item->get_clearance_class());
        for (Item* curr : obstacles)
        {
            if (curr == p_item)
            {
                continue;
            }
            if (std::find(p_ignore_items.begin(), p_ignore_items.end(), curr) != p_ignore_items.end())
            {
                continue;
            }
            if (curr->is_obstacle(p_item))
            {
                return false;
            }
        }
    }
    return true;
}

geometry::planar::FloatPoint item_center_float(const Item* p_item)
{
    if (const auto* drill = dynamic_cast<const DrillItem*>(p_item))
    {
        return drill->get_center().to_float();
    }
    return p_item->get_bounding_box().centre_of_gravity();
}

} // namespace

MoveComponent::MoveComponent(Item* p_item,
                             const geometry::planar::Vector& p_translate_vector,
                             int p_max_recursion_depth,
                             int p_max_via_recursion_depth)
    : translate_vector_(to_int_vector(p_translate_vector))
    , max_recursion_depth_(p_max_recursion_depth)
    , max_via_recursion_depth_(p_max_via_recursion_depth)
{
    if (p_item == nullptr)
    {
        all_items_movable_ = false;
        return;
    }
    board_ = dynamic_cast<RoutingBoard*>(p_item->get_board());
    if (board_ == nullptr)
    {
        all_items_movable_ = false;
        return;
    }

    std::vector<Item*> item_group_list;
    const int component_no = p_item->get_component_no();
    if (component_no > 0)
    {
        // TODO: resolve the Component object once RoutingBoard exposes its
        // components (Java: board.components.get(component_no) + move/notify
        // in insert). Grouping by component number already works.
        const int item_count = board_->get_item_count();
        for (int i = 0; i < item_count; ++i)
        {
            Item* curr = board_->get_item(i);
            if (curr != nullptr && curr->get_component_no() == component_no)
            {
                item_group_list.push_back(curr);
            }
        }
    }
    else
    {
        item_group_list.push_back(p_item);
    }

    std::vector<geometry::planar::FloatPoint> item_centers;
    for (Item* curr_item : item_group_list)
    {
        const bool curr_movable = !curr_item->is_user_fixed()
            && (dynamic_cast<DrillItem*>(curr_item) != nullptr
                || dynamic_cast<ObstacleArea*>(curr_item) != nullptr
                || dynamic_cast<ComponentOutline*>(curr_item) != nullptr);
        if (!curr_movable)
        {
            // MoveComponent currently only implemented for DrillItems
            all_items_movable_ = false;
            item_group_arr_.clear();
            return;
        }
        if (dynamic_cast<DrillItem*>(curr_item) != nullptr)
        {
            item_centers.push_back(item_center_float(curr_item));
        }
    }
    if (item_group_list.empty())
    {
        all_items_movable_ = false;
        return;
    }
    // calculate the gravity point of all drill item centers (0,0 if none)
    double gravity_x = 0.0;
    double gravity_y = 0.0;
    for (const geometry::planar::FloatPoint& curr_center : item_centers)
    {
        gravity_x += curr_center.x;
        gravity_y += curr_center.y;
    }
    if (!item_centers.empty())
    {
        gravity_x /= static_cast<double>(item_centers.size());
        gravity_y /= static_cast<double>(item_centers.size());
    }
    const geometry::planar::FloatPoint translate_f = translate_vector_.to_float();
    std::vector<SortedItem> sorted;
    sorted.reserve(item_group_list.size());
    for (Item* curr_item : item_group_list)
    {
        const geometry::planar::FloatPoint center = item_center_float(curr_item);
        const double projection =
            (gravity_x - center.x) * translate_f.x + (gravity_y - center.y) * translate_f.y;
        sorted.push_back(SortedItem{curr_item, projection});
    }
    // sort the items in the direction of p_translate_vector, so that
    // the items in front come first.
    std::sort(sorted.begin(), sorted.end());
    item_group_arr_.reserve(sorted.size());
    for (const SortedItem& entry : sorted)
    {
        item_group_arr_.push_back(entry.item);
    }
}

bool MoveComponent::check()
{
    if (!all_items_movable_ || board_ == nullptr)
    {
        return false;
    }
    datastructures::TimeLimit time_limit(CHECK_TIME_LIMIT);
    std::vector<Item*> ignore_items;
    for (Item* curr_item : item_group_arr_)
    {
        bool move_ok = false;
        if (auto* curr_drill = dynamic_cast<DrillItem*>(curr_item))
        {
            const double min_width =
                static_cast<double>(curr_drill->get_bounding_box().min_width());
            if (translate_vector_.length_approx() >= min_width)
            {
                // a clearance violation with a connecting trace may occur
                move_ok = false;
            }
            else
            {
                move_ok = MoveDrillItemAlgo::check(curr_drill, translate_vector_, max_recursion_depth_,
                                                  max_via_recursion_depth_, ignore_items, board_,
                                                  &time_limit);
            }
        }
        else
        {
            move_ok = check_move_item(board_, curr_item, translate_vector_, ignore_items);
        }
        if (!move_ok)
        {
            return false;
        }
    }
    return true;
}

bool MoveComponent::insert(int p_tidy_width, int p_pull_tight_accuracy)
{
    (void)p_tidy_width;
    (void)p_pull_tight_accuracy;
    if (!all_items_movable_ || board_ == nullptr)
    {
        return false;
    }
    if (component_ != nullptr)
    {
        // TODO: move the component first once RoutingBoard exposes components
        // (Java: board.components.move + observers.notify_moved), so that the
        // new pin shapes are calculated correctly.
    }
    for (Item* curr_item : item_group_arr_)
    {
        if (auto* curr_drill = dynamic_cast<DrillItem*>(curr_item))
        {
            // Java routes through RoutingBoard.move_drill_item (shove + tidy +
            // pull-tight); the shove core is MoveDrillItemAlgo::insert, the
            // tidy/opt step is a TODO until opt_changed_area wiring is done.
            if (!MoveDrillItemAlgo::insert(curr_drill, translate_vector_, max_recursion_depth_,
                                           max_via_recursion_depth_, nullptr, board_))
            {
                return false;
            }
        }
        else if (auto* obstacle = dynamic_cast<ObstacleArea*>(curr_item))
        {
            obstacle->translate_by(translate_vector_);
        }
        else if (auto* outline = dynamic_cast<ComponentOutline*>(curr_item))
        {
            outline->translate_by(translate_vector_);
        }
        else
        {
            return false;
        }
    }
    return true;
}

} // namespace freerouting::board
