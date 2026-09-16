#pragma once

#include <cstdint>

namespace freerouting::core {

struct RouterJobResourceUsage
{
    float cpu_time_used = 0.0f;
    float max_memory_used = 0.0f;
    float peak_memory_used = 0.0f;
    float io_read = 0.0f;
    float io_write = 0.0f;

    void reset()
    {
        cpu_time_used = 0.0f;
        max_memory_used = 0.0f;
        peak_memory_used = 0.0f;
        io_read = 0.0f;
        io_write = 0.0f;
    }
};

} // namespace freerouting::core
