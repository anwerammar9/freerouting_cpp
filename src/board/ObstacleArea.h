#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/FixedState.h"
#include "board/Item.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"

namespace freerouting::geometry::planar {
class Area;
class FloatPoint;
class TileShape;
}

namespace freerouting::board {

class ItemSelectionFilter;

/**
 * An item on the board with a relative_area shape, for example keepout or
 * conduction area. Mirrors Java ObstacleArea.java.
 */
class ObstacleArea : public Item
{
public:
    ObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                 int p_layer,
                 const geometry::planar::Vector& p_translation,
                 double p_rotation_in_degree,
                 bool p_side_changed,
                 const std::vector<int>& p_net_no_arr,
                 int p_clearance_class,
                 int p_id_no,
                 int p_component_no,
                 const std::string& p_name,
                 FixedState p_fixed_state,
                 BasicBoard* p_board);

    ObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                 int p_layer,
                 const geometry::planar::Vector& p_translation,
                 double p_rotation_in_degree,
                 bool p_side_changed,
                 int p_clearance_class,
                 int p_id_no,
                 int p_component_no,
                 const std::string& p_name,
                 FixedState p_fixed_state,
                 BasicBoard* p_board);

    std::unique_ptr<Item> copy(int p_id_no) const;

    const std::string& get_name() const { return name_; }

    std::shared_ptr<geometry::planar::Area> get_area() const;
    std::shared_ptr<geometry::planar::Area> get_relative_area() const;

    bool is_on_layer(int p_layer) const;
    int first_layer() const override;
    int last_layer() const override;
    int get_layer() const override;

    geometry::planar::IntBox bounding_box() const;

    bool is_obstacle(const Item* p_other) const override;

    int tile_shape_count() const override;
    geometry::planar::IntOctagon get_tile_shape(int p_index) const override;
    int shape_layer(int p_index) const override;

    void translate_by(const geometry::planar::Vector& p_vector);
    void turn_90_degree(int p_factor, const geometry::planar::IntPoint& p_pole);
    void rotate_approx(double p_angle_in_degree, const geometry::planar::FloatPoint& p_pole);
    void change_placement_side(const geometry::planar::IntPoint& p_pole);

    bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const override;

    /** Invalidates the precalculated absolute area. */
    void clear_derived_data();

    const std::shared_ptr<geometry::planar::Vector> get_translation() const { return translation_; }
    double get_rotation_in_degree() const { return rotation_in_degree_; }
    bool get_side_changed() const { return side_changed_; }

    std::string to_string() const override;

private:
    const std::vector<std::unique_ptr<geometry::planar::TileShape>>& get_split_to_convex() const;

    std::string name_;
    std::shared_ptr<geometry::planar::Area> relative_area_;
    int layer_;
    mutable std::shared_ptr<geometry::planar::Area> precalculated_absolute_area_;
    std::shared_ptr<geometry::planar::Vector> translation_;
    double rotation_in_degree_;
    bool side_changed_;
};

} // namespace freerouting::board