#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "board/AngleRestriction.h"
#include "board/FixedState.h"
#include "designforms/specctra/parser/NetList.h"
#include "geometry/planar/PolylineShape.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/Rule.h"
#include "rules/BoardRules.h"
#include "rules/ClearanceMatrix.h"
#include "designforms/specctra/parser/ScopeKeyword.h"
#include "designforms/specctra/parser/Shape.h"

namespace freerouting::board {
class RoutingBoard;
} // namespace freerouting::board

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
    /** Reads a (via ...) padstack name list. Java Structure.readViaPadstacks; nullopt on error. */
    static std::optional<std::vector<std::string>> read_via_padstacks(IJFlexScanner& p_scanner);
    /** Reads a (snap_angle ...) scope. Java Structure.readSnapAngle; nullopt on error. */
    static std::optional<board::AngleRestriction> read_snap_angle(IJFlexScanner& p_scanner);
    /** Reads a (control ...) scope. Java Structure.readControlScope. */
    static bool read_control_scope(ReadScopeParameter& p_par);

    /**
     * Updates the board rules from the rules read from the structure scope: clearance rules on all
     * layers, default trace widths, layer dependent rules, and the pin-edge-to-turn distance.
     * Java Structure.updateBoardRules.
     */
    static void update_board_rules(
        const ReadScopeParameter& p_par, const BoardConstructionInfo& p_info, rules::BoardRules& p_board_rules);
    /**
     * Converts a dsn clearance rule into a board clearance rule. p_layer < 0 sets it on all layers.
     * Returns true, if "smd_to_turn_gap" was found. Java Structure.setClearanceRule.
     */
    static bool set_clearance_rule(const ClearanceRule& p_rule, int p_layer, const CoordinateTransform& p_transform,
        rules::BoardRules& p_board_rules, const std::string& p_string_quote);
    /** Java Structure.containsWireClearancePair. */
    static bool contains_wire_clearance_pair(const std::vector<std::string>& p_clearance_pairs);
    /** Appends the clearance classes via, smd, pin and area. Java Structure.createDefaultClearanceClasses. */
    static void create_default_clearance_classes(rules::BoardRules& p_board_rules);
    /**
     * Appends a clearance class and makes it the default class of its item class in the default net
     * class. Returns its index. Java Structure.appendClearanceClass.
     */
    static int append_clearance_class(rules::BoardRules& p_board_rules, const std::string& p_name);
    /** True, if all clearance values on both layers are equal. Java Structure.clearanceEquals. */
    static bool clearance_equals(const rules::ClearanceMatrix& p_matrix, int p_layer_1, int p_layer_2);

    /** Java Structure.KeepoutType. */
    enum class KeepoutType
    {
        keepout,
        via_keepout,
        place_keepout
    };

    /**
     * Creates the board from the structure data through p_par.board_handling: layers, coordinate
     * transform, outline (holes become obstacles), rules and communication. Java Structure.createBoard.
     */
    static bool create_board(ReadScopeParameter& p_par, BoardConstructionInfo& p_info);
    /**
     * Removes the outline shapes that are holes of another outline shape and returns them.
     * Java Structure.separateHoles.
     */
    static std::vector<std::shared_ptr<geometry::planar::PolylineShape>> separate_holes(
        std::vector<std::shared_ptr<geometry::planar::PolylineShape>>& p_outline_shapes);
    /** Inserts a keepout area of the structure scope. Java Structure.insertKeepout (area overload). */
    static bool insert_keepout(const Shape::ReadAreaScopeResult& p_area, ReadScopeParameter& p_par,
        KeepoutType p_keepout_type, board::FixedState p_fixed_state);
    /**
     * Inserts a conduction area over the board for every power layer with a net but without a
     * conduction area. Java Structure.insertMissingPowerPlanes.
     */
    static void insert_missing_power_planes(
        const std::vector<std::shared_ptr<Layer>>& p_layer_info, NetList& p_netlist, board::RoutingBoard& p_board);
};

} // namespace freerouting::designforms::specctra::parser