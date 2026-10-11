#include "designforms/specctra/parser/MinimalBoardManager.h"

#include <algorithm>

#include "rules/DefaultItemClearanceClasses.h"
#include "rules/NetClass.h"

namespace freerouting::designforms::specctra::parser {

void MinimalBoardManager::create_board(const geometry::planar::IntBox& p_bounding_box,
    const board::LayerStructure& p_layer_structure,
    std::vector<std::shared_ptr<geometry::planar::PolylineShape>> p_outline_shapes,
    const std::string& p_outline_clearance_class_name, std::unique_ptr<rules::BoardRules> p_rules,
    std::unique_ptr<board::Communication> p_board_communication)
{
    int outline_clearance_no = 0;
    if (p_rules != nullptr)
    {
        if (!p_outline_clearance_class_name.empty())
        {
            outline_clearance_no = std::max(0, p_rules->get_clearance_matrix().get_no(p_outline_clearance_class_name));
        }
        else
        {
            outline_clearance_no =
                p_rules->get_default_net_class()->get_default_item_clearance_classes().get(rules::ItemClass::AREA);
        }
    }
    board_ = std::make_unique<board::RoutingBoard>(p_bounding_box, p_layer_structure, std::move(p_rules), nullptr);
    board_->adopt_communication(std::move(p_board_communication));
    // Java's RoutingBoard constructor inserts the outline; the C++ constructor does not, so it is
    // inserted right here (it must stay the first item: item ids follow insertion order).
    board_->insert_outline(std::move(p_outline_shapes), outline_clearance_no);
}

} // namespace freerouting::designforms::specctra::parser
