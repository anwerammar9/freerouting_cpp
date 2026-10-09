#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "designforms/specctra/parser/Rule.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/Shape.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads the (structure ...) scope of a dsn file. Port of the read part of Java
 * parser.Structure. T107 covers the layer, boundary and plane sections; the
 * keepouts, via list, snap angle and control scopes follow (T108), and the
 * board creation / insertion follows (T109).
 */
class Structure : public ScopeKeyword
{
public:
    Structure() : ScopeKeyword("structure") {}

    /** A (layer_rule <layer> <rules>) rule. Java Structure.LayerRule. */
    struct LayerRule
    {
        std::string layer_name;
        std::vector<std::shared_ptr<Rule>> rules;
    };

    /** Data collected while reading the structure scope. Java Structure.BoardConstructionInfo. */
    struct BoardConstructionInfo
    {
        std::vector<std::shared_ptr<Layer>> layer_info;
        std::shared_ptr<Shape> bounding_shape;
        std::vector<std::shared_ptr<Shape>> outline_shapes;
        std::string outline_clearance_class_name;
        int found_layer_count = 0;
        std::vector<std::shared_ptr<Rule>> default_rules;
        std::vector<LayerRule> layer_dependent_rules;
    };

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Reads a (layer ...) scope. Java Structure.readLayerScope. */
    static bool read_layer_scope(
        IJFlexScanner& p_scanner, BoardConstructionInfo& p_info, const std::string& p_string_quote);
    /** Reads a (boundary ...) scope. Java Structure.readBoundaryScope. */
    static bool read_boundary_scope(IJFlexScanner& p_scanner, BoardConstructionInfo& p_info);
    /** Adds a boundary shape to the construction info. Java Structure.addBoundaryShape. */
    static void add_boundary_shape(BoardConstructionInfo& p_info, std::unique_ptr<Shape> p_shape);
};

} // namespace freerouting::designforms::specctra::parser