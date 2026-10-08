#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {

/** Reads the (resolution <unit> <n>) scope. Port of Java parser.Resolution. */
class Resolution : public ScopeKeyword
{
public:
    Resolution() : ScopeKeyword("resolution") {}

    bool read_scope(ReadScopeParameter& p_par) const override;
};

} // namespace freerouting::designforms::specctra::parser
