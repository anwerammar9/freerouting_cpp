#include "board/Via.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "board/ConductionArea.h"
#include "board/Padstack.h"
#include "board/Trace.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/Shape.h"
#include "board/SearchTreeManager.h"
#include "board/ShapeSearchTree.h"
#include "board/BasicBoard.h"
#include "logger/FRLogger.h"

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

int DrillItem::tile_shape_count() const
{
    if (padstack_ == nullptr)
    {
        return 1;
    }
    return padstack_->get_to_layer() - padstack_->get_from_layer() + 1;
}

int DrillItem::shape_layer(int p_index) const
{
    // Java DrillItem.shapeLayer
    const int index = std::min(std::max(p_index, 0), last_layer() - first_layer());
    return first_layer() + index;
}

const geometry::planar::Shape* DrillItem::get_shape(int p_index) const
{
    if (!shapes_precalculated_)
    {
        // All shapes are calculated at once, like in Java.
        precalculated_shapes_ = calculate_shapes();
        shapes_precalculated_ = true;
    }
    if (p_index < 0 || p_index >= static_cast<int>(precalculated_shapes_.size()))
    {
        return nullptr;
    }
    return precalculated_shapes_[p_index].get();
}

std::vector<std::unique_ptr<geometry::planar::Shape>> DrillItem::calculate_shapes() const
{
    return {};
}

geometry::planar::IntOctagon DrillItem::get_tile_shape(int p_index) const
{
    // Java Item.getTileShape returns the tree shape in the default search tree.
    if (padstack_ != nullptr && board_ != nullptr && board_->get_search_tree_manager() != nullptr)
    {
        const ShapeSearchTree* default_tree = board_->get_search_tree_manager()->get_default_tree();
        if (default_tree != nullptr)
        {
            if (const geometry::planar::TileShape* tree_shape = get_tree_shape(*default_tree, p_index))
            {
                return tree_shape->bounding_octagon();
            }
        }
    }
    return Item::get_tile_shape(p_index);
}

void DrillItem::clear_derived_data()
{
    Item::clear_derived_data();
    precalculated_shapes_.clear();
    shapes_precalculated_ = false;
}

std::vector<std::shared_ptr<geometry::planar::TileShape>> DrillItem::calculate_tree_shapes(
    const ShapeSearchTree& p_search_tree) const
{
    if (padstack_ == nullptr)
    {
        // C++ only: drill items created without a library padstack keep the
        // bounding-box tree shape of the Item base class.
        return Item::calculate_tree_shapes(p_search_tree);
    }
    return p_search_tree.calculate_tree_shapes(*this);
}

std::vector<std::unique_ptr<geometry::planar::Shape>> Via::calculate_shapes() const
{
    // Port of Java Via.getShape.
    std::vector<std::unique_ptr<geometry::planar::Shape>> result;
    if (padstack_ == nullptr)
    {
        logger::FRLogger::instance().warn("Via.get_shape: padstack is null");
        return result;
    }
    const int shape_count = padstack_->get_to_layer() - padstack_->get_from_layer() + 1;
    const geometry::planar::IntPoint& center = get_center();
    const geometry::planar::IntVector translate_vector(center.x, center.y);
    for (int i = 0; i < shape_count; ++i)
    {
        const geometry::planar::Shape* curr_shape = padstack_->get_shape(i + first_layer());
        result.push_back(curr_shape != nullptr ? translate_pad_shape(*curr_shape, translate_vector) : nullptr);
    }
    return result;
}

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

const geometry::planar::TileShape* DrillItem::get_tree_shape_on_layer(
    const ShapeSearchTree* p_search_tree, int p_layer) const
{
    // Mirrors Java DrillItem.getTreeShapeOnLayer(ShapeSearchTree, int).
    if (p_layer < from_layer_ || p_layer > to_layer_)
    {
        logger::FRLogger::instance().warn("DrillItem.get_tree_shape_on_layer: layer out of range");
        return nullptr;
    }
    if (p_search_tree == nullptr)
    {
        return nullptr;
    }
    return get_tree_shape(*p_search_tree, p_layer - from_layer_);
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
    ItemSet result_set;
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

const geometry::planar::Point* DrillItem::normal_contact_point(const Item* p_other) const
{
    if (p_other == nullptr)
    {
        return nullptr;
    }
    if (const Trace* other_trace = dynamic_cast<const Trace*>(p_other))
    {
        // Mirrors Java DrillItem.normal_contact_point(Trace): the drill center
        // is the unique contact point iff it equals a trace corner on a
        // shared layer.
        if (!shares_layer(p_other))
        {
            return nullptr;
        }
        if (center_.equals(&other_trace->first_corner())
            || center_.equals(&other_trace->last_corner()))
        {
            return &center_;
        }
        return nullptr;
    }
    if (const DrillItem* other_drill = dynamic_cast<const DrillItem*>(p_other))
    {
        // Mirrors Java DrillItem.normal_contact_point(DrillItem): contact iff
        // centers are equal and the drill items share a layer.
        if (shares_layer(p_other) && center_.equals(&other_drill->get_center()))
        {
            return &center_;
        }
        return nullptr;
    }
    return nullptr;
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
    // Mirrors Java DrillItem.first_layer/last_layer: the layer range of a drill
    // item is derived from its padstack (Java isPlacedOnFront always returns
    // true for the headless subset).
    if (padstack_ != nullptr)
    {
        from_layer_ = padstack_->get_from_layer();
        to_layer_ = padstack_->get_to_layer();
    }
}

void DrillItem::set_padstack(Padstack* p_padstack)
{
    padstack_ = p_padstack;
    if (padstack_ != nullptr)
    {
        from_layer_ = padstack_->get_from_layer();
        to_layer_ = padstack_->get_to_layer();
    }
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

bool Via::is_tail() const
{
    std::vector<Item*> contact_list = get_normal_contacts();
    if (contact_list.size() <= 1)
    {
        return true;
    }
    auto it = contact_list.begin();
    Item* current_contact_item = *it;
    int first_contact_first_layer = current_contact_item->first_layer();
    int first_contact_last_layer = current_contact_item->last_layer();
    for (++it; it != contact_list.end(); ++it)
    {
        current_contact_item = *it;
        if (current_contact_item->first_layer() != first_contact_first_layer
            || current_contact_item->last_layer() != first_contact_last_layer)
        {
            return false;
        }
    }
    return true;
}

} // namespace freerouting::board