// Tests for the java.util.Random port. Expected values were printed by the JDK
// (Temurin 25.0.4.1) with a throwaway program using java.util.Random / Collections.shuffle.

#include "datastructures/JavaRandom.h"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace freerouting::datastructures {
namespace {

TEST(JavaRandomTest, NextIntBoundMatchesJdk)
{
    JavaRandom random(99);
    const std::vector<int> expected = {4, 6, 4, 0, 1, 1, 5, 1, 0, 6};
    for (int value : expected)
    {
        EXPECT_EQ(value, random.next_int(7));
    }
}

TEST(JavaRandomTest, NextIntPowerOfTwoBoundMatchesJdk)
{
    JavaRandom random(99);
    const std::vector<int> expected = {11, 6, 5, 11, 13, 6, 13, 9, 12, 0};
    for (int value : expected)
    {
        EXPECT_EQ(value, random.next_int(16));
    }
}

TEST(JavaRandomTest, NextIntUnboundedMatchesJdk)
{
    JavaRandom random(99);
    const std::vector<int32_t> expected = {-1192035722, 1672896916, 1491444859, -1244121273, -791680696};
    for (int32_t value : expected)
    {
        EXPECT_EQ(value, random.next_int());
    }
}

TEST(JavaRandomTest, NextDoubleMatchesJdkBitForBit)
{
    JavaRandom random(99);
    const std::vector<int64_t> expected_bits = {4604682538640585484LL, 4599927192999002996LL,
        4605522144080823135LL, 4605984827647199408LL, 4605088735798932770LL};
    for (int64_t bits : expected_bits)
    {
        const double value = random.next_double();
        int64_t actual_bits;
        std::memcpy(&actual_bits, &value, sizeof(actual_bits));
        EXPECT_EQ(bits, actual_bits);
    }
}

TEST(JavaRandomTest, NegativeSeedAndLargeBoundMatchJdk)
{
    JavaRandom random(-5);
    for (int value : {411, 721, 953, 328, 352})
    {
        EXPECT_EQ(value, random.next_int(1000));
    }
    random.set_seed(99);
    for (int value : {836448458, 745722429, 840971762, 1275104006, 120279599})
    {
        EXPECT_EQ(value, random.next_int(1500000001));
    }
}

TEST(JavaRandomTest, SetSeedResetsTheSequence)
{
    JavaRandom random(99);
    const int first = random.next_int(7);
    random.next_int(7);
    random.set_seed(99);
    EXPECT_EQ(first, random.next_int(7));
}

TEST(JavaRandomTest, KnownAnchorFromMemory)
{
    // 01_MEMORY.md gotcha 19: new Random(1000).next(32) == -1244746321
    JavaRandom random(1000);
    EXPECT_EQ(-1244746321, random.next_int());
}

TEST(JavaRandomTest, ShuffleMatchesCollectionsShuffle)
{
    std::vector<int> list = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    JavaRandom random(99);
    java_shuffle(list, random);
    EXPECT_EQ((std::vector<int>{8, 1, 4, 3, 9, 6, 0, 2, 5, 7}), list);

    std::vector<int> small = {0, 1, 2};
    JavaRandom random_small(99);
    java_shuffle(small, random_small);
    EXPECT_EQ((std::vector<int>{2, 0, 1}), small);
}

} // namespace
} // namespace freerouting::datastructures
