#include "board/ViaObstacleArea.h"

#include "board/ItemSelectionFilter.h"
#include "geometry/planar/Shape.h"

namespace freerouting::board {

ViaObstacleArea::ViaObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
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
                                 BasicBoard* p_board)
    : ObstacleArea(p_area, p_layer, p_translation, p_rotation_in_degree, p_side_changed, p_net_no_arr,
                   p_clearance_class, p_id_no, p_component_no, p_name, p_fixed_state, p_board)
{
}

ViaObstacleArea::ViaObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
                                 int p_layer,
                                 const geometry::planar::Vector& p_translation,
                                 double p_rotation_in_degree,
                                 bool p_side_changed,
                                 int p_clearance_class,
                                 int p_id_no,
                                 int p_component_no,
                                 const std::string& p_name,
                                 FixedState p_fixed_state,
                                 BasicBoard* p_board)
    : ViaObstacleArea(p_area, p_layer, p_translation, p_rotation_in_degree, p_side_changed, {}, p_clearance_class,
                      p_id_no, p_component_no, p_name, p_fixed_state, p_board)
{
}

std::unique_ptr<Item> ViaObstacleArea::copy(int p_id_no) const
{
    return std::make_unique<ViaObstacleArea>(get_relative_area(), get_layer(), *get_translation(),
                                             get_rotation_in_degree(), get_side_changed(), net_nos_, clearance_class_,
                                             p_id_no, component_no_, get_name(), fixed_state_, board_);
}

bool ViaObstacleArea::is_obstacle(const Item* p_other) const
{
    if (p_other == nullptr)
    {
        return false;
    }
    if (p_other->shares_net(this))
    {
        return false;
    }
    return p_other->is_via();
}

bool ViaObstacleArea::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    if (!is_selected_by_fixed_filter(p_filter))
    {
        return false;
    }
    return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::VIA_KEEPOUT);
}

} // namespace freerouting::board