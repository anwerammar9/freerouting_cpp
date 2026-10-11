#pragma once

#include <array>
#include <string_view>
#include <utility>

#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/** Selects the router-board score formula independently from optimizer scoring. Java RouterScoringVersion. */
enum class RouterScoringVersion
{
    /** Existing combined score retained for compatibility. */
    V1_LEGACY,
    /** Completion plus discrete and continuous clearance penalties. */
    V2_CONTINUOUS
};

template <>
struct SettingsEnumNames<RouterScoringVersion>
{
    static constexpr std::array<std::pair<RouterScoringVersion, std::string_view>, 2> names{{
        {RouterScoringVersion::V1_LEGACY, "V1_LEGACY"},
        {RouterScoringVersion::V2_CONTINUOUS, "V2_CONTINUOUS"},
    }};
};

} // namespace freerouting::settings
