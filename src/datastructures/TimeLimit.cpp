#include "datastructures/TimeLimit.h"

namespace freerouting {
namespace datastructures {

TimeLimit::TimeLimit(int64_t p_milliseconds)
    : time_limit_(p_milliseconds)
    , start_time_(std::chrono::steady_clock::now())
{
}

bool TimeLimit::limit_exceeded() const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return elapsed.count() >= time_limit_;
}

void TimeLimit::multiply(double p_factor)
{
    if (p_factor <= 0.0)
    {
        return;
    }
    time_limit_ = static_cast<int64_t>(time_limit_ * p_factor);
}

bool TimeLimit::is_stop_requested() const
{
    if (stopped_)
    {
        return true;
    }
    return limit_exceeded();
}

} // namespace datastructures
} // namespace freerouting