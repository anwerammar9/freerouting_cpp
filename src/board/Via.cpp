#include "board/Via.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "board/ConductionArea.h"
#include "board/Padstack.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Shape.h"

namespace freerouting::board {

namespace {

/**
 * Checks, whether the Manhattan distance between p_p1 and p_p2 is not larger
 * than p_tolerance. Mirrors Java DrillItem.isWithinTolerance.
 */
bool is_within_tolerance(const geometry::planar::Point& p_p1,
                         const geometry::planar::Point& p_p2,
                         int p_tolerance)
{
    const geometry::planar::FloatPoint fp1 = p_p1.to_float();
    const geometry::planar::FloatPoint fp2 = p_p2.to_float();
    const double dx = std::abs(fp1.x - fp2.x);
    const double dy = std::abs(fp1.y - fp2.y);
    return (dx + dy) <= static_cast<double>(p_tolerance);
}

} // namespace

double DrillItem::min_width() const
{
    double result = std::numeric_limits<double>::max();
    if (padstack_ != nullptr)
    {
        for (int layer = padstack_->get_from_layer(); layer <= padstack_->get_to_layer(); ++layer)
        {
            const geometry::planar::Shape* shape = padstack_->get_shape(layer);
            if (shape != nullptr)
            {
                geometry::planar::IntBox box = shape->bounding_box();
                result = std::min(result, static_cast<double>(box.width()));
                result = std::min(result, static_cast<double>(box.height()));
            }
        }
    }
    return result;
}

std::vector<Item*> DrillItem::get_normal_contacts() const
{
    std::vector<Item*> result;
    if (board_ == nullptr)
    {
        return result;
    }
    // Use tolerance for connectivity detection: min_width/2 + 1.
    // This matches the tolerance used in Trace (half_width + 1).
    const int tolerance = static_cast<int>(min_width() / 2.0) + 1;
    // Java queries with the degenerate point box TileShape.get_instance(center);
    // the C++ MinAreaTree needs non-degenerate bounding shapes, so we use a
    // tolerance-sized box and filter the candidates afterwards.
    const geometry::planar::FloatPoint center_fp = get_center().to_float();
    const geometry::planar::IntBox search_shape(
        static_cast<int64_t>(center_fp.x) - tolerance,
        static_cast<int64_t>(center_fp.y) - tolerance,
        static_cast<int64_t>(center_fp.x) + tolerance,
        static_cast<int64_t>(center_fp.y) + tolerance);
    // Java ignores the layer in the tree query (overlapping_objects(shape, -1));
    // the layer constraint is checked below via shares_layer.
    const std::set<SearchTreeObject*, TreeStorableLess> overlaps =
        board_->overlapping_objects(search_shape, -1);
    std::set<Item*> result_set;
    for (SearchTreeObject* curr_ob : overlaps)
    {
        Item* curr_item = dynamic_cast<Item*>(curr_ob);
        if (curr_item == nullptr || curr_item == this)
        {
            continue;
        }
        if (!curr_item->shares_net(this) || !curr_item->shares_layer(this))
        {
            continue;
        }
        if (const auto* curr_trace = dynamic_cast<const Trace*>(curr_item))
        {
            if (is_within_tolerance(get_center(), curr_trace->first_corner(), tolerance) ||
                is_within_tolerance(get_center(), curr_trace->last_corner(), tolerance))
            {
                result_set.insert(curr_item);
            }
        }
        else if (const auto* curr_drill_item = dynamic_cast<const DrillItem*>(curr_item))
        {
            if (is_within_tolerance(get_center(), curr_drill_item->get_center(), tolerance))
            {
                result_set.insert(curr_item);
            }
        }
        else if (const auto* curr_area = dynamic_cast<const ConductionArea*>(curr_item))
        {
            if (curr_area->get_area()->contains(get_center()))
            {
                result_set.insert(curr_item);
            }
        }
    }
    result.insert(result.end(), result_set.begin(), result_set.end());
    return result;
}

std::vector<std::shared_ptr<geometry::planar::Point>> DrillItem::get_ratsnest_corners() const
{
    std::vector<std::shared_ptr<geometry::planar::Point>> result;
    result.push_back(std::make_shared<geometry::planar::IntPoint>(get_center()));
    return result;
}

DrillItem::DrillItem(const geometry::planar::IntPoint& p_center,
                       const std::vector<int>& p_net_nos,
                       int p_clearance_class,
                       int p_id_no,
                       int p_component_no,
                       FixedState p_fixed_state,
                       BasicBoard* p_board)
    : Item(p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      center_(p_center),
      from_layer_(0),
      to_layer_(0),
      padstack_(nullptr)
{
}

Via::Via(Padstack* p_padstack,
             const geometry::planar::IntPoint& p_center,
             const std::vector<int>& p_net_nos,
             int p_clearance_class,
             int p_id_no,
             int p_component_no,
             FixedState p_fixed_state,
             bool p_attach_allowed,
             BasicBoard* p_board)
    : DrillItem(p_center, p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      attach_allowed_(p_attach_allowed)
{
    padstack_ = p_padstack;
}

Item* Via::copy(int p_id_no) const
{
    return new Via(padstack_, get_center(), net_nos_, clearance_class_,
                   p_id_no, component_no_, fixed_state_, attach_allowed_, board_);
}

std::string Via::to_string() const
{
    return "Via[id=" + std::to_string(id_no_) + "]";
}

const geometry::planar::TileShape* Via::get_tree_shape_on_layer(ShapeSearchTree* p_search_tree, int p_layer)
{
    if (!padstack_)
    {
        return nullptr;
    }
    int layer_index = p_layer - from_layer_;
    if (layer_index < 0 || layer_index >= tile_shape_count())
    {
        return nullptr;
    }
    return get_tree_shape(*p_search_tree, layer_index);
}

const geometry::planar::TileShape* Via::get_tree_shape_on_layer(const ShapeSearchTree* p_search_tree, int p_layer) const
{
    if (!padstack_)
    {
        return nullptr;
    }
    int layer_index = p_layer - from_layer_;
    if (layer_index < 0 || layer_index >= tile_shape_count())
    {
        return nullptr;
    }
    return get_tree_shape(*p_search_tree, layer_index);
}

} // namespace freerouting::board