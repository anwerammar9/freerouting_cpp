#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <optional>

#include "board/AngleRestriction.h"
#include "board/Unit.h"
#include "designforms/specctra/parser/CoordinateTransform.h"
#include "designforms/specctra/parser/IJFlexScanner.h"
#include "designforms/specctra/parser/Layer.h"
#include "designforms/specctra/parser/MinimalBoardManager.h"
#include "designforms/specctra/parser/NetList.h"
#include "designforms/specctra/parser/Shape.h"

namespace freerouting::settings {
class RouterSettings;
} // namespace freerouting::settings

namespace freerouting::designforms::specctra::parser {

/**
 * Structured properties and helper data shared by the scope readers while a
 * DSN file is parsed. Port of Java ReadScopeParameter; the fields are added
 * as the scope readers using them are ported.
 */
class ReadScopeParameter
{
public:
    /** A plane read from a (plane ...) scope. Java ReadScopeParameter.PlaneInfo. */
    struct PlaneInfo
    {
        Shape::ReadAreaScopeResult area;
        std::string net_name;
    };

    explicit ReadScopeParameter(IJFlexScanner& p_scanner)
        : scanner(p_scanner), board_handling(std::make_unique<MinimalBoardManager>())
    {
    }

    IJFlexScanner& scanner;

    /** Creates and holds the board. Java boardHandling (a MinimalBoardManager by default). */
    std::unique_ptr<BoardParserCallback> board_handling;

    /** The board created by the structure scope, or nullptr. Java getBoard. */
    board::RoutingBoard* get_board() const { return board_handling->get_routing_board(); }

    /** The nets of the network scope; always present (Java initialises it). Java netlist. */
    NetList netlist;

    /** Warnings collected during parsing. Java ReadScopeParameter.warnings. */
    std::vector<std::string> warnings;

    /** The quote character of the parser scope. Java stringQuote. */
    std::string string_quote = "\"";
    std::string host_cad;
    std::string host_version;
    /** (constant name value) entries of the parser scope. Java constants. */
    std::vector<std::array<std::string, 2>> constants;
    /** Java writeResolution (parser scope), absent = Java null. */
    std::optional<board::Communication::WriteResolution> write_resolution;
    bool dsn_file_generated_by_host = true;
    /** Set to false by the structure reader when the board outline is absent. */
    bool board_outline_ok = true;

    /** Set when the structure scope is read. Java coordinateTransform. */
    std::optional<CoordinateTransform> coordinate_transform;
    /** From the resolution scope. Java unit / resolution (defaults MIL, 100). */
    board::Unit unit = board::Unit::MIL;
    int resolution = 100;

    /** The layer structure read so far; created from the layer scopes. Java layerStructure. */
    std::shared_ptr<LayerStructure> layer_structure;
    /**
     * The via padstack names of the structure scope. Java viaPadstackNames; nullopt when the
     * (via ...) scope could not be read (Java null).
     */
    std::optional<std::vector<std::string>> via_padstack_names;
    /** The planes of the structure scope. Java planeList. */
    std::vector<PlaneInfo> plane_list;
    /** The keepouts of the (keepout ...) scopes. Java Structure.readScope keepoutList. */
    std::vector<Shape::ReadAreaScopeResult> keepout_list;
    /** The keepouts of the (via_keepout ...) scopes. Java Structure.readScope viaKeepoutList. */
    std::vector<Shape::ReadAreaScopeResult> via_keepout_list;
    /** The keepouts of the (place_keepout ...) scopes. Java Structure.readScope placeKeepoutList. */
    std::vector<Shape::ReadAreaScopeResult> place_keepout_list;
    /** Whether vias may be placed on smd pads. Java viaAtSmdAllowed (control scope). */
    bool via_at_smd_allowed = false;
    /** The snap angle of the structure scope. Java snapAngle, defaults to FORTYFIVE_DEGREE. */
    board::AngleRestriction snap_angle = board::AngleRestriction::FORTYFIVE_DEGREE;
    /**
     * The autoroute settings read from an (autoroute_settings ...) scope.
     * Java autorouteSettings; nullptr (null) when the scope is not present.
     */
    std::shared_ptr<settings::RouterSettings> autoroute_settings;
};

} // namespace freerouting::designforms::specctra::parser
