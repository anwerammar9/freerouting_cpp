// Tests for the DSN class scope reader (Java parser.NetClass): the (class ...)
// scope and the (class_class ...) scope.

#include "designforms/specctra/parser/NetClass.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/Rule.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

namespace freerouting::designforms::specctra::parser {
namespace {

// NetClass.readScope is called right after the "(class" tokens are consumed.
std::optional<NetClass> read_net_class(const std::string& p_text)
{
    SpecctraDsnStreamReader scanner(p_text, true);
    scanner.next_token(); // (
    scanner.next_token(); // class
    return NetClass::read_scope(scanner);
}

// NetClass.readClassClassScope is called right after the "(class_class" tokens.
std::optional<NetClass::ClassClass> read_class_class(const std::string& p_text)
{
    SpecctraDsnStreamReader scanner(p_text, true);
    scanner.next_token(); // (
    scanner.next_token(); // class_class
    return NetClass::read_class_class_scope(scanner);
}

TEST(NetClassTest, ReadScopeReadsNameNetsAndWidthRule)
{
    const std::optional<NetClass> net_class = read_net_class("(class Foo A B (rule (width 0.3)))");
    ASSERT_TRUE(net_class.has_value());
    EXPECT_EQ("Foo", net_class->name);
    ASSERT_EQ(2u, net_class->net_list.size());
    EXPECT_EQ("A", net_class->net_list[0]);
    EXPECT_EQ("B", net_class->net_list[1]);
    ASSERT_EQ(1u, net_class->rules.size());
    const auto* width_rule = dynamic_cast<const WidthRule*>(net_class->rules[0].get());
    ASSERT_NE(nullptr, width_rule);
    EXPECT_DOUBLE_EQ(0.3, width_rule->value);
    // Defaults.
    EXPECT_FALSE(net_class->trace_clearance_class.has_value());
    EXPECT_FALSE(net_class->via_rule.has_value());
    EXPECT_TRUE(net_class->pull_tight);
    EXPECT_FALSE(net_class->shove_fixed);
    EXPECT_DOUBLE_EQ(0, net_class->min_trace_length);
    EXPECT_DOUBLE_EQ(0, net_class->max_trace_length);
}

TEST(NetClassTest, ReadScopeReadsClearanceClassViaRuleAndSwitches)
{
    const std::optional<NetClass> net_class = read_net_class(
        "(class Foo A (clearance_class C) (via_rule VR) (shove_fixed on) (pull_tight off))");
    ASSERT_TRUE(net_class.has_value());
    ASSERT_TRUE(net_class->trace_clearance_class.has_value());
    EXPECT_EQ("C", *net_class->trace_clearance_class);
    ASSERT_TRUE(net_class->via_rule.has_value());
    EXPECT_EQ("VR", *net_class->via_rule);
    EXPECT_TRUE(net_class->shove_fixed);
    EXPECT_FALSE(net_class->pull_tight);
}

TEST(NetClassTest, ReadScopeReadsCircuitUseViaUseLayerAndLength)
{
    const std::optional<NetClass> net_class =
        read_net_class("(class Foo (circuit (use_via V1 V2) (use_layer F.Cu B.Cu) (length 10 5)))");
    ASSERT_TRUE(net_class.has_value());
    ASSERT_EQ(2u, net_class->use_via.size());
    EXPECT_EQ("V1", net_class->use_via[0]);
    EXPECT_EQ("V2", net_class->use_via[1]);
    ASSERT_EQ(2u, net_class->use_layer.size());
    EXPECT_EQ("F.Cu", net_class->use_layer[0]);
    EXPECT_EQ("B.Cu", net_class->use_layer[1]);
    EXPECT_DOUBLE_EQ(10, net_class->max_trace_length);
    EXPECT_DOUBLE_EQ(5, net_class->min_trace_length);
}

TEST(NetClassTest, ReadScopeReadsLayerRulesAndUnknownScopesAreSkipped)
{
    const std::optional<NetClass> net_class =
        read_net_class("(class Foo A (unknown 1 2) (layer_rule F.Cu (rule (width 0.2))))");
    ASSERT_TRUE(net_class.has_value());
    ASSERT_EQ(1u, net_class->layer_rules.size());
    ASSERT_EQ(1u, net_class->layer_rules[0].layer_names.size());
    EXPECT_EQ("F.Cu", net_class->layer_rules[0].layer_names[0]);
    ASSERT_EQ(1u, net_class->layer_rules[0].rules.size());
    const auto* width_rule = dynamic_cast<const WidthRule*>(net_class->layer_rules[0].rules[0].get());
    ASSERT_NE(nullptr, width_rule);
    EXPECT_DOUBLE_EQ(0.2, width_rule->value);
}

TEST(NetClassTest, ReadScopeUnexpectedEndOfFileReturnsNullopt)
{
    EXPECT_FALSE(read_net_class("(class Foo A").has_value());
}

TEST(NetClassTest, ReadClassClassReadsNamesAndClearanceRule)
{
    const std::optional<NetClass::ClassClass> class_class =
        read_class_class("(class_class (classes A B) (rule (clearance 0.3)))");
    ASSERT_TRUE(class_class.has_value());
    ASSERT_EQ(2u, class_class->class_names.size());
    EXPECT_EQ("A", class_class->class_names[0]);
    EXPECT_EQ("B", class_class->class_names[1]);
    ASSERT_EQ(1u, class_class->rules.size());
    const auto* clearance_rule = dynamic_cast<const ClearanceRule*>(class_class->rules[0].get());
    ASSERT_NE(nullptr, clearance_rule);
    EXPECT_DOUBLE_EQ(0.3, clearance_rule->value);
    EXPECT_TRUE(class_class->layer_rules.empty());
}

TEST(NetClassTest, ReadClassClassUnexpectedEndOfFileReturnsNullopt)
{
    EXPECT_FALSE(read_class_class("(class_class (classes A B)").has_value());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
