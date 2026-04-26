#pragma once

#include <memory>
#include <string>
#include <locale>

namespace freerouting {
namespace interactive {

class BoardManager
{
public:
    virtual ~BoardManager() = default;

    virtual std::locale get_locale() = 0;

    virtual std::shared_ptr<board::RoutingBoard> get_routing_board() = 0;

    virtual void initialize_manual_trace_half_widths() = 0;

    virtual void create_board(const geometry::planar::IntBox& p_bounding_box,
                              const board::LayerStructure& p_layer_structure,
                              const std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_outline_shapes,
                              const std::string& p_outline_clearance_class_name,
                              std::shared_ptr<rules::BoardRules> p_rules,
                              board::Communication* p_board_communication) = 0;

    virtual std::shared_ptr<class InteractiveSettings> get_settings() = 0;
};

} // namespace interactive
} // namespace freerouting