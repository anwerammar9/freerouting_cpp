#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/IJFlexScanner.h"

namespace freerouting::designforms::specctra::parser {

/** A rule of a DSN rule scope. Port of the read part of Java parser.Rule. */
class Rule
{
public:
    virtual ~Rule() = default;

    /**
     * Reads the rules of a (rule ...) scope up to its closing bracket; the
     * "(rule" was read already. nullopt on error. Java Rule.readScope.
     */
    static std::optional<std::vector<std::shared_ptr<Rule>>> read_scope(IJFlexScanner& p_scanner);

    struct LayerRule
    {
        std::vector<std::string> layer_names;
        std::vector<std::shared_ptr<Rule>> rules;
    };
    /** Reads a (layer_rule <layers> (rule ...)) scope. Java Rule.readLayerRuleScope. */
    static std::optional<LayerRule> read_layer_rule_scope(IJFlexScanner& p_scanner);
};

/** (width <value>). Java Rule.WidthRule. */
class WidthRule : public Rule
{
public:
    explicit WidthRule(double p_value) : value(p_value) {}
    /** Java Rule.readWidthRule; nullptr if the closing bracket is missing. */
    static std::shared_ptr<WidthRule> read(IJFlexScanner& p_scanner);
    const double value;
};

/** (clearance <value> [(type <class-pairs>)]). Java Rule.ClearanceRule. */
class ClearanceRule : public Rule
{
public:
    ClearanceRule(double p_value, std::vector<std::string> p_class_pairs)
        : value(p_value), clearance_class_pairs(std::move(p_class_pairs)) {}
    /** Java Rule.readClearanceRule. */
    static std::shared_ptr<ClearanceRule> read(IJFlexScanner& p_scanner);
    const double value;
    /** Class pairs like "smd_smd" or "default-wide" (empty: the default clearance). */
    const std::vector<std::string> clearance_class_pairs;
};

} // namespace freerouting::designforms::specctra::parser
