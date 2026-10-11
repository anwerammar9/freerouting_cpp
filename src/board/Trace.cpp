#include "board/Trace.h"

#include "board/BasicBoard.h"
#include "board/ComponentObstacleArea.h"
#include "board/ConductionArea.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/IntOctagon.h"
#include "board/ViaObstacleArea.h"
#include "logger/FRLogger.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Shape.h"
#include "rules/Net.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace freerouting {
namespace board {

namespace {

/**
 * Checks, whether the area shape of p_area contains p_point, accepting both
 * integer and float points.
 */
bool area_contains_point(const geometry::planar::Area& p_area,
                         const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return p_area.contains(*int_point);
    }
    if (const auto* float_point = dynamic_cast<const geometry::planar::FloatPoint*>(&p_point))
    {
        return p_area.contains(*float_point);
    }
    return false;
}

} // namespace

Trace::Trace(int p_layer, int p_half_width,
             const std::vector<int>& p_net_no_arr,
             int p_clearance_type, int p_id_no, int p_group_no,
             FixedState p_fixed_state)
    : Item(p_net_no_arr, p_clearance_type, p_id_no, p_group_no, p_fixed_state, nullptr)
    , half_width_(p_half_width)
    , layer_(std::max(p_layer, 0))
{
}

void Trace::set_layer(int p_layer)
{
    layer_ = std::max(p_layer, 0);
}

int Trace::get_compensated_half_width(const ShapeSearchTree& p_search_tree) const
{
    return half_width_ + p_search_tree.clearance_compensation_value(get_clearance_class(), layer_);
}

std::string Trace::to_string() const
{
    return "Trace";
}

bool Trace::is_routable() const
{
    return get_fixed_state() != FixedState::USER_FIXED && !net_nos_.empty();
}

ItemSet Trace::get_normal_contacts(const geometry::planar::Point& p_point, bool p_ignore_net) const
{
    ItemSet result;
    const geometry::planar::Point& first = first_corner();
    const geometry::planar::Point& last = last_corner();
    if (!(p_point.equals(&first) || p_point.equals(&last)))
    {
        return result;
    }
    if (board_ == nullptr)
    {
        return result;
    }
    // Java searches with TileShape.getInstance(point), the degenerate box of the point. The C++
    // MinAreaTree needs a non-degenerate box, so the box grows by 1; the candidates are then
    // filtered by Java's exact corner / center equality, so the result is the same.
    const geometry::planar::FloatPoint fp = p_point.to_float();
    const geometry::planar::IntBox search_shape(
        static_cast<int64_t>(std::floor(fp.x)) - 1,
        static_cast<int64_t>(std::floor(fp.y)) - 1,
        static_cast<int64_t>(std::ceil(fp.x)) + 1,
        static_cast<int64_t>(std::ceil(fp.y)) + 1);
    const std::set<SearchTreeObject*, TreeStorableLess> overlaps = board_->overlapping_objects(search_shape, layer_);
    for (SearchTreeObject* curr_ob : overlaps)
    {
        Item* curr_item = dynamic_cast<Item*>(curr_ob);
        if (curr_item == nullptr || curr_item == this)
        {
            continue;
        }
        if (!curr_item->shares_layer(this))
        {
            continue;
        }
        if (!p_ignore_net && !curr_item->shares_net(this))
        {
            continue;
        }
        if (const Trace* curr_trace = dynamic_cast<const Trace*>(curr_item))
        {
            if (p_point.equals(&curr_trace->first_corner()) || p_point.equals(&curr_trace->last_corner()))
            {
                result.insert(curr_item);
            }
        }
        else if (const DrillItem* curr_drill_item = dynamic_cast<const DrillItem*>(curr_item))
        {
            if (p_point.equals(&curr_drill_item->get_center()))
            {
                result.insert(curr_item);
            }
        }
        else if (const ConductionArea* curr_area = dynamic_cast<const ConductionArea*>(curr_item))
        {
            if (curr_area->get_area() != nullptr &&
                area_contains_point(*curr_area->get_area(), p_point))
            {
                result.insert(curr_item);
            }
        }
    }
    return result;
}

ItemSet Trace::get_start_contacts() const
{
    return get_normal_contacts(first_corner(), false);
}

ItemSet Trace::get_end_contacts() const
{
    return get_normal_contacts(last_corner(), false);
}

std::vector<Item*> Trace::get_normal_contacts() const
{
    ItemSet contacts = get_normal_contacts(first_corner(), false);
    const ItemSet end_contacts = get_normal_contacts(last_corner(), false);
    contacts.insert(end_contacts.begin(), end_contacts.end());
    return std::vector<Item*>(contacts.begin(), contacts.end());
}

bool Trace::is_obstacle(const Item* p_other) const
{
    if (p_other == nullptr || p_other == this)
    {
        return false;
    }
    // Mirrors Java Trace.is_obstacle: never an obstacle for via/component
    // keepouts, nor for conduction areas that are not marked as obstacle.
    if (dynamic_cast<const ViaObstacleArea*>(p_other) != nullptr
        || dynamic_cast<const ComponentObstacleArea*>(p_other) != nullptr)
    {
        return false;
    }
    if (const auto* p_conduction_area = dynamic_cast<const ConductionArea*>(p_other))
    {
        if (!p_conduction_area->get_is_obstacle())
        {
            return false;
        }
    }
    return !p_other->shares_net(this);
}

const geometry::planar::Point* Trace::normal_contact_point(const Item* p_other) const
{
    if (p_other == nullptr)
    {
        return nullptr;
    }
    if (const Trace* other_trace = dynamic_cast<const Trace*>(p_other))
    {
        // Java Trace.normal_contact_point(Trace)
        if (layer_ != other_trace->layer_)
        {
            return nullptr;
        }
        const bool contact_at_first_corner = first_corner().equals(&other_trace->first_corner())
            || first_corner().equals(&other_trace->last_corner());
        const bool contact_at_last_corner = last_corner().equals(&other_trace->first_corner())
            || last_corner().equals(&other_trace->last_corner());
        if (!(contact_at_first_corner || contact_at_last_corner) ||
            (contact_at_first_corner && contact_at_last_corner))
        {
            // no contact point or more than 1 contact point
            return nullptr;
        }
        if (contact_at_first_corner)
        {
            return &first_corner();
        }
        return &last_corner();
    }
    if (const DrillItem* other_drill = dynamic_cast<const DrillItem*>(p_other))
    {
        // Mirrors Java DrillItem.normal_contact_point(Trace): the drill center
        // is the unique contact point iff it equals a trace corner on a
        // shared layer.
        if (!shares_layer(p_other))
        {
            return nullptr;
        }
        const geometry::planar::IntPoint& center = other_drill->get_center();
        if (first_corner().equals(&center) || last_corner().equals(&center))
        {
            return &center;
        }
        return nullptr;
    }
    // ConductionArea and other items have no unique contact point with a trace.
    return nullptr;
}

bool Trace::is_tail() const
{
    return get_start_contacts().empty() || get_end_contacts().empty();
}

bool Trace::is_overlap() const
{
    const ItemSet start_contacts = get_start_contacts();
    const ItemSet end_contacts = get_end_contacts();
    for (const Item* curr_start : start_contacts)
    {
        if (end_contacts.find(const_cast<Item*>(curr_start)) != end_contacts.end())
        {
            return true;
        }
    }
    return false;
}

bool Trace::is_shove_fixed() const
{
    if (Item::is_shove_fixed())
    {
        return true;
    }
    // Check if the trace belongs to a net which is not shovable.
    if (board_ == nullptr)
    {
        return false;
    }
    for (const int curr_net_no : net_nos_)
    {
        if (rules::Nets::is_normal_net_no(curr_net_no))
        {
            const rules::Net* curr_net = board_->get_rules().get_nets().get(curr_net_no);
            if (curr_net != nullptr && curr_net->get_net_class() != nullptr &&
                curr_net->get_net_class()->is_shove_fixed())
            {
                return true;
            }
        }
    }
    return false;
}

bool Trace::is_drillable(int p_net_no) const
{
    return contains_net(p_net_no);
}

std::vector<std::shared_ptr<geometry::planar::Point>> Trace::get_ratsnest_corners() const
{
    // Use only uncontacted endpoints of the trace.
    std::vector<std::shared_ptr<geometry::planar::Point>> result;
    if (get_start_contacts().empty())
    {
        result.push_back(create_point_copy(first_corner()));
    }
    if (get_end_contacts().empty())
    {
        result.push_back(create_point_copy(last_corner()));
    }
    return result;
}

bool Trace::validate()
{
    bool result = Item::validate();
    if (first_corner().equals(&last_corner()))
    {
        logger::FRLogger::instance().warn("Trace.validate: first and last corner are equal");
        result = false;
    }
    return result;
}

const geometry::planar::Point& Trace::nearest_end_point(const geometry::planar::Point& p_from_point) const
{
    const geometry::planar::Point& p1 = first_corner();
    const geometry::planar::Point& p2 = last_corner();
    const geometry::planar::FloatPoint from_point = p_from_point.to_float();
    const double d1 = from_point.distance(p1.to_float());
    const double d2 = from_point.distance(p2.to_float());
    if (d1 < d2)
    {
        return p1;
    }
    return p2;
}

bool Trace::is_cycle() const
{
    if (is_overlap())
    {
        return true;
    }
    ItemSet start_contacts = get_start_contacts();
    ItemSet visited_items = start_contacts;
    bool ignore_areas = false;
    if (!net_nos_.empty() && board_ != nullptr)
    {
        const rules::Net* curr_net = board_->get_rules().get_nets().get(net_nos_[0]);
        if (curr_net != nullptr && curr_net->get_net_class() != nullptr)
        {
            ignore_areas = curr_net->get_net_class()->is_ignore_cycles_with_areas();
        }
    }
    for (Item* curr_contact : start_contacts)
    {
        if (curr_contact->is_cycle_recu(visited_items, this, this, ignore_areas))
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<geometry::planar::Point> Trace::create_point_copy(const geometry::planar::Point& p_point)
{
    if (const auto* int_point = dynamic_cast<const geometry::planar::IntPoint*>(&p_point))
    {
        return std::make_shared<geometry::planar::IntPoint>(*int_point);
    }
    return std::make_shared<geometry::planar::FloatPoint>(p_point.to_float());
}

bool Trace::normalize(const geometry::planar::IntOctagon* p_opt_area)
{
    (void)p_opt_area;
    // Stub: full implementation would clean up trace geometry
    return false;
}

bool Trace::combine()
{
    return false;
}

std::set<Pin*> Trace::touching_pins_at_end_corners() const
{
    std::set<Pin*> result;
    if (board_ == nullptr)
    {
        return result;
    }
    const geometry::planar::Point* curr_end_point = &first_corner();
    for (int i = 0; i < 2; ++i)
    {
        geometry::planar::IntOctagon curr_oct = curr_end_point->surrounding_octagon();
        curr_oct = curr_oct.enlarge(half_width_);
        std::vector<int> empty_nets;
        std::set<Item*, TreeStorableLess> overlaps = board_->overlapping_items_with_clearance(
            curr_oct, layer_, empty_nets, clearance_class_);
        for (Item* curr_item : overlaps)
        {
            if (auto* pin = dynamic_cast<Pin*>(curr_item))
            {
                if (curr_item->shares_net(this))
                {
                    result.insert(pin);
                }
            }
        }
        curr_end_point = &last_corner();
    }
    return result;
}

std::vector<Trace*> Trace::split(const geometry::planar::Point& p_point)
{
    (void)p_point;
    return {};
}

std::vector<PolylineTrace*> Trace::split(const geometry::planar::IntOctagon* p_clip_shape)
{
    (void)p_clip_shape;
    if (auto* poly_trace = dynamic_cast<PolylineTrace*>(this))
    {
        return {poly_trace};
    }
    return {};
}

} // namespace board
} // namespace freerouting