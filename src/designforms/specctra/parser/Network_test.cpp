// Tests for the DSN network scope reader (Java parser.Network): net scopes,
// pins, subnet numbers and ordered subnets (T113). The via/class branches are
// stubs until T114a/T114b.

#include "designforms/specctra/parser/Network.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/NetList.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "rules/Net.h"
#include "rules/NetClass.h"

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

} // namespace
} // namespace freerouting::designforms::specctra::parser
