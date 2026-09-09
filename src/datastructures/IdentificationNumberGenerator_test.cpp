#include "datastructures/IdentificationNumberGenerator.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {

TEST(IdentificationNumberGeneratorTest, StartsAtOne)
{
    IdentificationNumberGeneratorImpl generator;

    EXPECT_EQ(generator.max_generated_no(), 0);
    EXPECT_EQ(generator.new_no(), 1);
    EXPECT_EQ(generator.max_generated_no(), 1);
}

TEST(IdentificationNumberGeneratorTest, GeneratesSequentialNumbers)
{
    IdentificationNumberGeneratorImpl generator;

    for (int expected = 1; expected <= 100; ++expected)
    {
        EXPECT_EQ(generator.new_no(), expected);
    }
    EXPECT_EQ(generator.max_generated_no(), 100);
}

TEST(IdentificationNumberGeneratorTest, SatisfiesInterfaceContract)
{
    IdentificationNumberGeneratorImpl implementation;
    IdentificationNumberGenerator& generator = implementation;

    EXPECT_EQ(generator.new_no(), 1);
    EXPECT_EQ(generator.max_generated_no(), 1);
}

} // namespace freerouting::datastructures
