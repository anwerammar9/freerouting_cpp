#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/ShapeSearchTree.h"
#include "rules/BoardRules.h"

namespace freerouting::geometry::planar {
class Point;
class Polyline;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class Item;
class ItemSelectionFilter;
class Pin;
class PolylineTrace;
class SearchTreeManager;
class Trace;

class BasicBoard
{
public:
    BasicBoard(const LayerStructure& p_layer_structure,
               std::unique_ptr<rules::BoardRules> p_rules);
    virtual ~BasicBoard();

    int get_layer_count() const { return layer_structure_.layer_count(); }
    const LayerStructure& get_layer_structure() const { return layer_structure_; }
    const rules::BoardRules& get_rules() const { return *rules_; }

    /**
     * Returns a new unique item id for items created on this board.
     * Mirrors the id_no_generator used by Java Item().
     */
    int generate_item_id() { return ++last_item_id_; }

    /**
     * Mirrors BoardComponents.get_flip_style_rotate_first() in Java: when true,
     * components (and their keepouts) are flipped by rotating around the
     * vertical axis first and then mirroring.
     */
    bool get_flip_style_rotate_first() const { return flip_style_rotate_first_; }
    void set_flip_style_rotate_first(bool p_value) { flip_style_rotate_first_ = p_value; }

    SearchTreeManager* get_search_tree_manager() { return search_tree_manager_.get(); }
    const SearchTreeManager* get_search_tree_manager() const { return search_tree_manager_.get(); }

    /**
     * Returns all SearchTreeObjects on layer p_layer, which overlap with
     * p_shape. If p_layer < 0, the layer is ignored.
     */
    std::set<SearchTreeObject*, TreeStorableLess> overlapping_objects(
        const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer) const;

    /**
     * Returns items, which overlap with p_shape on layer p_layer inclusive
     * clearance. p_clearance_class is the index in the clearance matrix, which
     * describes the required clearance restrictions to other items. The method
     * may also return items, which are nearly overlapping, but do not overlap
     * with exact calculation. If p_layer < 0, the layer is ignored.
     */
    std::set<Item*, TreeStorableLess> overlapping_items_with_clearance(const ::freerouting::geometry::planar::TileShape& p_shape,
        int p_layer, const std::vector<int>& p_ignore_net_nos, int p_clearance_class) const;

    /**
     * Returns the items on the board whose shape on layer p_layer contains
     * p_location. If p_layer < 0, the layer is ignored. If p_filter != nullptr,
     * only items of types selected by the filter are picked.
     * Mirrors Java BasicBoard.pick_items().
     */
    std::set<Item*> pick_items(const geometry::planar::Point& p_location, int p_layer,
                               const ItemSelectionFilter* p_filter) const;

    /**
     * Checks if the clearance-compensated-Object p_shape can be inserted on
     * layer p_layer without clearance violation. If p_contact_pins != nullptr,
     * pins not in the set are obstacles even of the own net (acid-trap guard);
     * listed pins are ignored. Mirrors Java BasicBoard.check_trace_shape().
     * The tie-pin exception for foreign traces inside multi-net pins needs Pin
     * tile shapes and is still TODO.
     */
    bool check_trace_shape(const geometry::planar::TileShape& p_shape,
                           int p_layer,
                           const std::vector<int>& p_net_no_arr,
                           int p_cl_class,
                           const std::set<Pin*>* p_contact_pins) const;

    /**
     * Inserts a trace into the board, whose geometry is described by a Polyline.
     * Because no internal cleaning of items is done, the new inserted item can
     * be returned. Mirrors Java BasicBoard.insert_trace_without_cleaning().
     */
    PolylineTrace* insert_trace_without_cleaning(
        const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
        int p_layer,
        int p_half_width,
        const std::vector<int>& p_net_no_arr,
        int p_clearance_class,
        FixedState p_fixed_state);

    /**
     * Looks for traces of net p_net_no on layer p_layer containing p_location
     * and splits them. Returns false if no trace was split.
     * Mirrors Java BasicBoard.split_traces().
     */
    bool split_traces(const geometry::planar::Point& p_location, int p_layer, int p_net_no);

    /**
     * Checks if p_trace is part of a cycle and removes it together with its
     * connection items in this case. Mirrors Java BasicBoard.remove_if_cycle().
     */
    bool remove_if_cycle(Trace* p_trace);



    std::string to_string() const { return "BasicBoard"; }

private:
    LayerStructure layer_structure_;
    std::unique_ptr<rules::BoardRules> rules_;
    std::unique_ptr<SearchTreeManager> search_tree_manager_;
    bool flip_style_rotate_first_ = false;
    int last_item_id_ = 0;
};

} // namespace freerouting::board