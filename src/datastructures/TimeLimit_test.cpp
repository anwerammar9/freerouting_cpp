#include "datastructures/TimeLimit.h"

#include <gtest/gtest.h>
#include <thread>

namespace freerouting::datastructures {

TEST(TimeLimitTest, InitiallyNotExceeded)
{
    TimeLimit tl(1000);
    EXPECT_FALSE(tl.limit_exceeded());
    EXPECT_FALSE(tl.is_stop_requested());
}

TEST(TimeLimitTest, LimitExceededAfterDuration)
{
    TimeLimit tl(50);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_TRUE(tl.limit_exceeded());
    EXPECT_TRUE(tl.is_stop_requested());
}

TEST(TimeLimitTest, MultiplyIncreasesLimit)
{
    TimeLimit tl(100);
    tl.multiply(2.0);
    EXPECT_EQ(tl.get_limit(), 200);
    EXPECT_FALSE(tl.limit_exceeded());
}

TEST(TimeLimitTest, MultiplyByZeroOrNegativeNoOp)
{
    TimeLimit tl(100);
    tl.multiply(0.0);
    EXPECT_EQ(tl.get_limit(), 100);
    tl.multiply(-1.0);
    EXPECT_EQ(tl.get_limit(), 100);
}

TEST(TimeLimitTest, SetLimitResetsTimer)
{
    TimeLimit tl(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    EXPECT_TRUE(tl.limit_exceeded());
    tl.set_limit(1000);
    EXPECT_FALSE(tl.limit_exceeded());
}

TEST(TimeLimitTest, InheritsFromStoppable)
{
    TimeLimit tl(1000);
    Stoppable* s = &tl;
    EXPECT_FALSE(s->is_stop_requested());
    s->request_stop();
    EXPECT_TRUE(s->is_stop_requested());
    EXPECT_TRUE(tl.is_stop_requested());
}

} // namespace freerouting::datastructures