#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {

/**
 * Reads a (plane <net_name> <area>) scope from a dsn file. Port of the read
 * part of Java parser.Plane; the write scope follows with the DSN writer.
 */
class Plane : public ScopeKeyword
{
public:
    Plane() : ScopeKeyword("plane") {}

    bool read_scope(ReadScopeParameter& p_par) const override;
};

} // namespace freerouting::designforms::specctra::parser
