#include "datastructures/ArrayStack.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {

TEST(ArrayStackTest, PushAndPopUsesLifoOrder)
{
    ArrayStack<int> stack(2);

    stack.push(10);
    stack.push(20);

    ASSERT_EQ(stack.pop(), 20);
    ASSERT_EQ(stack.pop(), 10);
    EXPECT_FALSE(stack.pop().has_value());
}

TEST(ArrayStackTest, GrowsBeyondInitialCapacity)
{
    ArrayStack<int> stack(1);

    for (int value = 0; value < 10; ++value)
    {
        stack.push(value);
    }

    for (int value = 9; value >= 0; --value)
    {
        ASSERT_EQ(stack.pop(), value);
    }
}

TEST(ArrayStackTest, ZeroCapacityStackCanGrow)
{
    ArrayStack<int> stack(0);

    stack.push(42);

    ASSERT_EQ(stack.pop(), 42);
    EXPECT_FALSE(stack.pop().has_value());
}

TEST(ArrayStackTest, ResetMakesStackEmpty)
{
    ArrayStack<int> stack(2);
    stack.push(1);
    stack.push(2);

    stack.reset();

    EXPECT_FALSE(stack.pop().has_value());
}

} // namespace freerouting::datastructures
