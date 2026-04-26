#include "board/BasicBoard.h"
#include "rules/BoardRules.h"

namespace freerouting::board {

BasicBoard::BasicBoard(const LayerStructure& p_layer_structure,
                       std::unique_ptr<rules::BoardRules> p_rules)
    : layer_structure_(p_layer_structure)
    , rules_(std::move(p_rules))
{
}

BasicBoard::~BasicBoard() = default;

} // namespace freerouting::board