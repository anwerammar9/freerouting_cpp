// Tests for the port of Java rules.BoardRules / NetClasses.

#include "rules/BoardRules.h"

#include <gtest/gtest.h>

#include <memory>

#include "board/Padstack.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::rules {
namespace {

board::LayerStructure two_layers()
{
    return board::LayerStructure(
        { std::make_shared<board::Layer>("F.Cu", true), std::make_shared<board::Layer>("B.Cu", true) });
}

TEST(BoardRulesTest, JavaDefaults)
{
    BoardRules rules(two_layers(), ClearanceMatrix::get_default_instance(two_layers(), 0));
    EXPECT_EQ(100000, rules.get_min_trace_half_width());
    EXPECT_EQ(100, rules.get_max_trace_half_width());
    EXPECT_EQ(AngleRestriction::FORTY_FIVE, rules.get_trace_angle_restriction());
    EXPECT_EQ(nullptr, rules.get_default_via_rule());
}

TEST(BoardRulesTest, DefaultNetClassIsCreatedOnDemandAndUsedForNewNets)
{
    BoardRules rules(two_layers(), 200);
    Net* net = rules.get_nets().add("GND", 1, false);
    ASSERT_NE(nullptr, net->get_net_class());
    EXPECT_EQ("default", net->get_net_class()->get_name());
    EXPECT_EQ(1500, net->get_net_class()->get_trace_half_width(0));
    EXPECT_EQ(1, net->get_net_class()->get_trace_clearance_class());
    EXPECT_EQ(1500, rules.get_trace_half_width(net->get_net_no(), 1));
}

TEST(BoardRulesTest, DefaultTraceWidthsUpdateMinAndMax)
{
    BoardRules rules(two_layers(), 200);
    rules.set_default_trace_half_widths(125);
    EXPECT_EQ(125, rules.get_min_trace_half_width());
    EXPECT_EQ(125, rules.get_max_trace_half_width());
    rules.set_default_trace_half_width(1, 300);
    EXPECT_EQ(300, rules.get_default_trace_half_width(1));
    EXPECT_EQ(300, rules.get_max_trace_half_width());
    Net* net = rules.get_nets().add("N1", 1, false);
    EXPECT_TRUE(rules.trace_widths_are_layer_dependent(net->get_net_no()));
}

TEST(BoardRulesTest, AppendNetClassReturnsTheExistingClassAndCopiesTheDefaults)
{
    BoardRules rules(two_layers(), 200);
    rules.set_default_trace_half_widths(150);
    NetClass* power = rules.append_net_class("power");
    EXPECT_EQ(power, rules.append_net_class("power"));
    EXPECT_EQ(150, power->get_trace_half_width(0));
    NetClass* generated = rules.get_new_net_class();
    EXPECT_EQ("class1", generated->get_name());
    EXPECT_EQ(3, rules.get_net_classes().count());
}

TEST(BoardRulesTest, DefaultViaRuleTakesTheSmallestViaPerLayerRange)
{
    BoardRules rules(two_layers(), 200);
    auto make_padstack = [](const std::string& p_name, int p_half) {
        std::vector<std::unique_ptr<geometry::planar::Shape>> shapes;
        for (int i = 0; i < 2; ++i)
        {
            shapes.push_back(std::make_unique<geometry::planar::ConvexTileShape>(
                std::make_unique<geometry::planar::IntBox>(-p_half, -p_half, p_half, p_half)));
        }
        return std::make_unique<board::Padstack>(p_name, 1, std::move(shapes), true, false);
    };
    auto big = make_padstack("big", 400);
    auto small = make_padstack("small", 300);
    rules.get_via_infos().add(std::make_shared<ViaInfo>("big", big.get(), 1, false, &rules));
    rules.get_via_infos().add(std::make_shared<ViaInfo>("small", small.get(), 1, false, &rules));
    NetClass* default_class = rules.get_default_net_class();
    rules.create_default_via_rule(default_class, "default");
    ViaRule* rule = rules.get_default_via_rule();
    ASSERT_NE(nullptr, rule);
    EXPECT_EQ(rule, default_class->get_via_rule());
    EXPECT_EQ(rule, rules.get_via_rule("default"));
    ASSERT_EQ(1, rule->via_count());
    EXPECT_EQ("small", rule->get_via(0)->get_name());
    EXPECT_DOUBLE_EQ(600.0, rules.get_default_via_diameter());
}

} // namespace
} // namespace freerouting::rules
