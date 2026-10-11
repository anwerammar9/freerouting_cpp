#pragma once

#include <optional>

#include "designforms/specctra/parser/ComponentPlacement.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads a (component <image> (place ...) ...) scope of the placement scope. Port of the read part of
 * Java parser.Component; the placements are inserted by Network.insertComponents (T115).
 */
class Component : public ScopeKeyword
{
public:
    Component() : ScopeKeyword("component") {}

    /** Appends the placement to p_par.placement_list. Java Component.readScope(ReadScopeParameter). */
    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Reads the component scope after the keyword; nullopt on error. Used also by the session reader. */
    static std::optional<ComponentPlacement> read_component_scope(IJFlexScanner& p_scanner);

    static std::optional<ComponentPlacement::ComponentLocation> read_place_scope(IJFlexScanner& p_scanner);
    static std::optional<ComponentPlacement::ItemClearanceInfo> read_item_clearance_info(IJFlexScanner& p_scanner);
    static bool read_lock_type(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
