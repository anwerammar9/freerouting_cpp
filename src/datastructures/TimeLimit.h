#pragma once

#include <chrono>
#include "datastructures/Stoppable.h"

namespace freerouting {
namespace datastructures {

class TimeLimit : public Stoppable
{
public:
    explicit TimeLimit(int64_t p_milliseconds);

    bool limit_exceeded() const;
    void multiply(double p_factor);

    int64_t get_limit() const { return time_limit_; }
    void set_limit(int64_t p_milliseconds) { time_limit_ = p_milliseconds; }

    bool is_stop_requested() const override;

private:
    int64_t time_limit_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace datastructures
} // namespace freerouting