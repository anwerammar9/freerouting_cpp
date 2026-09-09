#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/FixedState.h"
#include "board/ObstacleArea.h"

namespace freerouting::geometry::planar {
class Area;
class Point;
class TileShape;
class Vector;
}

namespace freerouting::datastructures {
class ShapeTree;
}

namespace freerouting::board {

class ItemSelectionFilter;

/**
 * A ObstacleArea, which can be electrically connected to other items.
 * Mirrors Java ConductionArea.java.
 */
class ConductionArea : public ObstacleArea
{
public:
    ConductionArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                   int p_layer,
                   const geometry::planar::Vector& p_translation,
                   double p_rotation_in_degree,
                   bool p_side_changed,
                   const std::vector<int>& p_net_no_arr,
                   int p_clearance_class,
                   int p_id_no,
                   int p_component_no,
                   const std::string& p_name,
                   bool p_is_obstacle,
                   FixedState p_fixed_state,
                   BasicBoard* p_board);

    std::unique_ptr<Item> copy(int p_id_no) const;

    std::vector<Item*> get_normal_contacts() const override;
    std::vector<std::shared_ptr<geometry::planar::Point>> get_ratsnest_corners() const override;
    std::shared_ptr<geometry::planar::TileShape> get_trace_connection_shape(
        const datastructures::ShapeTree& p_search_tree, int p_index) const;

    bool is_obstacle(const Item* p_other) const override;
    bool get_is_obstacle() const { return is_obstacle_; }
    void set_is_obstacle(bool p_value) { is_obstacle_ = p_value; }
    bool is_trace_obstacle(int p_net_no) const override;
    bool is_drillable(int p_net_no) const override;
    bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const override;

private:
    bool is_obstacle_;
};

} // namespace freerouting::board