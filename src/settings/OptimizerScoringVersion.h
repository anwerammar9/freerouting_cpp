#pragma once

#include <array>
#include <string_view>
#include <utility>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/** Selects the optimizer-board score formula independently from router scoring. Java OptimizerScoringVersion. */
enum class OptimizerScoringVersion
{
    /** Existing combined score retained for compatibility. */
    V1_LEGACY,
    /** Excess length, vias, and bends relative to stored lower bounds. */
    V2_LOWER_BOUND
};

template <>
struct SettingsEnumNames<OptimizerScoringVersion>
{
    static constexpr std::array<std::pair<OptimizerScoringVersion, std::string_view>, 2> names{{
        {OptimizerScoringVersion::V1_LEGACY, "V1_LEGACY"},
        {OptimizerScoringVersion::V2_LOWER_BOUND, "V2_LOWER_BOUND"},
    }};
};

} // namespace freerouting::settings
