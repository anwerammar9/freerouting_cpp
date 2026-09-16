#pragma once

#include <cstdint>
#include <string>

namespace freerouting::core {

struct RouterCounters
{
    int32_t pass_count = 0;
    int32_t queued_to_be_routed_count = 0;
    int32_t routed_count = 0;
    int32_t skipped_count = 0;
    int32_t ripped_count = 0;
    int32_t failed_to_be_routed_count = 0;
    int32_t incomplete_count = 0;

    void reset()
    {
        pass_count = 0;
        queued_to_be_routed_count = 0;
        routed_count = 0;
        skipped_count = 0;
        ripped_count = 0;
        failed_to_be_routed_count = 0;
        incomplete_count = 0;
    }
};

} // namespace freerouting::core
