#pragma once

#include <set>
#include <vector>
#include <memory>
#include <string>

#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/BasicBoard.h"
#include "board/SearchTreeObject.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::geometry::planar {
class Polyline;
class Point;
class TileShape;
}

namespace freerouting::autoroute {
class ItemAutorouteInfo;
}

namespace freerouting::board {

class ItemSearchTreesInfo;
class ShapeSearchTree;
class ItemSelectionFilter;

class Item : public SearchTreeObject
{
public:
    virtual ~Item();

    int get_id() const { return id_no_; }

    const std::vector<int>& get_net_nos() const { return net_nos_; }

    int get_clearance_class() const { return clearance_class_; }
    void set_clearance_class(int p_class) { clearance_class_ = p_class; }

    FixedState get_fixed_state() const { return fixed_state_; }
    void set_fixed_state(FixedState p_state) { fixed_state_ = p_state; }

    int get_component_no() const { return component_no_; }
    void set_component_no(int p_no) { component_no_ = p_no; }

    ::freerouting::geometry::planar::IntBox get_bounding_box() const { return bounding_box_; }
    void set_bounding_box(const ::freerouting::geometry::planar::IntBox& p_box) { bounding_box_ = p_box; }

    bool is_on_board() const { return on_board_; }
    void set_on_board(bool p_value) { on_board_ = p_value; }

    BasicBoard* get_board() const { return board_; }
    void set_board(BasicBoard* p_board) { board_ = p_board; }

    virtual int get_layer() const { return -1; }
    virtual int first_layer() const { return get_layer(); }
    virtual int last_layer() const { return get_layer(); }
    virtual bool shares_layer(const Item* p_other) const;
    virtual bool is_routable() const { return true; }
    virtual bool is_trace() const { return false; }
    virtual bool is_via() const { return false; }
    virtual bool is_pin() const { return false; }

    virtual int tile_shape_count() const { return 0; }
    virtual ::freerouting::geometry::planar::IntOctagon get_tile_shape(int p_index) const;
    virtual int shape_layer(int p_index) const override { (void)p_index; return get_layer(); }

    virtual std::shared_ptr<autoroute::ItemAutorouteInfo> get_autoroute_info();
    virtual std::vector<Item*> get_normal_contacts() const;
    virtual int first_common_layer(const Item* p_other) const;
    virtual bool shares_net(const Item* p_other) const;
    virtual bool shares_net_no(const std::vector<int>& p_net_no_arr) const;

    virtual bool contains_net(int p_net_no) const;

    /** Returns true if all nets of this item are normal (non-subnet) nets. */
    bool nets_normal() const;

    bool is_obstacle(int p_net_no) const override;
    bool is_trace_obstacle(int p_net_no) const override;

    // ---- Item.java virtuals (mirrors Java interface) ----

    /** Returns true if this item is an obstacle for the other item. */
    virtual bool is_obstacle(const Item* p_other) const;

    /** Returns the unique contact point between this item and p_other, or nullptr. */
    virtual const ::freerouting::geometry::planar::Point* normal_contact_point(const Item* p_other) const { return nullptr; }

    /** Returns true if this item is not contacted at either endpoint (Trace/Via override). */
    virtual bool is_tail() const { return false; }

    /**
     * Returns the set of all Connectable items of the net with number p_net_no
     * which can be reached recursively via normal contacts from this item.
     * If p_net_no <= 0, the net number is ignored. If p_stop_at_plane, the
     * recursive algorithm stops, when a conduction area is reached, which does
     * not belong to a component.
     * Mirrors Java Item.get_connected_set(int, boolean).
     */
    std::set<Item*> get_connected_set(int p_net_no, bool p_stop_at_plane) const;

    /** Recursive part of get_connected_set. */
    void get_connected_set_recu(std::set<Item*>& p_result, int p_net_no, bool p_stop_at_plane) const;

    /** Returns true if this item has overlapping contacts at the same endpoint. */
    virtual bool is_overlap() const { return false; }

    /** Returns true if this item cannot be moved by the push/shove algorithm. */
    virtual bool is_shove_fixed() const;

    /** Returns true if this item can be drilled through at the given net number. */
    virtual bool is_drillable(int p_net_no) const { return false; }

    /** Returns uncontacted endpoints used for ratsnest display. */
    virtual std::vector<std::shared_ptr<::freerouting::geometry::planar::Point>> get_ratsnest_corners() const { return {}; }

    /** Returns true if this item is fixed by the user (fixed_state >= USER_FIXED). */
    virtual bool is_user_fixed() const;

    /** Returns true if this item contains exactly the same nets as p_other. */
    bool nets_equal(const Item* p_other) const;
    bool nets_equal(const std::vector<int>& p_net_no_arr) const;

    /**
     * Invalidates precalculated derived data (tree entries and shapes).
     * The caller must have removed the item from all search trees first.
     */
    virtual void clear_derived_data();

    /**
     * Returns true if this item is contained in the input filter.
     * Mirrors Item.java (abstract there); the base default returns true so
     * existing subclasses keep compiling until they override it.
     */
    virtual bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const;

    /** Stop connection options for get_connection_items. */
    enum class StopConnectionOption
    {
        NONE,
        VIA,
        FANOUT_VIA
    };

    /** Returns connection items until next fork or terminal item. */
    virtual std::vector<Item*> get_connection_items(StopConnectionOption p_stop_option) const;

    /** Test function checking the item for inconsistencies. */
    virtual bool validate();

    /** Recursive part of Trace.is_cycle. Returns true if p_search_item is reachable. */
    bool is_cycle_recu(std::set<Item*>& p_visited_items,
                       const Item* p_search_item,
                       const Item* p_come_from_item,
                       bool p_ignore_areas) const;

    // ================= ShapeTree::Storable =================
    int tree_shape_count(const datastructures::ShapeTree& p_tree) const override;
    const ::freerouting::geometry::planar::TileShape* get_tree_shape(const datastructures::ShapeTree& p_tree, int p_index) const override;
    void set_search_tree_entries(std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree) override;
    int compare_to(const datastructures::ShapeTree::Storable& p_other) const override;

    /**
     * Returns the tree entries of this item in p_tree as raw pointers.
     */
    std::vector<datastructures::ShapeTree::Leaf*> get_search_tree_entries(const ShapeSearchTree& p_tree) const;

    /**
     * Moves the tree entries of this item in p_tree out of this item.
     */
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> take_search_tree_entries(ShapeSearchTree& p_tree);

    virtual ::freerouting::geometry::planar::IntBox surrounding_octagon() const;
    virtual std::string to_string() const;

protected:
    Item(const std::vector<int>& p_net_nos, int p_clearance_class, int p_id_no,
         int p_component_no, FixedState p_fixed_state, BasicBoard* p_board);

    /**
     * Returns the tree shapes of this item for p_search_tree. Default
     * implementation uses the tile shapes of this item.
     */
    virtual std::vector<std::shared_ptr<::freerouting::geometry::planar::TileShape>> calculate_tree_shapes(const ShapeSearchTree& p_search_tree) const;

    /**
     * Internally used for implementing is_selected_by_filter.
     * Mirrors Item.java (protected there).
     */
    bool is_selected_by_fixed_filter(const ItemSelectionFilter& p_filter) const;

    int id_no_;
    std::vector<int> net_nos_;
    int clearance_class_;
    int component_no_;
    FixedState fixed_state_;
    ::freerouting::geometry::planar::IntBox bounding_box_;
    bool on_board_;
    BasicBoard* board_;
    double smallest_clearance_;
    std::shared_ptr<autoroute::ItemAutorouteInfo> autoroute_info_;

private:
    const std::vector<std::shared_ptr<::freerouting::geometry::planar::TileShape>>& ensure_tree_shapes(const datastructures::ShapeTree& p_tree) const;

    mutable std::unique_ptr<ItemSearchTreesInfo> search_trees_info_;
};

} // namespace freerouting::board