#include "board/Item.h"
#include "board/ConductionArea.h"
#include "board/ItemSearchTreesInfo.h"
#include "board/ItemSelectionFilter.h"
#include "board/ShapeSearchTree.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/BasicBoard.h"
#include "geometry/planar/IntOctagon.h"
#include "rules/Net.h"
#include <algorithm>

namespace freerouting::board {

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
    // Stub: simplified for now
    return -1;
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
    (void)p_stop_option;
    return {};
}

} // namespace freerouting::board