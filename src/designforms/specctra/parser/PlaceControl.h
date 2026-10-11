#pragma once

#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads the (place_control ...) scope (flip style). Port of Java parser.PlaceControl.
 */
class PlaceControl : public ScopeKeyword
{
public:
    PlaceControl() : ScopeKeyword("place_control") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Reads "(flip_style rotate_first)" after the keyword. Java PlaceControl.readFlipStyleRotateFirst. */
    static bool read_flip_style_rotate_first(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
