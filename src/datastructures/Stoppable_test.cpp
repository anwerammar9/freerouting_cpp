#include "datastructures/Stoppable.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {

TEST(StoppableTest, InitiallyNotStopped)
{
    Stoppable s;
    EXPECT_FALSE(s.is_stop_requested());
}

TEST(StoppableTest, RequestStopSetsFlag)
{
    Stoppable s;
    s.request_stop();
    EXPECT_TRUE(s.is_stop_requested());
}

TEST(StoppableTest, MultipleRequestsIdempotent)
{
    Stoppable s;
    s.request_stop();
    s.request_stop();
    EXPECT_TRUE(s.is_stop_requested());
}

} // namespace freerouting::datastructures