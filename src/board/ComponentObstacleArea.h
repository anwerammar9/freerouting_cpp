#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/FixedState.h"
#include "board/ObstacleArea.h"

namespace freerouting::geometry::planar {
class Area;
class Vector;
}

namespace freerouting::board {

class ItemSelectionFilter;

/**
 * Describes areas of the board, where components are not allowed.
 * Mirrors Java ComponentObstacleArea.java.
 */
class ComponentObstacleArea : public ObstacleArea
{
public:
    ComponentObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
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

    bool is_obstacle(const Item* p_other) const override;
    bool is_trace_obstacle(int p_net_no) const override { (void)p_net_no; return false; }
    bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const override;
};

} // namespace freerouting::board