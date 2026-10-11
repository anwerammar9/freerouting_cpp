// Tests for the DSN circuit scope reader (Java parser.Circuit).
//
// Java parity: Circuit.readScope(IJFlexScanner) is called right after the
// `circuit` keyword token is consumed (e.g. from NetClass.readScope), so the
// scanner is positioned at the scope content: `( ... )`.

#include "designforms/specctra/parser/Circuit.h"

#include <gtest/gtest.h>

#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

namespace freerouting::designforms::specctra::parser {
namespace {

TEST(CircuitTest, ReadScopeReadsLengthViaAndLayer)
{
    SpecctraDsnStreamReader scanner("(length 10 5) (use_via Via1) (use_layer F.Cu B.Cu))", true);
    std::optional<Circuit::ReadScopeResult> result = Circuit::read_scope(scanner);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(10, result->max_length);
    EXPECT_DOUBLE_EQ(5, result->min_length);
    ASSERT_EQ(1u, result->use_via.size());
    EXPECT_EQ("Via1", result->use_via[0]);
    ASSERT_EQ(2u, result->use_layer.size());
    EXPECT_EQ("F.Cu", result->use_layer[0]);
    EXPECT_EQ("B.Cu", result->use_layer[1]);
}

TEST(CircuitTest, ReadScopeEmptyCircuitHasZeroedLengths)
{
    SpecctraDsnStreamReader scanner(")", true);
    std::optional<Circuit::ReadScopeResult> result = Circuit::read_scope(scanner);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(0, result->max_length);
    EXPECT_DOUBLE_EQ(0, result->min_length);
    EXPECT_TRUE(result->use_via.empty());
    EXPECT_TRUE(result->use_layer.empty());
}

TEST(CircuitTest, ReadScopeSkipsUnknownScopes)
{
    SpecctraDsnStreamReader scanner("(length 10 5 (extra 2)) (unknown 3))", true);
    std::optional<Circuit::ReadScopeResult> result = Circuit::read_scope(scanner);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(10, result->max_length);
    EXPECT_DOUBLE_EQ(5, result->min_length);
}

TEST(CircuitTest, ReadScopeLengthFailureKeepsZeros)
{
    // Java: a failed readLengthScope returns null and the zeros are kept (the scope is not aborted).
    SpecctraDsnStreamReader scanner("(length bad 5))", true);
    std::optional<Circuit::ReadScopeResult> result = Circuit::read_scope(scanner);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(0, result->max_length);
    EXPECT_DOUBLE_EQ(0, result->min_length);
}

TEST(CircuitTest, ReadScopeMultipleUseViaTokens)
{
    SpecctraDsnStreamReader scanner("(use_via V1 V2) (use_via V3) (use_layer L1 L2))", true);
    std::optional<Circuit::ReadScopeResult> result = Circuit::read_scope(scanner);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(3u, result->use_via.size());
    EXPECT_EQ("V1", result->use_via[0]);
    EXPECT_EQ("V2", result->use_via[1]);
    EXPECT_EQ("V3", result->use_via[2]);
    ASSERT_EQ(2u, result->use_layer.size());
    EXPECT_EQ("L1", result->use_layer[0]);
    EXPECT_EQ("L2", result->use_layer[1]);
}

TEST(CircuitTest, ReadScopeUnexpectedEndOfFileReturnsNullopt)
{
    SpecctraDsnStreamReader scanner("(length", true);
    EXPECT_FALSE(Circuit::read_scope(scanner).has_value());
}

} // namespace
} // namespace freerouting::designforms::specctra::parser