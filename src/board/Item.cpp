#include "board/Item.h"
#include "board/ConductionArea.h"
#include "board/ItemSearchTreesInfo.h"
#include "board/ItemSelectionFilter.h"
#include "board/Pin.h"
#include "board/PolylineTrace.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/BasicBoard.h"
#include "geometry/planar/IntOctagon.h"
#include "rules/Net.h"
#include <algorithm>

namespace freerouting::board {

namespace {

/** Mirrors Java Item.PROTECT_FANOUT_LENGTH. */
constexpr double kProtectFanoutLength = 400.0;

} // namespace

Item::~Item() = default;

Item::Item(const std::vector<int>& p_net_nos, int p_clearance_class, int p_id_no,
             int p_component_no, FixedState p_fixed_state, BasicBoard* p_board)
    : id_no_(p_id_no <= 0 && p_board != nullptr ? p_board->generate_item_id() : p_id_no),
      net_nos_(p_net_nos),
      clearance_class_(p_clearance_class),
      component_no_(p_component_no),
      fixed_state_(p_fixed_state),
      on_board_(false),
      board_(p_board),
      smallest_clearance_(0)
{
}

std::string Item::to_string() const
{
    return "Item[id=" + std::to_string(id_no_) + "]";
}

std::shared_ptr<autoroute::ItemAutorouteInfo> Item::get_autoroute_info()
{
    if (!autoroute_info_)
    {
        autoroute_info_ = std::make_shared<autoroute::ItemAutorouteInfo>(this);
    }
    return autoroute_info_;
}

std::vector<Item*> Item::get_normal_contacts() const
{
    if (!board_) return {};
    // Stub: actual implementation would query the board's search trees
    return {};
}

namespace {

/**
 * Returns true, if p_item implements the Java Connectable interface. In Java
 * the interface is implemented by Trace (and therefore PolylineTrace),
 * DrillItem (and therefore Pin and Via) and ConductionArea; the C++
 * Connectable class is a dead draft which no real item derives from, so the
 * check is reproduced with dynamic_casts.
 */
bool is_connectable(const Item* p_item)
{
    return p_item != nullptr &&
           (dynamic_cast<const Trace*>(p_item) != nullptr ||
            dynamic_cast<const DrillItem*>(p_item) != nullptr ||
            dynamic_cast<const ConductionArea*>(p_item) != nullptr);
}

} // namespace

std::set<Item*> Item::get_all_contacts() const
{
    // Port of Java Item.getAllContacts().
    std::set<Item*> result;
    if (board_ == nullptr || !is_connectable(this))
    {
        return result;
    }
    for (int i = 0; i < tile_shape_count(); i++)
    {
        std::set<SearchTreeObject*, TreeStorableLess> overlapping_objects =
            board_->overlapping_objects(get_tile_shape(i), shape_layer(i));
        for (SearchTreeObject* curr_object : overlapping_objects)
        {
            auto* curr_item = dynamic_cast<Item*>(curr_object);
            if (curr_item == nullptr)
            {
                continue;
            }
            if (curr_item != this && is_connectable(curr_item) && curr_item->shares_net(this))
            {
                result.insert(curr_item);
            }
        }
    }
    return result;
}

std::set<Item*> Item::get_all_contacts(int p_layer) const
{
    // Port of Java Item.getAllContacts(int).
    std::set<Item*> result;
    if (board_ == nullptr || !is_connectable(this))
    {
        return result;
    }
    for (int i = 0; i < tile_shape_count(); i++)
    {
        if (shape_layer(i) != p_layer)
        {
            continue;
        }
        std::set<SearchTreeObject*, TreeStorableLess> overlapping_objects =
            board_->overlapping_objects(get_tile_shape(i), p_layer);
        for (SearchTreeObject* curr_object : overlapping_objects)
        {
            auto* curr_item = dynamic_cast<Item*>(curr_object);
            if (curr_item == nullptr)
            {
                continue;
            }
            if (curr_item != this && is_connectable(curr_item) && curr_item->shares_net(this))
            {
                result.insert(curr_item);
            }
        }
    }
    return result;
}

std::set<Item*> Item::get_connected_set(int p_net_no, bool p_stop_at_plane) const
{
    std::set<Item*> result;
    if (p_net_no > 0 && !contains_net(p_net_no))
    {
        return result;
    }
    result.insert(const_cast<Item*>(this));
    get_connected_set_recu(result, p_net_no, p_stop_at_plane);
    return result;
}

std::set<Item*> Item::get_unconnected_set(int p_net_no) const
{
    // Port of Java Item.get_unconnected_set(int). Divergence: Java returns a
    // TreeSet ordered by Item.compare_to (descending item id), while the C++
    // get_connected_set() results are ordered by item pointer.
    std::set<Item*> result;
    if (p_net_no > 0 && !contains_net(p_net_no))
    {
        return result;
    }
    // Java BasicBoard.get_connectable_items() is a RoutingBoard method in C++.
    const auto* routing_board = dynamic_cast<const RoutingBoard*>(board_);
    if (routing_board == nullptr)
    {
        return result;
    }
    if (p_net_no > 0)
    {
        const std::vector<Item*> connectable_items = routing_board->get_connectable_items(p_net_no);
        result.insert(connectable_items.begin(), connectable_items.end());
    }
    else
    {
        for (int curr_net_no : net_nos_)
        {
            const std::vector<Item*> connectable_items = routing_board->get_connectable_items(curr_net_no);
            result.insert(connectable_items.begin(), connectable_items.end());
        }
    }
    const std::set<Item*> connected_items = get_connected_set(p_net_no);
    for (Item* curr_item : connected_items)
    {
        result.erase(curr_item);
    }
    return result;
}

void Item::get_connected_set_recu(std::set<Item*>& p_result, int p_net_no, bool p_stop_at_plane) const
{
    const std::vector<Item*> contact_list = get_normal_contacts();
    for (Item* curr_contact : contact_list)
    {
        if (p_stop_at_plane)
        {
            const auto* curr_area = dynamic_cast<const ConductionArea*>(curr_contact);
            if (curr_area != nullptr && curr_contact->get_component_no() <= 0)
            {
                continue;
            }
        }
        if (p_net_no > 0 && !curr_contact->contains_net(p_net_no))
        {
            continue;
        }
        if (p_result.insert(curr_contact).second)
        {
            curr_contact->get_connected_set_recu(p_result, p_net_no, p_stop_at_plane);
        }
    }
}

int Item::first_common_layer(const Item* p_other) const
{
    if (!p_other) return -1;
    const int max_first_layer = std::max(first_layer(), p_other->first_layer());
    const int min_last_layer = std::min(last_layer(), p_other->last_layer());
    if (max_first_layer > min_last_layer)
    {
        return -1;
    }
    return max_first_layer;
}

bool Item::shares_net(const Item* p_other) const
{
    if (!p_other) return false;
    return shares_net_no(p_other->net_nos_);
}

bool Item::shares_net_no(const std::vector<int>& p_net_no_arr) const
{
    for (int this_net : net_nos_)
    {
        for (int other_net : p_net_no_arr)
        {
            if (this_net == other_net) return true;
        }
    }
    return false;
}

bool Item::shares_layer(const Item* p_other) const
{
    if (!p_other) return false;
    const int max_first_layer = std::max(first_layer(), p_other->first_layer());
    const int min_last_layer = std::min(last_layer(), p_other->last_layer());
    return max_first_layer <= min_last_layer;
}

::freerouting::geometry::planar::IntOctagon Item::get_tile_shape(int p_index) const
{
    (void)p_index;
    // Return bounding octagon as default
    return bounding_box_.bounding_octagon();
}

::freerouting::geometry::planar::IntBox Item::surrounding_octagon() const
{
    return bounding_box_;
}

bool Item::nets_equal(const Item* p_other) const
{
    if (p_other == nullptr)
    {
        return false;
    }
    return nets_equal(p_other->net_nos_);
}

bool Item::nets_equal(const std::vector<int>& p_net_no_arr) const
{
    return net_nos_ == p_net_no_arr;
}

void Item::clear_derived_data()
{
    search_trees_info_.reset();
}

bool Item::nets_normal() const
{
    for (int net_no : net_nos_)
    {
        if (!rules::Nets::is_normal_net_no(net_no))
        {
            return false;
        }
    }
    return true;
}

bool Item::contains_net(int p_net_no) const
{
    if (p_net_no <= 0)
    {
        return false;
    }
    for (int net_no : net_nos_)
    {
        if (net_no == p_net_no)
        {
            return true;
        }
    }
    return false;
}

bool Item::is_obstacle(int p_net_no) const
{
    return !contains_net(p_net_no);
}

bool Item::is_trace_obstacle(int p_net_no) const
{
    return !contains_net(p_net_no);
}

bool Item::is_obstacle(const Item* p_other) const
{
    (void)p_other;
    return true;
}

bool Item::is_shove_fixed() const
{
    return static_cast<int>(fixed_state_) >= static_cast<int>(FixedState::SHOVE_FIXED);
}

bool Item::is_user_fixed() const
{
    return static_cast<int>(fixed_state_) >= static_cast<int>(FixedState::USER_FIXED);
}

bool Item::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    (void)p_filter;
    return true;
}

bool Item::is_selected_by_fixed_filter(const ItemSelectionFilter& p_filter) const
{
    if (is_user_fixed())
    {
        return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::FIXED);
    }
    return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::UNFIXED);
}

bool Item::validate()
{
    if (tile_shape_count() == 0)
    {
        return true;
    }
    bool result = true;
    for (int i = 0; i < tile_shape_count(); ++i)
    {
        auto shape = get_tile_shape(i);
        if (shape.is_empty())
        {
            result = false;
        }
    }
    return result;
}

bool Item::is_cycle_recu(std::set<Item*>& p_visited_items,
                         const Item* p_search_item,
                         const Item* p_come_from_item,
                         bool p_ignore_areas) const
{
    // Java skips conduction areas here when p_ignore_areas is true; the
    // ConductionArea class is not ported yet.
    (void)p_ignore_areas;
    const std::vector<Item*> contact_list = get_normal_contacts();
    for (Item* curr_contact : contact_list)
    {
        if (curr_contact == p_come_from_item)
        {
            continue;
        }
        if (curr_contact == p_search_item)
        {
            return true;
        }
        if (p_visited_items.insert(curr_contact).second)
        {
            if (curr_contact->is_cycle_recu(p_visited_items, p_search_item, this, p_ignore_areas))
            {
                return true;
            }
        }
    }
    return false;
}

int Item::tree_shape_count(const datastructures::ShapeTree& p_tree) const
{
    return static_cast<int>(ensure_tree_shapes(p_tree).size());
}

const geometry::planar::TileShape* Item::get_tree_shape(const datastructures::ShapeTree& p_tree, int p_index) const
{
    const std::vector<std::shared_ptr<geometry::planar::TileShape>>& shapes = ensure_tree_shapes(p_tree);
    if (p_index < 0 || static_cast<size_t>(p_index) >= shapes.size())
    {
        return nullptr;
    }
    return shapes[static_cast<size_t>(p_index)].get();
}

void Item::set_search_tree_entries(std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree)
{
    if (!search_trees_info_)
    {
        search_trees_info_ = std::make_unique<ItemSearchTreesInfo>();
    }
    search_trees_info_->set_tree_entries(std::move(p_entries), p_tree);
}

int Item::compare_to(const datastructures::ShapeTree::Storable& p_other) const
{
    const Item* other = dynamic_cast<const Item*>(&p_other);
    if (other == nullptr)
    {
        return 1;
    }
    return other->id_no_ - id_no_;
}

std::vector<datastructures::ShapeTree::Leaf*> Item::get_search_tree_entries(const ShapeSearchTree& p_tree) const
{
    if (!search_trees_info_)
    {
        return {};
    }
    return search_trees_info_->get_tree_entries(p_tree);
}

std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> Item::take_search_tree_entries(ShapeSearchTree& p_tree)
{
    if (!search_trees_info_)
    {
        return {};
    }
    return search_trees_info_->take_tree_entries(p_tree);
}

std::vector<std::shared_ptr<geometry::planar::TileShape>> Item::calculate_tree_shapes(const ShapeSearchTree& p_search_tree) const
{
    (void)p_search_tree;
    std::vector<std::shared_ptr<geometry::planar::TileShape>> result;
    int shape_count = tile_shape_count();
    result.reserve(static_cast<size_t>(shape_count));
    for (int i = 0; i < shape_count; ++i)
    {
        result.push_back(std::make_shared<geometry::planar::IntOctagon>(get_tile_shape(i)));
    }
    return result;
}

const std::vector<std::shared_ptr<geometry::planar::TileShape>>& Item::ensure_tree_shapes(const datastructures::ShapeTree& p_tree) const
{
    if (!search_trees_info_)
    {
        search_trees_info_ = std::make_unique<ItemSearchTreesInfo>();
    }
    const std::vector<std::shared_ptr<geometry::planar::TileShape>>* stored_shapes = search_trees_info_->get_precalculated_tree_shapes(p_tree);
    if (stored_shapes == nullptr)
    {
        const ShapeSearchTree& search_tree = static_cast<const ShapeSearchTree&>(p_tree);
        search_trees_info_->set_precalculated_tree_shapes(calculate_tree_shapes(search_tree), p_tree);
        stored_shapes = search_trees_info_->get_precalculated_tree_shapes(p_tree);
    }
    return *stored_shapes;
}

std::vector<Item*> Item::get_connection_items(StopConnectionOption p_stop_option) const
{
    const std::vector<Item*> contacts = get_normal_contacts();
    std::set<Item*> result;
    if (is_routable())
    {
        result.insert(const_cast<Item*>(this));
    }
    for (Item* current_item : contacts)
    {
        const geometry::planar::Point* prev_contact_point = normal_contact_point(current_item);
        if (prev_contact_point == nullptr)
        {
            // no unique contact point
            continue;
        }
        int prev_contact_layer = first_common_layer(current_item);
        if (const Trace* start_trace = dynamic_cast<const Trace*>(this))
        {
            // Check, that there is only 1 contact at this location.
            // Only for pins and vias items of more than 1 connection
            // are collected.
            const std::set<Item*> check_contacts = start_trace->get_normal_contacts(*prev_contact_point, false);
            if (check_contacts.size() != 1)
            {
                continue;
            }
        }
        // Search from current_item along the contacts
        // until the next fork or nonroute item.
        for (;;)
        {
            if (!current_item->is_routable())
            {
                // connection ends
                break;
            }
            if (dynamic_cast<const Via*>(current_item) != nullptr)
            {
                if (p_stop_option == StopConnectionOption::VIA)
                {
                    break;
                }
                if (p_stop_option == StopConnectionOption::FANOUT_VIA
                    && current_item->is_fanout_via(&result))
                {
                    break;
                }
            }
            result.insert(current_item);
            const std::vector<Item*> current_ob_contacts = current_item->get_normal_contacts();
            // Filter the contacts at the previous contact point, because we
            // were already there. If then there is not exactly 1 new contact
            // left, there is a stub or a fork.
            const geometry::planar::Point* next_contact_point = nullptr;
            int next_contact_layer = -1;
            Item* next_contact = nullptr;
            bool fork_found = false;
            for (Item* tmp_contact : current_ob_contacts)
            {
                const int tmp_contact_layer = current_item->first_common_layer(tmp_contact);
                if (tmp_contact_layer >= 0)
                {
                    const geometry::planar::Point* tmp_contact_point = current_item->normal_contact_point(tmp_contact);
                    if (tmp_contact_point == nullptr)
                    {
                        // no unique contact point
                        fork_found = true;
                        break;
                    }
                    if (prev_contact_layer != tmp_contact_layer
                        || !prev_contact_point->equals(tmp_contact_point))
                    {
                        if (next_contact != nullptr)
                        {
                            // second new contact found
                            fork_found = true;
                            break;
                        }
                        next_contact = tmp_contact;
                        next_contact_point = tmp_contact_point;
                        next_contact_layer = tmp_contact_layer;
                    }
                }
            }
            if (next_contact == nullptr || fork_found)
            {
                break;
            }
            current_item = next_contact;
            prev_contact_point = next_contact_point;
            prev_contact_layer = next_contact_layer;
        }
    }
    return std::vector<Item*>(result.begin(), result.end());
}

bool Item::is_fanout_via(const std::set<Item*>* p_ignore_items) const
{
    const std::vector<Item*> contact_list = get_normal_contacts();
    for (Item* current_contact : contact_list)
    {
        if (dynamic_cast<const Pin*>(current_contact) != nullptr
            && current_contact->first_layer() == current_contact->last_layer()
            && static_cast<int>(current_contact->get_normal_contacts().size()) <= 1)
        {
            return true;
        }
        if (const Trace* current_trace = dynamic_cast<const Trace*>(current_contact))
        {
            if (p_ignore_items != nullptr
                && p_ignore_items->find(current_contact) != p_ignore_items->end())
            {
                continue;
            }
            if (current_trace->get_length() >= kProtectFanoutLength * current_trace->get_half_width())
            {
                continue;
            }
            const std::vector<Item*> trace_contact_list = current_trace->get_normal_contacts();
            for (Item* tmp_contact : trace_contact_list)
            {
                if (dynamic_cast<const Pin*>(tmp_contact) != nullptr
                    && tmp_contact->first_layer() == tmp_contact->last_layer()
                    && static_cast<int>(tmp_contact->get_normal_contacts().size()) <= 1)
                {
                    return true;
                }
                if (const PolylineTrace* contact_trace = dynamic_cast<const PolylineTrace*>(tmp_contact))
                {
                    // look for shove fixed exit traces of SMD-pins
                    if (contact_trace->get_fixed_state() == FixedState::SHOVE_FIXED
                        && contact_trace->corner_count() == 2)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

} // namespace freerouting::board