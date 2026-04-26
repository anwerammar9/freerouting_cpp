#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/Layer.h"
#include "rules/BoardRules.h"

namespace freerouting::board {

class BasicBoard
{
public:
    BasicBoard(const LayerStructure& p_layer_structure,
               std::unique_ptr<rules::BoardRules> p_rules);
    virtual ~BasicBoard();

    int get_layer_count() const { return layer_structure_.layer_count(); }
    const LayerStructure& get_layer_structure() const { return layer_structure_; }
    const rules::BoardRules& get_rules() const { return *rules_; }

    std::string to_string() const { return "BasicBoard"; }

private:
    LayerStructure layer_structure_;
    std::unique_ptr<rules::BoardRules> rules_;
};

} // namespace freerouting::board