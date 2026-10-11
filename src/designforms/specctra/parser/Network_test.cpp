// Tests for the DSN network scope reader (Java parser.Network): net scopes,
// pins, subnet numbers and ordered subnets (T113). The via infos / via rules /
// via padstacks are covered by the T114a tests below; the class branches are
// stubs until T114b.

#include "designforms/specctra/parser/Network.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "board/BoardLibrary.h"
#include "board/Component.h"
#include "board/ComponentOutline.h"
#include "board/LogicalPart.h"
#include "board/ObstacleArea.h"
#include "board/ViaObstacleArea.h"
#include "board/Pin.h"
#include "board/Padstack.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/NetList.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "rules/Net.h"
#include "rules/NetClass.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"

namespace freerouting::designforms::specctra::parser {
namespace {

constexpr const char* kStructure =
    "(resolution um 10)"
    "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
    " (boundary (rect pcb 0 0 100000 100000)))";

std::vector<Net::Pin> net_pins(const ReadScopeParameter& p_par, const std::string& p_name, int p_subnet)
{
    Net* net = p_par.netlist.get_net({ p_name, p_subnet });
    if (net == nullptr || !net->get_pins().has_value())
    {
        return {};
    }
    const auto& pins = *net->get_pins();
    return std::vector<Net::Pin>(pins.begin(), pins.end());
}

TEST(NetworkTest, CreateOrderedSubnetsPairsConsecutivePins)
{
    const std::vector<Net::Pin> pins = { { "U1", "1" }, { "U1", "2" }, { "U1", "3" } };
    const std::vector<std::set<Net::Pin>> subnets = Network::create_ordered_subnets(pins);
    ASSERT_EQ(2u, subnets.size());
    EXPECT_EQ((std::set<Net::Pin>{ { "U1", "1" }, { "U1", "2" } }), subnets[0]);
    EXPECT_EQ((std::set<Net::Pin>{ { "U1", "2" }, { "U1", "3" } }), subnets[1]);
    EXPECT_TRUE(Network::create_ordered_subnets({}).empty());
}

TEST(NetworkTest, NetScopeReadsPinsAndCreatesTheBoardNet)
{
    SpecctraDsnStreamReader scanner(std::string(kStructure) + "(network (net GND (pins U1-1 U1-2)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    ASSERT_TRUE(par.netlist.contains({ "GND", 1 }));
    const std::vector<Net::Pin> pins = net_pins(par, "GND", 1);
    ASSERT_EQ(2u, pins.size());
    EXPECT_EQ((Net::Pin{ "U1", "1" }), pins[0]);
    EXPECT_EQ((Net::Pin{ "U1", "2" }), pins[1]);

    ASSERT_NE(nullptr, par.get_board());
    EXPECT_EQ(1, par.get_board()->get_rules().get_nets().max_net_no());
    EXPECT_EQ("GND", par.get_board()->get_rules().get_nets().get(1)->get_name());
}

TEST(NetworkTest, ExplicitSubnetNumberIsKept)
{
    SpecctraDsnStreamReader scanner(std::string(kStructure) + "(network (net VCC 2 (pins U2-1)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    EXPECT_TRUE(par.netlist.contains({ "VCC", 2 }));
    EXPECT_FALSE(par.netlist.contains({ "VCC", 1 }));
    ASSERT_NE(nullptr, par.get_board());
    ASSERT_NE(nullptr, par.get_board()->get_rules().get_nets().get("VCC", 2));
}

TEST(NetworkTest, EachFromtoBecomesItsOwnSubnetWithIncreasingNumber)
{
    SpecctraDsnStreamReader scanner(
        std::string(kStructure) + "(network (net VCC (fromto U2-1 U2-2) (fromto U2-2 U2-3)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    EXPECT_TRUE(par.netlist.contains({ "VCC", 1 }));
    EXPECT_TRUE(par.netlist.contains({ "VCC", 2 }));
    EXPECT_EQ((std::vector<Net::Pin>{ { "U2", "1" }, { "U2", "2" } }), net_pins(par, "VCC", 1));
    EXPECT_EQ((std::vector<Net::Pin>{ { "U2", "2" }, { "U2", "3" } }), net_pins(par, "VCC", 2));
}

TEST(NetworkTest, OrderCreatesConsecutiveTwoPinSubnets)
{
    SpecctraDsnStreamReader scanner(std::string(kStructure) + "(network (net N (order U1-1 U1-2 U1-3)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    EXPECT_TRUE(par.netlist.contains({ "N", 1 }));
    EXPECT_TRUE(par.netlist.contains({ "N", 2 }));
    EXPECT_FALSE(par.netlist.contains({ "N", 3 }));
    EXPECT_EQ((std::vector<Net::Pin>{ { "U1", "1" }, { "U1", "2" } }), net_pins(par, "N", 1));
    EXPECT_EQ((std::vector<Net::Pin>{ { "U1", "2" }, { "U1", "3" } }), net_pins(par, "N", 2));
}

TEST(NetworkTest, EmptyNetScopeCreatesAnEmptyNet)
{
    SpecctraDsnStreamReader scanner(std::string(kStructure) + "(network (net X))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    ASSERT_TRUE(par.netlist.contains({ "X", 1 }));
    EXPECT_TRUE(net_pins(par, "X", 1).empty());
    ASSERT_NE(nullptr, par.get_board());
    ASSERT_NE(nullptr, par.get_board()->get_rules().get_nets().get("X", 1));
}

TEST(NetworkTest, WidthRuleCreatesANewNetClass)
{
    // resolution um 10: dsn 0.3 -> board 3, half width round(3/2) = 2.
    SpecctraDsnStreamReader scanner(
        std::string(kStructure) + "(network (net W (pins U1-1 U1-2) (rule (width 0.3))))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));

    ASSERT_NE(nullptr, par.get_board());
    const rules::Net* board_net = par.get_board()->get_rules().get_nets().get("W", 1);
    ASSERT_NE(nullptr, board_net);
    ASSERT_NE(nullptr, board_net->get_net_class());
    EXPECT_EQ(2, board_net->get_net_class()->get_trace_half_width(0));
    EXPECT_EQ(2, par.get_board()->get_rules().get_net_classes().count()); // default + the new class
}

TEST(NetworkTest, PlusNetNameIsReadRawByNextString)
{
    // readNetScope uses scanner.nextString(), which reads the raw buffer (unlike the
    // identifier tokenizer that skips illegal characters); Java keeps the '+'.
    SpecctraDsnStreamReader scanner(std::string(kStructure) + "(network (net +5V (pins U1-1)))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    EXPECT_TRUE(par.netlist.contains({ "+5V", 1 }));
}

TEST(NetworkTest, NetScopeWithoutStructureFails)
{
    SpecctraDsnStreamReader scanner("(network (net X))", true);
    ReadScopeParameter par(scanner);
    EXPECT_FALSE(Keyword::NETWORK_SCOPE->read_scope(par));
    EXPECT_EQ(nullptr, par.get_board());
}

TEST(NetworkTest, ViaInfoAndViaRuleAreReadAndInserted){
    // structure declares the via padstack list, network the via info and the via rule.
    SpecctraDsnStreamReader scanner(
        "(resolution um 10)"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (via MyVia) (boundary (rect pcb 0 0 100000 100000)))"
        "(library (padstack MyVia (shape (circle signal 600))))"
        "(network (via V1 MyVia default attach) (via_rule default V1))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    // The structure's (via MyVia) list made MyVia a via padstack.
    ASSERT_EQ(1, board.library().via_padstack_count());
    EXPECT_EQ("MyVia", board.library().get_via_padstack(0)->get_name());

    // readViaInfo added V1 to the via infos (clearance class 1 = default).
    rules::ViaInfos& via_infos = board.mutable_rules().get_via_infos();
    ASSERT_EQ(1, via_infos.count());
    std::shared_ptr<rules::ViaInfo> v1 = via_infos.get("V1");
    ASSERT_NE(nullptr, v1);
    EXPECT_EQ(board.library().get_via_padstack(0), v1->get_padstack());
    EXPECT_EQ(1, v1->get_clearance_class());
    EXPECT_TRUE(v1->attach_smd_allowed());

    // insertViaRules created the "default" rule from V1 and assigned it to every net class.
    rules::ViaRule* default_rule = board.mutable_rules().get_via_rule("default");
    ASSERT_NE(nullptr, default_rule);
    ASSERT_EQ(1, default_rule->via_count());
    EXPECT_EQ(v1, default_rule->get_via(0));
    ASSERT_EQ(1, board.mutable_rules().get_net_classes().count());
    EXPECT_EQ(default_rule, board.mutable_rules().get_net_classes().get(0)->get_via_rule());
}

TEST(NetworkTest, NoViaInfoCreatesDefaultViaInfosFromViaPadstacks)
{
    SpecctraDsnStreamReader scanner(
        "(resolution um 10)"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (via MyVia) (boundary (rect pcb 0 0 100000 100000)))"
        "(library (padstack MyVia (shape (circle signal 600)) (attach off)))"
        "(network (net N (pins U1-1)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    ASSERT_EQ(1, board.library().via_padstack_count());
    rules::ViaInfos& via_infos = board.mutable_rules().get_via_infos();
    ASSERT_EQ(1, via_infos.count());
    std::shared_ptr<rules::ViaInfo> via = via_infos.get("MyVia");
    ASSERT_NE(nullptr, via);
    EXPECT_EQ(1, via->get_clearance_class());
    // (attach off) on the padstack, so even with viaAtSmdAllowed the via is not attachable.
    EXPECT_FALSE(via->attach_smd_allowed());

    rules::ViaRule* default_rule = board.mutable_rules().get_via_rule("default");
    ASSERT_NE(nullptr, default_rule);
    ASSERT_EQ(1, default_rule->via_count());
    EXPECT_EQ(via, default_rule->get_via(0));
}

TEST(NetworkTest, MissingViaPadstackIsDroppedWithAWarning)
{
    SpecctraDsnStreamReader scanner(
        "(resolution um 10)"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (via NoSuchPadstack) (boundary (rect pcb 0 0 100000 100000)))"
        "(library (padstack Foo (shape (circle signal 600))))"
        "(network (net N (pins U1-1)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    // NoSuchPadstack is not in the library, so the via padstack list stays empty and no default
    // via info (nor via rule) can be created.
    EXPECT_EQ(0, board.library().via_padstack_count());
    EXPECT_EQ(0, board.mutable_rules().get_via_infos().count());
    EXPECT_EQ(nullptr, board.mutable_rules().get_via_rule("default"));
}

TEST(NetworkTest, ViaRuleWithoutViaInfosFallsBackToTheDefaultRule)
{
    // The via rule names a via info that does not exist: addViaRule fails, so insertViaRules
    // creates the default rule from the (also empty) via infos.
    SpecctraDsnStreamReader scanner(
        "(resolution um 10)"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (boundary (rect pcb 0 0 100000 100000)))"
        "(network (via_rule r1 NoSuchVia))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    EXPECT_EQ(nullptr, board.mutable_rules().get_via_rule("r1"));
    // createDefaultViaRule returns early with no via infos, so no rule exists at all.
    EXPECT_EQ(nullptr, board.mutable_rules().get_default_via_rule());
}

TEST(NetworkTest, ClassCreatesANetClassAndAssignsItToTheNet)
{
    // resolution um 10: dsn 0.5 -> board 5, half width round(5/2) = 3.
    SpecctraDsnStreamReader scanner(
        std::string(kStructure) + "(network (net A (pins U1-1)) (class PWR A (rule (width 0.5))))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    rules::Net* net = board.get_rules().get_nets().get("A", 1);
    ASSERT_NE(nullptr, net);
    ASSERT_NE(nullptr, net->get_net_class());
    EXPECT_EQ("PWR", net->get_net_class()->get_name());
    EXPECT_EQ(3, net->get_net_class()->get_trace_half_width(0));
    // default + PWR
    EXPECT_EQ(2, board.get_rules().get_net_classes().count());
}

TEST(NetworkTest, KiCadDefaultClassNameMapsToTheDefaultNetClass)
{
    SpecctraDsnStreamReader scanner(
        std::string(kStructure) + "(network (net A (pins U1-1)) (class kicad_default A (rule (width 0.5))))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    rules::Net* net = board.get_rules().get_nets().get("A", 1);
    ASSERT_NE(nullptr, net);
    EXPECT_EQ(board.get_rules().get_default_net_class(), net->get_net_class());
    // No new class was appended.
    EXPECT_EQ(1, board.get_rules().get_net_classes().count());
}

TEST(NetworkTest, ClearanceRuleCreatesAClearanceClass)
{
    // resolution um 10: dsn 0.4 -> board 4.
    SpecctraDsnStreamReader scanner(
        std::string(kStructure) + "(network (net A (pins U1-1)) (class PWR A (rule (clearance 0.4))))", true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    rules::ClearanceMatrix& matrix = board.mutable_rules().get_clearance_matrix();
    const int class_no = matrix.get_no("PWR");
    ASSERT_GE(class_no, 0);
    rules::Net* net = board.get_rules().get_nets().get("A", 1);
    ASSERT_NE(nullptr, net);
    ASSERT_NE(nullptr, net->get_net_class());
    EXPECT_EQ(class_no, net->get_net_class()->get_trace_clearance_class());
    EXPECT_EQ(4, matrix.get_value(class_no, class_no, 0));
}

TEST(NetworkTest, ClassPairClearanceRuleSetsMixedClearance)
{
    // resolution um 10: dsn 0.6 -> board 6.
    SpecctraDsnStreamReader scanner(
        std::string(kStructure)
            + "(network (net A (pins U1-1)) (net B (pins U2-1))"
              " (class PWR A (rule (clearance 0.4))) (class SIG B (rule (width 0.5)))"
              " (class_class (classes PWR SIG) (rule (clearance 0.6))))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    rules::ClearanceMatrix& matrix = board.mutable_rules().get_clearance_matrix();
    const int pwr_no = matrix.get_no("PWR");
    const int sig_no = matrix.get_no("SIG");
    ASSERT_GE(pwr_no, 0);
    ASSERT_GE(sig_no, 0);
    EXPECT_EQ(6, matrix.get_value(pwr_no, sig_no, 0));
    EXPECT_EQ(6, matrix.get_value(sig_no, pwr_no, 0));
}

TEST(NetworkTest, ClassUseViaCreatesAViaRule)
{
    SpecctraDsnStreamReader scanner(
        "(resolution um 10)"
        "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
        " (boundary (rect pcb 0 0 100000 100000)))"
        "(library (padstack MyVia (shape (circle signal 600))))"
        "(network (net A (pins U1-1)) (class PWR A (rule (clearance 0.4)) (circuit (use_via MyVia))))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    ASSERT_EQ(1, board.library().via_padstack_count());
    rules::ViaRule* pwr_rule = board.mutable_rules().get_via_rule("PWR");
    ASSERT_NE(nullptr, pwr_rule);
    ASSERT_EQ(1, pwr_rule->via_count());
    EXPECT_EQ("MyVia-PWR", pwr_rule->get_via(0)->get_name());
    rules::Net* net = board.get_rules().get_nets().get("A", 1);
    ASSERT_NE(nullptr, net);
    ASSERT_NE(nullptr, net->get_net_class());
    EXPECT_EQ(pwr_rule, net->get_net_class()->get_via_rule());
}

TEST(NetworkTest, ClassUseLayerRestrictsTheActiveTraceLayers)
{
    SpecctraDsnStreamReader scanner(
        std::string(kStructure)
            + "(network (net A (pins U1-1)) (class SIG A (rule (width 0.5)) (circuit (use_layer F.Cu))))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    board::RoutingBoard& board = *par.get_board();

    rules::Net* net = board.get_rules().get_nets().get("A", 1);
    ASSERT_NE(nullptr, net);
    ASSERT_NE(nullptr, net->get_net_class());
    const rules::NetClass* net_class = net->get_net_class();
    EXPECT_TRUE(net_class->is_active_routing_layer(0));
    EXPECT_FALSE(net_class->is_active_routing_layer(1));
    EXPECT_EQ(3, net_class->get_trace_half_width(0));
    EXPECT_EQ(0, net_class->get_trace_half_width(1));
}


// ---- T115: component, pin, keepout and logical part insertion ----

// Expected values below come from the Java 2.1.0 jar (throwaway PinCenters.java dumper, see
// docs/migration/MIGRATION_designforms.md, T115).
constexpr const char* kTwoComponentBoard =
    "(pcb two_comp"
    " (parser (host_cad \"test\") (host_version \"1\"))"
    " (resolution um 10) (unit um)"
    " (structure (layer F.Cu (type signal)) (layer B.Cu (type signal))"
    "  (boundary (rect pcb 0 0 100000 100000)) (via V1) (rule (width 250) (clearance 200)))"
    " (placement (component IMG (place U1 20000 30000 front 0) (place U2 60000 50000 back 90)))"
    " (library"
    "  (image IMG (pin SMD 1 -1000 0) (pin THRU 2 1000 500)"
    "   (keepout \"k1\" (rect F.Cu -500 -500 500 500)) (via_keepout \"\" (rect B.Cu -300 -300 300 300)))"
    "  (padstack SMD (shape (rect F.Cu -200 -200 200 200)))"
    "  (padstack THRU (shape (circle F.Cu 600)) (shape (circle B.Cu 600)))"
    "  (padstack V1 (shape (circle F.Cu 400)) (shape (circle B.Cu 400))))"
    " (network (net GND (pins U1-1 U2-1)) (net VCC (pins U1-2 U2-2)))"
    " (wiring))";

/** "PIN <component> <pin> <x> <y> [<nets>] <clearance class>" lines, sorted like the Java dumper. */
std::vector<std::string> pin_lines(const board::RoutingBoard& p_board)
{
    std::vector<std::string> lines;
    for (board::Item* item : p_board.get_items())
    {
        auto* pin = dynamic_cast<board::Pin*>(item);
        if (pin == nullptr)
        {
            continue;
        }
        std::string nets;
        for (int i = 0; i < pin->net_count(); ++i)
        {
            nets += (i > 0 ? "," : "") + std::to_string(pin->get_net_no(i));
        }
        const auto& center = pin->get_center();
        lines.push_back("PIN " + std::to_string(pin->get_component_no()) + " " + std::to_string(pin->get_pin_no()) + " "
            + std::to_string(center.x) + " " + std::to_string(center.y) + " [" + nets + "] "
            + std::to_string(pin->get_clearance_class()));
    }
    std::sort(lines.begin(), lines.end());
    return lines;
}

TEST(NetworkTest, ComponentPinsArePlacedRotatedAndMirrored)
{
    SpecctraDsnStreamReader scanner(kTwoComponentBoard, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    board::RoutingBoard* board = par.get_board();
    ASSERT_NE(nullptr, board);
    ASSERT_EQ(2, board->components()->component_count());
    EXPECT_EQ("U1", board->components()->get(1)->get_name());
    EXPECT_FALSE(board->components()->get(2)->is_placed_on_front());

    const std::vector<std::string> expected = {
        "PIN 1 0 190000 300000 [1] 1",
        "PIN 1 1 210000 305000 [2] 1",
        "PIN 2 0 600000 510000 [1] 1",
        "PIN 2 1 595000 490000 [2] 1",
    };
    EXPECT_EQ(expected, pin_lines(*board));
    for (board::Item* item : board->get_items())
    {
        if (auto* pin = dynamic_cast<board::Pin*>(item))
        {
            EXPECT_NE(nullptr, pin->get_shape(0)) << "component " << pin->get_component_no() << " pin "
                                                   << pin->get_pin_no();
        }
    }
}

TEST(NetworkTest, PackageKeepoutsOfBackSideComponentsUseTheMirroredLayer)
{
    SpecctraDsnStreamReader scanner(kTwoComponentBoard, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    std::vector<std::string> areas;
    for (board::Item* item : par.get_board()->get_items())
    {
        auto* area = dynamic_cast<board::ObstacleArea*>(item);
        if (area == nullptr)
        {
            continue;
        }
        const auto box = area->bounding_box();
        const bool is_via_keepout = dynamic_cast<board::ViaObstacleArea*>(item) != nullptr;
        areas.push_back(std::string(is_via_keepout ? "via " : "area ") + std::to_string(area->get_component_no()) + " "
            + std::to_string(area->get_layer()) + " " + std::to_string(area->get_clearance_class()) + " "
            + std::to_string(box.ll.x) + "," + std::to_string(box.ll.y) + " " + std::to_string(box.ur.x) + ","
            + std::to_string(box.ur.y));
    }
    std::sort(areas.begin(), areas.end());
    const std::vector<std::string> expected = {
        "area 1 0 1 195000,295000 205000,305000",
        "area 2 1 1 595000,495000 605000,505000",
        "via 1 1 1 197000,297000 203000,303000",
        "via 2 0 1 597000,497000 603000,503000",
    };
    EXPECT_EQ(expected, areas);
}

TEST(NetworkTest, UnplacedComponentIsAddedWithoutPins)
{
    std::string board_text = kTwoComponentBoard;
    const std::string placed = "(place U2 60000 50000 back 90)";
    board_text.replace(board_text.find(placed), placed.size(), "(place U2)");
    SpecctraDsnStreamReader scanner(board_text, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    board::RoutingBoard* board = par.get_board();
    ASSERT_EQ(2, board->components()->component_count());
    EXPECT_FALSE(board->components()->get(2)->is_placed());
    EXPECT_EQ(2u, pin_lines(*board).size());
}

TEST(NetworkTest, LogicalPartsAreInsertedAndAssignedToComponents)
{
    std::string board_text = kTwoComponentBoard;
    // The part library scope precedes the network scope, whose end inserts the logical parts.
    const std::string network = " (network ";
    board_text.replace(board_text.find(network), network.size(),
        " (part_library (logical_part_mapping GATE (component U1 U2))"
        "  (logical_part GATE (pin 2 0 A 1 out 0) (pin 1 0 A 1 in 0)))"
        " (network ");
    SpecctraDsnStreamReader scanner(board_text, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    board::RoutingBoard* board = par.get_board();
    const board::LogicalParts& parts = board->library().logical_parts();
    ASSERT_EQ(1, parts.count());
    board::LogicalPart* part = parts.get("GATE");
    ASSERT_NE(nullptr, part);
    ASSERT_EQ(2, part->pin_count());
    // Sorted by package pin index: pin "1" is index 0.
    EXPECT_EQ("1", part->get_pin(0)->pin_name);
    EXPECT_EQ(1, part->get_pin(1)->pin_index);
    EXPECT_EQ(part, board->components()->get(1)->get_logical_part());
    EXPECT_EQ(part, board->components()->get(2)->get_logical_part());
}

TEST(NetworkTest, Issue026PinsMatchTheJavaReader)
{
    std::ifstream dsn_file(std::string(FREEROUTING_TEST_DATA_DIR) + "/Issue026-J2_reference.dsn");
    ASSERT_TRUE(dsn_file.good());
    std::stringstream dsn_text;
    dsn_text << dsn_file.rdbuf();
    SpecctraDsnStreamReader scanner(dsn_text.str(), true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    board::RoutingBoard* board = par.get_board();
    ASSERT_NE(nullptr, board);
    EXPECT_EQ(2, board->components()->component_count());

    std::ifstream expected_file(
        std::string(FREEROUTING_TEST_DATA_DIR) + "/expected/Issue026-J2_reference.dsn.pins.txt");
    ASSERT_TRUE(expected_file.good());
    std::vector<std::string> expected;
    for (std::string line; std::getline(expected_file, line);)
    {
        if (!line.empty())
        {
            expected.push_back(line);
        }
    }
    std::sort(expected.begin(), expected.end());
    ASSERT_EQ(59u, expected.size());
    EXPECT_EQ(expected, pin_lines(*board));

    // Item counts of tests/data/expected/Issue026-J2_reference.dsn.counts.json (Java T007 harness).
    int component_outlines = 0;
    int obstacle_areas = 0;
    for (board::Item* item : board->get_items())
    {
        if (dynamic_cast<board::ComponentOutline*>(item) != nullptr)
        {
            ++component_outlines;
        }
        else if (dynamic_cast<board::ObstacleArea*>(item) != nullptr
            && dynamic_cast<board::ViaObstacleArea*>(item) == nullptr)
        {
            ++obstacle_areas;
        }
    }
    EXPECT_EQ(17, component_outlines);
    EXPECT_EQ(4, obstacle_areas);
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
