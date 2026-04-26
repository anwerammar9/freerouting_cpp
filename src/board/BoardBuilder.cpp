#include "board/BoardBuilder.h"
#include "board/BasicBoard.h"
#include "board/RoutingBoard.h"
#include "board/Layer.h"
#include "board/Pin.h"
#include "rules/BoardRules.h"
#include "rules/NetClass.h"
#include "rules/ClearanceMatrix.h"
#include "logger/FRLogger.h"

namespace freerouting::board {

LayerStructure BoardBuilder::create_layer_structure(const std::vector<designforms::specctra::LayerDef>& p_layers)
{
    std::vector<Layer> layers;
    layers.reserve(p_layers.size());
    
    for (const auto& layer_def : p_layers)
    {
        layers.emplace_back(layer_def.name, layer_def.is_signal);
    }
    
    return LayerStructure(layers);
}

std::unique_ptr<rules::BoardRules> BoardBuilder::create_rules(const LayerStructure& p_layer_structure)
{
    int default_clearance = 200;
    auto rules = std::make_unique<rules::BoardRules>(p_layer_structure, default_clearance);
    return rules;
}

int BoardBuilder::get_net_no_for_pin(const designforms::specctra::Result& p_result, 
                                const std::string& p_component, 
                                const std::string& p_pin_name)
{
    for (const auto& net : p_result.nets)
    {
        for (const auto& pin_ref : net.pins)
        {
            if (pin_ref.first == p_component && pin_ref.second == p_pin_name)
            {
                return net.id;
            }
        }
    }
    return 0;
}

std::unique_ptr<RoutingBoard> BoardBuilder::build(const designforms::specctra::Result& p_result)
{
    if (!p_result.success)
    {
        return nullptr;
    }
    
    LayerStructure layer_structure = create_layer_structure(p_result.layers);
    auto rules = create_rules(layer_structure);
    
    geometry::planar::IntBox bounding_box(0, 0, p_result.board_width, p_result.board_height);
    auto board = std::make_unique<RoutingBoard>(bounding_box, layer_structure, std::move(rules), nullptr);
    
    if (board)
    {
        board->set_parsed_result(p_result);
        board->create_pins_from_parsed();
    }
    
    return board;
}

} // namespace freerouting::board