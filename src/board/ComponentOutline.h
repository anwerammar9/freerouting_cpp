#pragma once

#include <memory>
#include <string>

#include "board/Item.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting::geometry::planar
{
class Area;
class FloatPoint;
}

namespace freerouting::board
{

class ItemSelectionFilter;

/**
 * An item representing the outline of a component on the board. It is placed
 * on the first or the last layer depending on the side of the component.
 * Mirrors Java ComponentOutline.java.
 */
class ComponentOutline : public Item
{
public:
    ComponentOutline(const std::shared_ptr<geometry::planar::Area>& p_area,
                     bool p_is_front,
                     const geometry::planar::Vector& p_translation,
                     double p_rotation_in_degree,
                     int p_id_no,
                     int p_component_no,
                     FixedState p_fixed_state,
                     BasicBoard* p_board);

    std::unique_ptr<Item> copy(int p_id_no) const;

    std::shared_ptr<geometry::planar::Area> get_area() const;
    std::shared_ptr<geometry::planar::Area> get_relative_area() const;

    int get_layer() const override;
    int first_layer() const override;
    int last_layer() const override;
    bool is_on_layer(int p_layer) const;

    geometry::planar::IntBox bounding_box() const;

    bool is_obstacle(const Item* p_other) const override;

    int shape_layer(int p_index) const override;
    int tile_shape_count() const override;

    void translate_by(const geometry::planar::Vector& p_vector);
    void turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole);
    void rotate_approx(double p_angle_in_degree, const geometry::planar::FloatPoint& p_pole);
    void change_placement_side(const geometry::planar::IntPoint& p_pole);

    bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const override;

    /** Invalidates the precalculated absolute area. */
    void clear_derived_data();

    bool is_front() const { return is_front_; }
    double get_rotation_in_degree() const { return rotation_in_degree_; }

    std::string to_string() const override;

private:
    std::shared_ptr<geometry::planar::Area> relative_area_;
    mutable std::shared_ptr<geometry::planar::Area> precalculated_absolute_area_;
    std::shared_ptr<geometry::planar::Vector> translation_;
    double rotation_in_degree_;
    bool is_front_;
};

} // namespace freerouting::board