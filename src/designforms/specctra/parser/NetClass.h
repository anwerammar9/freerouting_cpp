#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Rule.h"

namespace freerouting::designforms::specctra::parser {

/**
 * Contains the information of a Specctra Class scope. Port of the read part of
 * Java parser.NetClass. Java fields are final except min/max trace length; the
 * C++ port uses plain public fields because the reader returns the value.
 */
class NetClass
{
public:
    /** A (class_class ...) scope. Java NetClass.ClassClass. */
    struct ClassClass
    {
        std::vector<std::string> class_names;
        std::vector<std::shared_ptr<Rule>> rules;
        std::vector<Rule::LayerRule> layer_rules;
    };

    std::string name;
    /** The (clearance_class ...) name, absent if not specified. Java traceClearanceClass. */
    std::optional<std::string> trace_clearance_class;
    std::vector<std::string> net_list;
    std::vector<std::shared_ptr<Rule>> rules;
    std::vector<Rule::LayerRule> layer_rules;
    std::vector<std::string> use_via;
    std::vector<std::string> use_layer;
    /** The (via_rule ...) name, absent if not specified. Java viaRule. */
    std::optional<std::string> via_rule;
    bool shove_fixed = false;
    bool pull_tight = true;
    double min_trace_length = 0;
    double max_trace_length = 0;

    /**
     * Reads a (class ...) scope; the "(class" was read already. nullopt on error.
     * Java NetClass.readScope.
     */
    static std::optional<NetClass> read_scope(IJFlexScanner& p_scanner);

    /**
     * Reads a (class_class ...) scope; the "(class_class" was read already. nullopt
     * on error. Java NetClass.readClassClassScope.
     */
    static std::optional<ClassClass> read_class_class_scope(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
