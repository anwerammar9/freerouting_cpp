#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "designforms/specctra/parser/Net.h"
#include "designforms/specctra/parser/NetClass.h"
#include "designforms/specctra/parser/Rule.h"
#include "designforms/specctra/parser/ScopeKeyword.h"

namespace freerouting::board {
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::rules {
class ClearanceMatrix;
class NetClass;
class ViaInfo;
} // namespace freerouting::rules

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

    /** Reads a (via ...) scope. Java Network.readViaInfo; nullptr = Java null. */
    static std::shared_ptr<rules::ViaInfo> read_via_info(IJFlexScanner& p_scanner, board::RoutingBoard& p_board);

    /** Reads a (via_rule ...) scope. Java Network.readViaRule; nullopt = Java null. */
    static std::optional<std::vector<std::string>> read_via_rule(IJFlexScanner& p_scanner, board::RoutingBoard& p_board);

    /** Java Network.insertViaInfos: add the read via infos, or create default ones. */
    static void insert_via_infos(const std::vector<std::shared_ptr<rules::ViaInfo>>& p_via_infos,
        board::RoutingBoard& p_board, bool p_attach_allowed);

    /** Java Network.createDefaultViaInfos: one via info per via padstack. */
    static void create_default_via_infos(board::RoutingBoard& p_board, rules::NetClass* p_net_class, bool p_attach_allowed);

    /** Java Network.insertViaRules: add all rules and assign the default rule to every net class. */
    static void insert_via_rules(const std::vector<std::vector<std::string>>& p_via_rules, board::RoutingBoard& p_board);

    /** Java Network.addViaRule: replaces an existing rule with the same name. */
    static bool add_via_rule(const std::vector<std::string>& p_name_list, board::RoutingBoard& p_board);

    /** Java Network.insertNetClasses: add all read classes to the board rules. */
    static void insert_net_classes(const std::vector<NetClass>& p_net_classes, ReadScopeParameter& p_par);

    /** Java Network.insertNetClass: create the board net class and insert its rules. */
    static void insert_net_class(const NetClass& p_net_class, const LayerStructure& p_layer_structure,
        board::RoutingBoard& p_board, const CoordinateTransform& p_coordinate_transform, bool p_via_at_smd_allowed);

    /** Java Network.insertClassPairs: insert the clearance rules of the class pairs. */
    static void insert_class_pairs(const std::vector<NetClass::ClassClass>& p_class_classes, ReadScopeParameter& p_par);

private:
    /** Java Network.insertClassPairInfo. */
    static void insert_class_pair_info(const NetClass::ClassClass& p_class_class, rules::NetClass* p_first_class,
        rules::NetClass* p_second_class, board::RoutingBoard& p_board, const CoordinateTransform& p_coordinate_transform);

    /** Java Network.addMixedClearanceRule. */
    static void add_mixed_clearance_rule(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_first_class,
        rules::NetClass* p_second_class, const ClearanceRule& p_clearance_rule, int p_layer_index,
        const CoordinateTransform& p_coordinate_transform);

    /** Java Network.addClearanceRule: insert a clearance rule of a net class. */
    static void add_clearance_rule(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_net_class,
        const ClearanceRule& p_clearance_rule, int p_layer_index, const CoordinateTransform& p_coordinate_transform);

    /** Java Network.createDefaultClearanceClasses (the via/smd/pin/area classes). */
    static void create_default_clearance_classes(rules::NetClass* p_net_class, rules::ClearanceMatrix& p_clearance_matrix);

    /** Java Network.getClearanceClass: the class "<net class>-<item class>", created on demand. */
    static int get_clearance_class(rules::ClearanceMatrix& p_clearance_matrix, rules::NetClass* p_net_class,
        const std::string& p_item_class_name);

    /** Java Network.createViaRule: a via rule from the use_via names of a net class. */
    static void create_via_rule(const std::vector<std::string>& p_use_via, rules::NetClass* p_net_class,
        board::RoutingBoard& p_board, bool p_attach_allowed);

    /** Java Network.createActiveTraceLayers. */
    static void create_active_trace_layers(const std::vector<std::string>& p_use_layer,
        const LayerStructure& p_layer_structure, rules::NetClass* p_net_class);
};

} // namespace freerouting::designforms::specctra::parser
