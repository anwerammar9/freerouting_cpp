#include "settings/sources/DefaultSettings.h"

#include <algorithm>
#include <climits>

namespace freerouting::settings::sources {

std::shared_ptr<RouterSettings> DefaultSettings::get_settings()
{
    return std::make_shared<RouterSettings>(create_settings());
}

RouterSettings DefaultSettings::create_settings()
{
    RouterSettings settings;

    settings.autorouter.enabled = true;
    settings.autorouter.algorithm = RouterSettings::ALGORITHM_CURRENT;
    settings.job_timeout_string = "12:00:00";
    settings.autorouter.max_passes = 0;
    settings.autorouter.max_items = INT_MAX;
    settings.trace_pull_tight_accuracy = 500;
    settings.vias_allowed = true;
    settings.automatic_neckdown = true;
    settings.autorouter.save_intermediate_stages = false;
    settings.autorouter.ignore_net_classes = std::vector<std::string>{};
    settings.max_threads = std::max(1, RouterSettings::available_processors() - 1);
    settings.autorouter.max_threads = settings.max_threads;
    settings.copper_to_edge_clearance_um = DEFAULT_COPPER_TO_EDGE_CLEARANCE_UM;
    settings.hole_clearance_um = DEFAULT_HOLE_CLEARANCE_UM;
    settings.plane_nets = std::vector<std::string>{};
    settings.plane_as_obstacle = false;
    settings.neck_width_um = 0.0;
    settings.strict_drc = false;

    // layers is left null intentionally: they are populated from the DSN layer count and then
    // overwritten with board-geometry-aware values by applyBoardSpecificOptimizations().

    // Fanout pre-pass defaults
    settings.fanout.enabled = true;
    settings.fanout.max_passes = 20;
    settings.fanout.max_milliseconds_per_pin = 10000LL;
    settings.fanout.ripup_allowed = true;
    settings.fanout.min_escape_length_mm = 2.5;
    settings.fanout.max_escape_length_mm = 4.5;
    settings.fanout.start_via_diameter_mm = 0.250;
    settings.fanout.end_via_diameter_mm = 0.250;
    settings.fanout.pin_sorting_order = "outer_first";
    settings.fanout.max_items = INT_MAX;
    settings.fanout.fallback_to_board_vias = true;

    // Optimizer defaults
    settings.optimizer.enabled = true;
    settings.optimizer.algorithm = "freerouting-optimizer";
    settings.optimizer.max_passes = 100;
    settings.optimizer.max_items = INT_MAX;
    settings.optimizer.max_threads = std::max(1, RouterSettings::available_processors() - 1);
    settings.optimizer.optimization_improvement_threshold = DEFAULT_OPTIMIZER_IMPROVEMENT_THRESHOLD;
    settings.optimizer.board_update_strategy = autoroute::BoardUpdateStrategy::GLOBAL_OPTIMAL;
    settings.optimizer.item_selection_strategy = autoroute::ItemSelectionStrategy::SEQUENTIAL;
    settings.optimizer.additional_ripup_cost_factor_at_start = 10;
    settings.optimizer.trace_ripup_cost_factor = 0.6F;
    settings.optimizer.max_autoroute_passes = 6;
    settings.optimizer.enable_preflight_guards = true;
    settings.optimizer.max_consecutive_failures = 50;
    settings.optimizer.max_consecutive_failures_pass1 = 12;

    // Scalar trace-cost defaults (the per-layer arrays depend on the board, see above).
    settings.scoring.default_preferred_direction_trace_cost = DEFAULT_PREFERRED_DIRECTION_TRACE_COST;
    settings.scoring.default_undesired_direction_trace_cost = DEFAULT_UNDESIRED_DIRECTION_TRACE_COST;

    settings.scoring.via_costs = DEFAULT_VIA_COSTS;
    settings.scoring.plane_via_costs = DEFAULT_PLANE_VIA_COSTS;
    settings.scoring.start_ripup_costs = DEFAULT_START_RIPUP_COSTS;
    settings.scoring.unrouted_net_penalty = DEFAULT_UNROUTED_NET_PENALTY;
    settings.scoring.clearance_violation_penalty = DEFAULT_CLEARANCE_VIOLATION_PENALTY;
    settings.scoring.bend_penalty = DEFAULT_BEND_PENALTY;
    settings.scoring.default_bend_cost = 0.0;

    settings.router_scoring.version = DEFAULT_ROUTER_SCORING_VERSION;
    settings.router_scoring.unrouted_connection_weight = DEFAULT_ROUTER_UNROUTED_CONNECTION_WEIGHT;
    settings.router_scoring.unrouted_free_fraction = DEFAULT_ROUTER_UNROUTED_FREE_FRACTION;
    settings.router_scoring.unrouted_first_half_weight = DEFAULT_ROUTER_UNROUTED_FIRST_HALF_WEIGHT;
    settings.router_scoring.unrouted_second_half_weight = DEFAULT_ROUTER_UNROUTED_SECOND_HALF_WEIGHT;
    settings.router_scoring.clearance_violation_count_weight = DEFAULT_ROUTER_CLEARANCE_COUNT_WEIGHT;
    settings.router_scoring.clearance_violation_depth_weight = DEFAULT_ROUTER_CLEARANCE_DEPTH_WEIGHT;
    settings.router_scoring.clearance_violation_depth_scale = DEFAULT_ROUTER_CLEARANCE_DEPTH_SCALE_UM;

    settings.optimizer_scoring.version = DEFAULT_OPTIMIZER_SCORING_VERSION;
    settings.optimizer_scoring.excess_wire_length_weight = DEFAULT_OPTIMIZER_EXCESS_LENGTH_WEIGHT;
    settings.optimizer_scoring.excess_via_weight = DEFAULT_OPTIMIZER_EXCESS_VIA_WEIGHT;
    settings.optimizer_scoring.excess_bend_weight = DEFAULT_OPTIMIZER_EXCESS_BEND_WEIGHT;
    settings.optimizer_scoring.length_floor = DEFAULT_OPTIMIZER_LENGTH_FLOOR;
    settings.optimizer_scoring.difficulty_scale_floor = DEFAULT_OPTIMIZER_DIFFICULTY_SCALE_FLOOR;

    return settings;
}

} // namespace freerouting::settings::sources
