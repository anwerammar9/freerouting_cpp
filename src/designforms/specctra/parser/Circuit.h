#pragma once

#include <optional>
#include <string>
#include <vector>

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads a (circuit ...) scope of a dsn file and the rule / length / use-via /
 * use-layer data the net classes use. Port of Java io.specctra.parser.Circuit.
 */
class Circuit
{
public:
    /**
     * Result of a circuit scope. Java Circuit.ReadScopeResult. When the length
     * scope is absent or unreadable the lengths are 0 (Java parity: LengthMatchingRule(0, 0)).
     */
    struct ReadScopeResult
    {
        double max_length;
        double min_length;
        std::vector<std::string> use_via;
        std::vector<std::string> use_layer;
    };

    /** Reads a (circuit ...) scope; nullopt on scan errors. Java Circuit.readScope. */
    static std::optional<ReadScopeResult> read_scope(IJFlexScanner& p_scanner);

    /** Reads a (length <max> <min>) scope. Java Circuit.readLengthScope. */
    static std::optional<std::pair<double, double>> read_length_scope(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser