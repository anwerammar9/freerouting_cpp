#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace freerouting::datastructures {

/**
 * Describes legal identifiers and the quote used for reserved names.
 */
class IdentifierType
{
public:
    IdentifierType(std::vector<std::string> p_reserved_chars, std::string p_string_quote);

    /**
     * Writes an identifier, quoting it when required by the configured rules.
     */
    void write(std::string p_name, std::ostream& p_file) const;

private:
    std::vector<std::string> reserved_chars_;
    std::string string_quote_;
};

} // namespace freerouting::datastructures
