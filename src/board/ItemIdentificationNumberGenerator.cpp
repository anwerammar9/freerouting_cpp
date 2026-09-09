#include "board/ItemIdentificationNumberGenerator.h"

#include "logger/FRLogger.h"

namespace freerouting::board {

int ItemIdentificationNumberGenerator::new_no()
{
    if (last_generated_id_no_ >= kMaxIdNo)
    {
        FR_LOG_WARN("IdNoGenerator: danger of overflow, please regenerate id numbers from scratch!");
    }
    ++last_generated_id_no_;
    return last_generated_id_no_;
}

} // namespace freerouting::board
