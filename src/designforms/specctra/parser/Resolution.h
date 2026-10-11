#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::datastructures {
class IndentFileWriter;
}

namespace freerouting::board {
class Communication;
}

namespace freerouting::designforms::specctra::parser {

/** Reads the (resolution <unit> <n>) scope. Port of Java parser.Resolution. */
class Resolution : public ScopeKeyword
{
public:
    Resolution() : ScopeKeyword("resolution") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Writes the (resolution ...) scope. Java Resolution.writeScope. */
    static void write_scope(datastructures::IndentFileWriter& p_file, const board::Communication& p_board_communication);
};

} // namespace freerouting::designforms::specctra::parser
