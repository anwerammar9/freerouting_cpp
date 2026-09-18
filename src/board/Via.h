#pragma once

#include "board/Item.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {

class Padstack;

class DrillItem : public Item
{
public:
    const geometry::planar::IntPoint& get_center() const { return center_; }
    void set_center(const geometry::planar::IntPoint& p_center) { center_ = p_center; }

    int get_layer() const override { return from_layer_; }
    int first_layer() const override { return from_layer_; }
    int last_layer() const override { return to_layer_; }
    int get_from_layer() const { return from_layer_; }
    int get_to_layer() const { return to_layer_; }

    // A drill item (pin/via) is a single convex shape on the tree (the pad
    // octagon). Without this the item is skipped by ShapeSearchTree::insert
    // and can never block routing.
    int tile_shape_count() const override { return 1; }

    Padstack* get_padstack() const { return padstack_; }
    void set_padstack(Padstack* p_padstack);

    /**
     * Returns the minimum width over all padstack shapes.
     * Mirrors Java DrillItem.min_width().
     */
    double min_width() const;

    /**
     * Returns all items of a net, which are connected to this drill item via
     * normal contacts. Mirrors Java DrillItem.get_normal_contacts().
     */
    std::vector<Item*> get_normal_contacts() const override;

    /**
     * Returns the uncontacted center point used for ratsnest display.
     * Mirrors Java DrillItem.get_ratsnest_corners().
     */
    std::vector<std::shared_ptr<geometry::planar::Point>> get_ratsnest_corners() const override;

    /**
     * Returns the unique contact point between this drill item and p_other,
     * or nullptr if there is none. Mirrors Java DrillItem.normal_contact_point.
     */
    const geometry::planar::Point* normal_contact_point(const Item* p_other) const override;

protected:
    DrillItem(const geometry::planar::IntPoint& p_center,
              const std::vector<int>& p_net_nos,
              int p_clearance_class,
              int p_id_no,
              int p_component_no,
              FixedState p_fixed_state,
              BasicBoard* p_board);

    geometry::planar::IntPoint center_;
    int from_layer_;
    int to_layer_;
    Padstack* padstack_;
};

class Via : public DrillItem
{
public:
    Via(Padstack* p_padstack,
        const geometry::planar::IntPoint& p_center,
        const std::vector<int>& p_net_nos,
        int p_clearance_class,
        int p_id_no,
        int p_component_no,
        FixedState p_fixed_state,
        bool p_attach_allowed,
        BasicBoard* p_board);

    bool is_via() const override { return true; }
    bool is_attach_allowed() const { return attach_allowed_; }

    Item* copy(int p_id_no) const;

    std::string to_string() const override;

    // Get tree shape for a specific layer (used by shove algorithms)
    const geometry::planar::TileShape* get_tree_shape_on_layer(ShapeSearchTree* p_search_tree, int p_layer);

    // Get tree shape for a specific layer (const version)
    const geometry::planar::TileShape* get_tree_shape_on_layer(const ShapeSearchTree* p_search_tree, int p_layer) const;

private:
    bool attach_allowed_;
};

} // namespace freerouting::board