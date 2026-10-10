// Tests for the parser Net / NetList (Java parser.Net, parser.NetList).

#include "designforms/specctra/parser/NetList.h"

#include <gtest/gtest.h>

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(NetListTest, AddNetTwiceReturnsNullTheSecondTime)
{
    NetList list;
    Net* net = list.add_net({"GND", 1});
    ASSERT_NE(nullptr, net);
    EXPECT_EQ(nullptr, list.add_net({"GND", 1}));
    EXPECT_TRUE(list.contains({"GND", 1}));
    EXPECT_FALSE(list.contains({"GND", 2}));
    EXPECT_EQ(net, list.get_net({"GND", 1}));
    EXPECT_EQ(nullptr, list.get_net({"VCC", 1}));
}

TEST(NetListTest, IdOrderIsNameThenSubnet)
{
    EXPECT_LT((Net::Id{"A", 1}), (Net::Id{"A", 2}));
    EXPECT_LT((Net::Id{"A", 9}), (Net::Id{"B", 1}));
}

TEST(NetListTest, PinsAreUnsetUntilSetAndStoredSorted)
{
    Net net({"N", 1});
    EXPECT_FALSE(net.get_pins().has_value());
    net.set_pins({{"U2", "3"}, {"R1", "1"}, {"U2", "3"}, {"R1", "10"}});
    ASSERT_TRUE(net.get_pins().has_value());
    std::vector<Net::Pin> pins(net.get_pins()->begin(), net.get_pins()->end());
    ASSERT_EQ(3u, pins.size()); // duplicates collapse like a Java TreeSet
    EXPECT_EQ((Net::Pin{"R1", "1"}), pins[0]);
    EXPECT_EQ((Net::Pin{"R1", "10"}), pins[1]);
    EXPECT_EQ((Net::Pin{"U2", "3"}), pins[2]);
}

TEST(NetListTest, GetNetsReturnsTheNetsOfAPinInIdOrder)
{
    NetList list;
    list.add_net({"VCC", 1})->set_pins({{"U1", "1"}});
    list.add_net({"GND", 2})->set_pins({{"U1", "1"}, {"U1", "2"}});
    list.add_net({"GND", 1})->set_pins({{"U1", "2"}});
    list.add_net({"EMPTY", 1}); // pins never set (Java null)
    std::vector<Net*> nets = list.get_nets("U1", "1");
    ASSERT_EQ(2u, nets.size());
    EXPECT_EQ((Net::Id{"GND", 2}), nets[0]->id);
    EXPECT_EQ((Net::Id{"VCC", 1}), nets[1]->id);
    EXPECT_TRUE(list.get_nets("U9", "1").empty());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
