#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads the (part_library ...) scope: logical part mappings and logical parts used for pin and gate
 * swap. Port of the read part of Java parser.PartLibrary.
 */
class PartLibrary : public ScopeKeyword
{
public:
    /** Java PartLibrary.LogicalPartMapping. */
    struct LogicalPartMapping
    {
        std::string name;
        std::set<std::string> components; // Java SortedSet<String>
    };

    /** Java PartLibrary.PartPin. */
    struct PartPin
    {
        std::string pin_name;
        std::string gate_name;
        int gate_swap_code = 0;
        std::string gate_pin_name;
        int gate_pin_swap_code = 0;
    };

    /** Java PartLibrary.LogicalPart. */
    struct LogicalPart
    {
        std::string name;
        std::vector<PartPin> part_pins;
    };

    PartLibrary() : ScopeKeyword("part_library") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    static std::optional<LogicalPartMapping> read_logical_part_mapping(IJFlexScanner& p_scanner);
    static std::optional<LogicalPart> read_logical_part(IJFlexScanner& p_scanner);
    static std::optional<PartPin> read_part_pin(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
