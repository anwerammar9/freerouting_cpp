#pragma once

#include <optional>
#include <set>
#include <vector>

#include "designforms/specctra/parser/Net.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::board {
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::designforms::specctra::parser {

class CoordinateTransform;
class IJFlexScanner;
class LayerStructure;
class NetList;
class ReadScopeParameter;

/**
 * Reads the (network ...) scope of a dsn file. Port of the read part of Java
 * parser.Network. T113 ports the scope skeleton plus the net scopes
 * (read_net_scope / read_net_pins / create_ordered_subnets); the via, via-rule,
 * class and class-class branches plus the component insertion follow in
 * T114a/T114b/T115.
 */
class Network : public ScopeKeyword
{
public:
    Network() : ScopeKeyword("network") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Java Network.createOrderedSubnets: 2-pin subnets from consecutive pins. */
    static std::vector<std::set<Net::Pin>> create_ordered_subnets(const std::vector<Net::Pin>& p_pin_list);

    /**
     * Reads one (net ...) scope; the "(net" was read already. Java
     * Network.readNetScope.
     */
    static bool read_net_scope(IJFlexScanner& p_scanner, NetList& p_net_list, board::RoutingBoard& p_board,
        const CoordinateTransform& p_coordinate_transform, const LayerStructure& p_layer_structure);

    /** Reads a pin list of a net scope. Java Network.readNetPins; nullopt = Java false. */
    static std::optional<std::vector<Net::Pin>> read_net_pins(IJFlexScanner& p_scanner);
};

} // namespace freerouting::designforms::specctra::parser
