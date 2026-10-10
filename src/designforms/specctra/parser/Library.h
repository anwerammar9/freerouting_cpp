#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::board {
class Padstacks;
} // namespace freerouting::board

namespace freerouting::designforms::specctra::parser {

class CoordinateTransform;
class IJFlexScanner;
class LayerStructure;

/**
 * Reads the (library ...) scope of a dsn file: padstacks and images. Port of the read part of
 * Java parser.Library. The images (packages) follow with T111; until then they are skipped.
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
};

} // namespace freerouting::designforms::specctra::parser
