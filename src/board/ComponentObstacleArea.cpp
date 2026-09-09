#include "board/ComponentObstacleArea.h"

#include "board/ItemSelectionFilter.h"
#include "geometry/planar/Shape.h"

namespace freerouting::board {

ComponentObstacleArea::ComponentObstacleArea(const std::shared_ptr<geometry::planar::Area>& p_area,
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
    : ObstacleArea(p_area, p_layer, p_translation, p_rotation_in_degree, p_side_changed, {}, p_clearance_class,
                   p_id_no, p_component_no, p_name, p_fixed_state, p_board)
{
}

std::unique_ptr<Item> ComponentObstacleArea::copy(int p_id_no) const
{
    return std::make_unique<ComponentObstacleArea>(get_relative_area(), get_layer(), *get_translation(),
                                                   get_rotation_in_degree(), get_side_changed(), clearance_class_,
                                                   p_id_no, component_no_, get_name(), fixed_state_, board_);
}

bool ComponentObstacleArea::is_obstacle(const Item* p_other) const
{
    if (p_other == this || p_other == nullptr)
    {
        return false;
    }
    const auto* p_other_component_area = dynamic_cast<const ComponentObstacleArea*>(p_other);
    if (p_other_component_area == nullptr)
    {
        return false;
    }
    return p_other_component_area->component_no_ != component_no_;
}

bool ComponentObstacleArea::is_selected_by_filter(const ItemSelectionFilter& p_filter) const
{
    if (!is_selected_by_fixed_filter(p_filter))
    {
        return false;
    }
    return p_filter.is_selected(ItemSelectionFilter::SelectableChoices::COMPONENT_KEEPOUT);
}

} // namespace freerouting::board