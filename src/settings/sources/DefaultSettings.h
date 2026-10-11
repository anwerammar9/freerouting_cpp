#pragma once

#include <memory>
#include <string>

#include "settings/OptimizerScoringVersion.h"
#include "settings/RouterScoringVersion.h"
#include "settings/RouterSettings.h"
#include "settings/SettingsSource.h"

namespace freerouting::settings::sources {

/**
 * Hardcoded default values for all router settings; the lowest-priority settings source and the
 * base of every merge. Mirrors Java settings/sources/DefaultSettings.java. The DEFAULT_* constants
 * are the single authoritative source of every scoring weight.
 */
class DefaultSettings : public SettingsSource
{
public:
    // ---- Scoring weight defaults ----
    static constexpr float DEFAULT_UNROUTED_NET_PENALTY = 5'000'000.0F;
    static constexpr float DEFAULT_CLEARANCE_VIOLATION_PENALTY = 1'000'000.0F;
    static constexpr float DEFAULT_BEND_PENALTY = 10.0F;
    static constexpr int DEFAULT_VIA_COSTS = 50;
    static constexpr int DEFAULT_PLANE_VIA_COSTS = 5;
    static constexpr int DEFAULT_START_RIPUP_COSTS = 100;
    static constexpr double DEFAULT_PREFERRED_DIRECTION_TRACE_COST = 1.0;
    static constexpr double DEFAULT_UNDESIRED_DIRECTION_TRACE_COST = 1.0;
    static constexpr double DEFAULT_COPPER_TO_EDGE_CLEARANCE_UM = 250.0;
    static constexpr double DEFAULT_HOLE_CLEARANCE_UM = 0.0;
    static constexpr RouterScoringVersion DEFAULT_ROUTER_SCORING_VERSION = RouterScoringVersion::V2_CONTINUOUS;
    static constexpr OptimizerScoringVersion DEFAULT_OPTIMIZER_SCORING_VERSION =
        OptimizerScoringVersion::V2_LOWER_BOUND;
    static constexpr float DEFAULT_ROUTER_UNROUTED_CONNECTION_WEIGHT = 1000.0F;
    static constexpr float DEFAULT_ROUTER_UNROUTED_FREE_FRACTION = 0.5F;
    static constexpr float DEFAULT_ROUTER_UNROUTED_FIRST_HALF_WEIGHT = 1000.0F / 3.0F;
    static constexpr float DEFAULT_ROUTER_UNROUTED_SECOND_HALF_WEIGHT = 2000.0F / 3.0F;
    static constexpr float DEFAULT_ROUTER_CLEARANCE_COUNT_WEIGHT = 25.0F;
    static constexpr float DEFAULT_ROUTER_CLEARANCE_DEPTH_WEIGHT = 300.0F;
    static constexpr float DEFAULT_ROUTER_CLEARANCE_DEPTH_SCALE_UM = 1000.0F;
    static constexpr float DEFAULT_OPTIMIZER_EXCESS_LENGTH_WEIGHT = 1000.0F;
    static constexpr float DEFAULT_OPTIMIZER_EXCESS_VIA_WEIGHT = 2000.0F;
    static constexpr float DEFAULT_OPTIMIZER_EXCESS_BEND_WEIGHT = 500.0F;
    static constexpr float DEFAULT_OPTIMIZER_LENGTH_FLOOR = 1.0F;
    static constexpr float DEFAULT_OPTIMIZER_DIFFICULTY_SCALE_FLOOR = 1.0F;
    static constexpr float DEFAULT_OPTIMIZER_IMPROVEMENT_THRESHOLD = 2.5F;

    /**
     * Java getSettings(): a RouterSettings with every default value. The layer-count-dependent
     * arrays (layers, per-layer trace costs) are intentionally left null; they come from the board.
     */
    std::shared_ptr<RouterSettings> get_settings() override;

    /** The same settings by value. */
    static RouterSettings create_settings();

    std::string get_source_name() const override { return "Default Settings"; }
    int get_priority() const override { return 0; }
};

} // namespace freerouting::settings::sources
