#include "datastructures/BigIntAux.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {

TEST(BigIntAuxTest, BinaryGcdHandlesZero)
{
    EXPECT_EQ(BigIntAux::binary_gcd(0, 12), 12);
    EXPECT_EQ(BigIntAux::binary_gcd(12, 0), 12);
    EXPECT_EQ(BigIntAux::binary_gcd(0, 0), 0);
}

TEST(BigIntAuxTest, BinaryGcdFindsCommonDivisor)
{
    EXPECT_EQ(BigIntAux::binary_gcd(84, 30), 6);
    EXPECT_EQ(BigIntAux::binary_gcd(17, 29), 1);
}

TEST(BigIntAuxTest, BinaryGcdUsesUnsignedInputSemantics)
{
    EXPECT_EQ(BigIntAux::binary_gcd(-8, 12), 4);
}

TEST(BigIntAuxTest, DeterminantAndCoordinateAdditionRemainAvailable)
{
    EXPECT_EQ(BigIntAux::determinant(2, 3, 5, 7), -1);

    const std::array<BigInt, 3> first = {1, 2, 3};
    const std::array<BigInt, 3> second = {4, 5, 3};
    const auto result = BigIntAux::add_rational_coordinates(first, second);
    EXPECT_EQ(result[0], 5);
    EXPECT_EQ(result[1], 7);
    EXPECT_EQ(result[2], 3);
}

} // namespace freerouting::datastructures