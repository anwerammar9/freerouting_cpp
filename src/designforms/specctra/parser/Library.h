#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

#include <vector>

namespace freerouting::board {
class Padstacks;
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::designforms::specctra::parser {

class CoordinateTransform;
class IJFlexScanner;
class LayerStructure;
class Package;

/**
 * Reads the (library ...) scope of a dsn file: padstacks and images. Port of the read part of
 * Java parser.Library.
 */
class Library : public ScopeKeyword
{
public:
    Library() : ScopeKeyword("library") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /**
     * Reads a (padstack <name> (shape ...) (attach on|off) (absolute on|off)) scope into
     * p_board_padstacks. Returns false on a syntax error. Java Library.readPadstackScope.
     */
    static bool read_padstack_scope(IJFlexScanner& p_scanner, const LayerStructure& p_layer_structure,
        const CoordinateTransform& p_transform, board::Padstacks& p_board_padstacks);

    /**
     * Creates the board packages from the images read: pins (padstack by cleaned name, relative
     * location, rotation), outlines with widths and closed flags, named keepouts, and the "::<n>"
     * de-duplication of images with equal names but different pins. Java Library.readScope 309-450.
     */
    static void create_board_packages(std::vector<Package>& p_package_list, ReadScopeParameter& p_par,
        board::RoutingBoard& p_board);
};

} // namespace freerouting::designforms::specctra::parser
