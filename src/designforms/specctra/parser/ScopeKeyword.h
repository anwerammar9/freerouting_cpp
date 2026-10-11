#pragma once

#include <string>

#include "designforms/specctra/parser/Keyword.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;
class ReadScopeParameter;

/** A keyword defining a scope of the DSN file. Port of Java ScopeKeyword. */
class ScopeKeyword : public Keyword
{
public:
    explicit ScopeKeyword(std::string p_name) : Keyword(std::move(p_name)) {}

    /**
     * Skips the current scope while reading a dsn file. Returns false, if no
     * legal scope was found. Java ScopeKeyword.skipScope.
     */
    static bool skip_scope(IJFlexScanner& p_scanner);

    /**
     * Reads the next scope of this keyword from the dsn file. The default
     * reads sub-scopes with a ScopeKeyword and skips all others. Java ScopeKeyword.readScope.
     */
    virtual bool read_scope(ReadScopeParameter& p_par) const;
};

} // namespace freerouting::designforms::specctra::parser
