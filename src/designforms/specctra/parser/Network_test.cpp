// Tests for the DSN network scope reader (Java parser.Network): net scopes,
// pins, subnet numbers and ordered subnets (T113). The via infos / via rules /
// via padstacks are covered by the T114a tests below; the class branches are
// stubs until T114b.

#include "designforms/specctra/parser/Network.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "board/BoardLibrary.h"
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

TEST(NetworkTest, ViaInfoAndViaRuleAreReadAndInserted)
{
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

} // namespace
} // namespace freerouting::designforms::specctra::parser
