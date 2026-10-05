#pragma once

#include "board/Item.h"
#include "geometry/planar/Point.h"

#include <memory>

namespace freerouting {
namespace board {

class Padstack;

class DrillItem : public Item
{
public:
    /**
     * Returns the center point of this drill item. The Java DrillItem.center
     * field is null until it is calculated or set, so a pin overrides this
     * method to calculate its center from the component placement.
     * Mirrors Java DrillItem.get_center().
     */
    virtual const geometry::planar::IntPoint& get_center() const { return center_; }
    void set_center(const geometry::planar::IntPoint& p_center)
    {
        center_ = p_center;
        center_is_set_ = true;
    }

    /**
     * Returns true, if the center of this drill item has already been
     * calculated or set. Java checks the nullable DrillItem.center field.
     */
    bool center_is_set() const { return center_is_set_; }

    int get_layer() const override { return from_layer_; }
    int first_layer() const override { return from_layer_; }
    int last_layer() const override { return to_layer_; }
    int get_from_layer() const { return from_layer_; }
    int get_to_layer() const { return to_layer_; }

    /** Mirrors Java DrillItem.isOnLayer(). */
    bool is_on_layer(int p_layer) const { return p_layer >= from_layer_ && p_layer <= to_layer_; }

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
     * Returns the tree shape of this drill item on p_layer, or nullptr, if
     * p_layer is outside of its layer range. Mirrors Java
     * DrillItem.getTreeShapeOnLayer(ShapeSearchTree, int).
     */
    const geometry::planar::TileShape* get_tree_shape_on_layer(
        const ShapeSearchTree* p_search_tree, int p_layer) const;

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

    mutable geometry::planar::IntPoint center_;
    /**
     * Mirrors the nullability of the Java DrillItem.center field: it is only
     * set for a drill item, whose center is known from the construction
     * (a via) or from the first get_center() call (a pin).
     */
    mutable bool center_is_set_ = true;
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

    /**
     * Returns true, if the Via has contacts on at most 1 layer.
     * Mirrors Java Via.isTail().
     */
    bool is_tail() const override;

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
} // namespace freerouting

