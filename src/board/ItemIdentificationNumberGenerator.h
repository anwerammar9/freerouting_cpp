#pragma once

#include <climits>

#include "datastructures/IdentificationNumberGenerator.h"

namespace freerouting::board {

/**
 * @brief Creates unique identification numbers for items on the board.
 *
 * Faithful port of app.freerouting.board.ItemIdentificationNumberGenerator.
 * Overflow protection: warns once the generated id approaches c_max_id_no.
 */
class ItemIdentificationNumberGenerator : public datastructures::IdentificationNumberGenerator
{
public:
    ItemIdentificationNumberGenerator() = default;

    /**
     * @brief Creates a new unique identification number.
     *
     * @return The new id number (starts at 1).
     */
    int new_no() override;

    /**
     * @brief Returns the maximum generated id number so far.
     */
    int max_generated_no() const override { return last_generated_id_no_; }

private:
    static constexpr int kMaxIdNo = INT_MAX / 2;

    int last_generated_id_no_ = 0;
};

} // namespace freerouting::board
