#pragma once

#include <optional>
#include <vector>

#include "board/FixedState.h"
#include "designforms/specctra/parser/Net.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::board {
class Item;
class Padstack;
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::geometry::planar {
class IntPoint;
} // namespace freerouting::geometry::planar

namespace freerouting::rules {
class BoardRules;
class Net;
} // namespace freerouting::rules

namespace freerouting::designforms::specctra::parser {

class IJFlexScanner;

/**
 * Reads the (wiring ...) scope of a dsn file: wires (traces and conduction areas) and vias.
 * Port of the read part of Java parser.Wiring (the write part follows with the DSN/SES writers).
 * The board's traces are normalized at the end of the scope (Java BasicBoard.normalizeAllTraces).
 */
class Wiring : public ScopeKeyword
{
public:
    Wiring() : ScopeKeyword("wiring") {}

    bool read_scope(ReadScopeParameter& p_par) const override;

    /** Reads the fixed state of a (type ...) scope; the "(type" was read. Java Wiring.calcFixed. */
    static board::FixedState calc_fixed(IJFlexScanner& p_scanner);

    /**
     * Reads a (net <name> [subnet]) scope; the "(net" was read. The subnet number is 0 if absent.
     * Java Wiring.readNetId.
     */
    static Net::Id read_net_id(IJFlexScanner& p_scanner);

    /**
     * The board nets of p_net_id: the one subnet if the subnet number is > 0, else all nets of that
     * name; empty for nullopt. Java Wiring.getSubnets.
     */
    static std::vector<rules::Net*> get_subnets(const std::optional<Net::Id>& p_net_id, rules::BoardRules& p_rules);

private:
    /** Java Wiring.readWireScope; the "(wire" was read. Returns the inserted item or nullptr. */
    static board::Item* read_wire_scope(ReadScopeParameter& p_par);

    /** Java Wiring.readViaScope; the "(via" was read. Returns false on an error that aborts the scope. */
    static bool read_via_scope(ReadScopeParameter& p_par);

    /** Java Wiring.tryCorrectNet: assign the net of a contact to a net-less trace (Mentor turrets). */
    static void try_correct_net(board::Item* p_item);

    /** Java Wiring.viaExists: a via with this padstack layer range, nets and center exists already. */
    static bool via_exists(const geometry::planar::IntPoint& p_location, const board::Padstack& p_padstack,
        const std::vector<int>& p_net_numbers, const board::RoutingBoard& p_board);
};

} // namespace freerouting::designforms::specctra::parser
