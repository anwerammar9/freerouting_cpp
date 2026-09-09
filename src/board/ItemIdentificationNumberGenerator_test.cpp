/**
 * @file ItemIdentificationNumberGenerator_test.cpp
 * @brief Unit tests for ItemIdentificationNumberGenerator
 */

#include "board/ItemIdentificationNumberGenerator.h"

#include <gtest/gtest.h>

namespace freerouting::board {

class ItemIdentificationNumberGeneratorTest : public ::testing::Test
{
protected:
    ItemIdentificationNumberGenerator generator_;
};

TEST_F(ItemIdentificationNumberGeneratorTest, FirstNumberStartsAtOne)
{
    EXPECT_EQ(generator_.new_no(), 1);
    EXPECT_EQ(generator_.max_generated_no(), 1);
}

TEST_F(ItemIdentificationNumberGeneratorTest, NumbersAreSequentiallyUnique)
{
    constexpr int kCount = 100;
    for (int i = 1; i <= kCount; ++i)
    {
        EXPECT_EQ(generator_.new_no(), i);
    }
    EXPECT_EQ(generator_.max_generated_no(), kCount);
}

TEST_F(ItemIdentificationNumberGeneratorTest, MaxGeneratedTracksLatest)
{
    generator_.new_no();
    generator_.new_no();
    generator_.new_no();
    EXPECT_EQ(generator_.max_generated_no(), 3);
}

TEST_F(ItemIdentificationNumberGeneratorTest, FreshGeneratorHasMaxZero)
{
    EXPECT_EQ(generator_.max_generated_no(), 0);
}

TEST_F(ItemIdentificationNumberGeneratorTest, SatisfiesInterfaceContract)
{
    datastructures::IdentificationNumberGenerator& as_interface = generator_;
    EXPECT_EQ(as_interface.new_no(), 1);
    EXPECT_EQ(as_interface.max_generated_no(), 1);
}

} // namespace freerouting::board
