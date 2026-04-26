#pragma once

#include "designforms/specctra/Scanner.h"
#include "board/Layer.h"
#include "board/Component.h"
#include "rules/BoardRules.h"

#include <memory>
#include <string>
#include <vector>

namespace freerouting::board {

class BasicBoard;
class RoutingBoard;

class BoardBuilder
{
public:
    static std::unique_ptr<RoutingBoard> build(const designforms::specctra::Result& p_result);

    static int get_net_no_for_pin(const designforms::specctra::Result& p_result, 
                              const std::string& p_component, 
                              const std::string& p_pin_name);

private:
    static LayerStructure create_layer_structure(const std::vector<designforms::specctra::LayerDef>& p_layers);
    static std::unique_ptr<rules::BoardRules> create_rules(const LayerStructure& p_layer_structure);
};

} // namespace freerouting::board