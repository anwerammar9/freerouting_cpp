#include "settings/RouterSettings.h"

#include <algorithm>
#include <climits>
#include <thread>

#include "logger/FRLogger.h"

namespace freerouting::settings {

namespace {

void warn(const std::string& p_message)
{
    logger::FRLogger::instance().warn(p_message);
}

std::string layer_range_message(const std::string& p_prefix, int p_layer, int p_layer_count)
{
    return p_prefix + ": layer=" + std::to_string(p_layer) + " out of range [0.."
        + std::to_string(p_layer_count - 1) + "]";
}

} // namespace

RouterSettings::RouterSettings(int p_layer_count)
    : RouterSettings()
{
    set_layer_count(p_layer_count);
}

int RouterSettings::available_processors()
{
    const unsigned int count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : static_cast<int>(count);
}

int RouterSettings::default_max_threads()
{
    return std::max(1, available_processors() - 1);
}

int RouterSettings::normalize_max_threads(std::optional<int> p_value)
{
    const int processors = available_processors();
    if (!p_value)
    {
        return default_max_threads();
    }
    if (*p_value < 0)
    {
        return default_max_threads();
    }
    if (*p_value == 0)
    {
        return processors;
    }
    return std::min(*p_value, processors);
}

void RouterSettings::set_max_threads(std::optional<int> p_value)
{
    max_threads = normalize_max_threads(p_value);
    // Keep the legacy flat knob, the nested autorouter knob, and the optimizer pool in sync when
    // the setMaxThreads path is used; nested CLI flags set those fields directly.
    autorouter.max_threads = max_threads;
    optimizer.max_threads = max_threads;
}

int RouterSettings::get_autorouter_max_threads() const
{
    const std::optional<int> configured = autorouter.max_threads ? autorouter.max_threads : max_threads;
    return normalize_max_threads(configured);
}

double RouterSettings::get_neck_width_um() const
{
    return neck_width_um && *neck_width_um > 0 ? *neck_width_um : 0;
}

int RouterSettings::get_layer_count() const
{
    return layers ? static_cast<int>(layers->size()) : 0;
}

void RouterSettings::set_layer_count(int p_count)
{
    if (!layers || static_cast<int>(layers->size()) != p_count)
    {
        board_specific_trace_costs_applied_ = false;
        layers = std::vector<LayerSettings>(static_cast<std::size_t>(p_count));
    }
    // Initialize per-layer cost arrays with a neutral default so callers can write individual
    // entries without waiting for applyBoardSpecificOptimizations.
    scoring.preferred_direction_trace_cost = std::vector<double>(static_cast<std::size_t>(p_count), 1.0);
    scoring.undesired_direction_trace_cost = std::vector<double>(static_cast<std::size_t>(p_count), 1.0);
    for (auto& layer : *layers)
    {
        layer.routable = true;
        layer.preferred_direction_horizontal.reset();
        layer.bend_cost.reset();
    }
}

RouterSettings RouterSettings::clone() const
{
    RouterSettings result;
    const int layer_count = get_layer_count();
    if (layer_count > 0)
    {
        result.set_layer_count(layer_count);
    }
    result.job_timeout_string = job_timeout_string;
    result.layers = layers;
    result.copper_to_edge_clearance_um = copper_to_edge_clearance_um;
    result.hole_clearance_um = hole_clearance_um;
    result.plane_nets = plane_nets;
    result.plane_as_obstacle = plane_as_obstacle;
    result.neck_width_um = neck_width_um;
    result.strict_drc = strict_drc;
    result.trace_pull_tight_accuracy = trace_pull_tight_accuracy;
    result.vias_allowed = vias_allowed;
    result.automatic_neckdown = automatic_neckdown;
    result.max_threads = max_threads;
    // Java parity: resultJsonPath is not copied by RouterSettings.clone().
    result.optimizer = optimizer;
    result.scoring = scoring;
    result.router_scoring = router_scoring;
    result.optimizer_scoring = optimizer_scoring;
    result.fanout = fanout;
    result.autorouter = autorouter;
    result.board_specific_trace_costs_applied_ = board_specific_trace_costs_applied_;
    return result;
}

int copy_fields(const RouterSettings& p_source, RouterSettings& p_target)
{
    // Java getDeclaredFields order; the private transient boardSpecificTraceCostsApplied is skipped.
    int changed = 0;
    changed += copy_fields(p_source.autorouter, p_target.autorouter);
    changed += copy_fields(p_source.fanout, p_target.fanout);
    changed += copy_boxed_field(p_source.copper_to_edge_clearance_um, p_target.copper_to_edge_clearance_um);
    changed += copy_boxed_field(p_source.hole_clearance_um, p_target.hole_clearance_um);
    changed += copy_array_field(p_source.plane_nets, p_target.plane_nets);
    changed += copy_boxed_field(p_source.plane_as_obstacle, p_target.plane_as_obstacle);
    changed += copy_boxed_field(p_source.neck_width_um, p_target.neck_width_um);
    changed += copy_boxed_field(p_source.strict_drc, p_target.strict_drc);
    changed += copy_boxed_field(p_source.job_timeout_string, p_target.job_timeout_string);
    changed += copy_object_array_field(p_source.layers, p_target.layers,
                                       [](const LayerSettings& p_from, LayerSettings& p_to) {
                                           return copy_fields(p_from, p_to);
                                       });
    changed += copy_boxed_field(p_source.trace_pull_tight_accuracy, p_target.trace_pull_tight_accuracy);
    changed += copy_boxed_field(p_source.vias_allowed, p_target.vias_allowed);
    changed += copy_boxed_field(p_source.automatic_neckdown, p_target.automatic_neckdown);
    changed += copy_fields(p_source.optimizer, p_target.optimizer);
    changed += copy_fields(p_source.scoring, p_target.scoring);
    changed += copy_fields(p_source.router_scoring, p_target.router_scoring);
    changed += copy_fields(p_source.optimizer_scoring, p_target.optimizer_scoring);
    changed += copy_boxed_field(p_source.max_threads, p_target.max_threads);
    changed += copy_boxed_field(p_source.result_json_path, p_target.result_json_path);
    return changed;
}

int RouterSettings::apply_new_values_from(const RouterSettings& p_settings)
{
    return copy_fields(p_settings, *this);
}

void RouterSettings::validate()
{
    // Validate maxPasses (0 means no limit)
    if (autorouter.max_passes)
    {
        if (*autorouter.max_passes < 0 || (*autorouter.max_passes > 9999 && *autorouter.max_passes != INT_MAX))
        {
            warn("Invalid maxPasses value: " + std::to_string(*autorouter.max_passes)
                 + ", using default 0 (no limit)");
            autorouter.max_passes = 0;
        }
    }

    // Validate maxThreads (0 means no limit - handled as max available during normalization)
    const int processors = available_processors();
    if (!max_threads)
    {
        max_threads = default_max_threads();
    }
    else if (*max_threads < 0)
    {
        warn("Invalid maxThreads value: " + std::to_string(*max_threads) + ", using "
             + std::to_string(default_max_threads()));
        max_threads = default_max_threads();
    }
    else if (*max_threads > processors)
    {
        warn("Invalid maxThreads value: " + std::to_string(*max_threads) + ", capping at "
             + std::to_string(processors));
        max_threads = processors;
    }

    // Validate tracePullTightAccuracy.
    // Java parity: Java unboxes the Integer and throws a NullPointerException when it is null;
    // a null value is left untouched here.
    if (trace_pull_tight_accuracy && *trace_pull_tight_accuracy < 1)
    {
        warn("Invalid tracePullTightAccuracy value: " + std::to_string(*trace_pull_tight_accuracy)
             + ", using default 500");
        trace_pull_tight_accuracy = 500;
    }
}

void RouterSettings::set_via_costs(int p_value)
{
    scoring.via_costs = std::max(p_value, 1);
}

void RouterSettings::set_plane_via_costs(int p_value)
{
    scoring.plane_via_costs = std::max(p_value, 1);
}

void RouterSettings::set_start_ripup_costs(int p_value)
{
    scoring.start_ripup_costs = std::max(p_value, 1);
}

bool RouterSettings::is_layer_active(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn(layer_range_message("AutorouteSettings.get_layer_active", p_layer, get_layer_count()));
        return false;
    }
    return (*layers)[p_layer].routable.value_or(true);
}

void RouterSettings::set_layer_active(int p_layer, bool p_active)
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn(layer_range_message("AutorouteSettings.set_layer_active", p_layer, get_layer_count()));
        return;
    }
    (*layers)[p_layer].routable = p_active;
}

void RouterSettings::set_bend_cost(int p_layer, double p_value)
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("RouterSettings.set_bend_cost: layer out of range");
        return;
    }
    (*layers)[p_layer].bend_cost = std::max(MIN_BEND_COST, std::min(MAX_BEND_COST, p_value));
}

double RouterSettings::get_bend_cost(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("RouterSettings.get_bend_cost: layer out of range");
        return 0.0;
    }
    const auto& bend_cost = (*layers)[p_layer].bend_cost;
    if (!bend_cost)
    {
        return scoring.default_bend_cost
            ? std::max(MIN_BEND_COST, std::min(MAX_BEND_COST, *scoring.default_bend_cost))
            : 0.0;
    }
    return *bend_cost;
}

bool RouterSettings::get_preferred_direction_is_horizontal(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn(layer_range_message("AutorouteSettings.get_preferred_direction_is_horizontal", p_layer,
                                 get_layer_count()));
        return false;
    }
    return (*layers)[p_layer].preferred_direction_horizontal.value_or(p_layer % 2 == 1);
}

void RouterSettings::set_preferred_direction_is_horizontal(int p_layer, bool p_value)
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn(layer_range_message("AutorouteSettings.set_preferred_direction_is_horizontal", p_layer,
                                 get_layer_count()));
        return;
    }
    (*layers)[p_layer].preferred_direction_horizontal = p_value;
}

void RouterSettings::set_preferred_direction_trace_costs(int p_layer, double p_value)
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.set_preferred_direction_trace_costs: layer out of range");
        return;
    }
    auto& costs = scoring.preferred_direction_trace_cost;
    if (!costs || static_cast<int>(costs->size()) != get_layer_count())
    {
        // Java parity: the new array is zero-filled (new double[layerCount]).
        costs = std::vector<double>(static_cast<std::size_t>(get_layer_count()), 0.0);
    }
    (*costs)[p_layer] = std::max(p_value, 0.1);
    board_specific_trace_costs_applied_ = true;
}

double RouterSettings::get_preferred_direction_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.get_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    const auto& costs = scoring.preferred_direction_trace_cost;
    if (!costs || p_layer >= static_cast<int>(costs->size()))
    {
        return 1.0;
    }
    return (*costs)[p_layer];
}

void RouterSettings::set_against_preferred_direction_trace_costs(int p_layer, double p_value)
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.set_against_preferred_direction_trace_costs: layer out of range");
        return;
    }
    auto& costs = scoring.undesired_direction_trace_cost;
    if (!costs || static_cast<int>(costs->size()) != get_layer_count())
    {
        // Java parity: the new array is zero-filled (new double[layerCount]).
        costs = std::vector<double>(static_cast<std::size_t>(get_layer_count()), 0.0);
    }
    (*costs)[p_layer] = std::max(p_value, 0.1);
    board_specific_trace_costs_applied_ = true;
}

double RouterSettings::get_against_preferred_direction_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.get_against_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    const auto& costs = scoring.undesired_direction_trace_cost;
    if (!costs || p_layer >= static_cast<int>(costs->size()))
    {
        return 1.0;
    }
    return (*costs)[p_layer];
}

double RouterSettings::get_horizontal_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.get_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    // Java parity: Java indexes the scoring arrays directly (throwing when they are missing); the
    // checked getters return 1.0 instead.
    if (get_preferred_direction_is_horizontal(p_layer))
    {
        return get_preferred_direction_trace_costs(p_layer);
    }
    return get_against_preferred_direction_trace_costs(p_layer);
}

double RouterSettings::get_vertical_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= get_layer_count())
    {
        warn("AutorouteSettings.get_against_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    if (get_preferred_direction_is_horizontal(p_layer))
    {
        return get_against_preferred_direction_trace_costs(p_layer);
    }
    return get_preferred_direction_trace_costs(p_layer);
}

std::vector<autoroute::ExpansionCostFactor> RouterSettings::get_trace_costs() const
{
    if (!scoring.preferred_direction_trace_cost)
    {
        return {};
    }
    std::vector<autoroute::ExpansionCostFactor> result(scoring.preferred_direction_trace_cost->size());
    for (int i = 0; i < static_cast<int>(result.size()); ++i)
    {
        result[i] = autoroute::ExpansionCostFactor(get_horizontal_trace_costs(i), get_vertical_trace_costs(i));
    }
    return result;
}

template <>
const std::vector<SettingsField<RouterSettings>>& settings_fields<RouterSettings>()
{
    using S = RouterSettings;
    static const std::vector<SettingsField<S>> fields = {
        settings_object_field("autorouter", "autorouter", &S::autorouter),
        settings_object_field("fanout", "fanout", &S::fanout),
        settings_leaf_field("copperToEdgeClearanceUm", "copper_to_edge_clearance_um", &S::copper_to_edge_clearance_um),
        settings_leaf_field("holeClearanceUm", "hole_clearance_um", &S::hole_clearance_um),
        settings_leaf_field("planeNets", "plane_nets", &S::plane_nets),
        settings_leaf_field("planeAsObstacle", "plane_as_obstacle", &S::plane_as_obstacle,
                            {"conduction_is_obstacle", "planeAsObstacle", "conductionIsObstacle"}),
        settings_leaf_field("neckWidthUm", "neck_width_um", &S::neck_width_um),
        settings_leaf_field("strictDrc", "strict_drc", &S::strict_drc),
        settings_leaf_field("jobTimeoutString", "job_timeout", &S::job_timeout_string),
        settings_transient(settings_object_array_field("layers", "layers", &S::layers)),
        settings_leaf_field("tracePullTightAccuracy", "trace_pull_tight_accuracy", &S::trace_pull_tight_accuracy,
                            {"tracePullTightAccuracy"}),
        settings_leaf_field("viasAllowed", "allowed_via_types", &S::vias_allowed),
        settings_leaf_field("automaticNeckdown", "automatic_neckdown", &S::automatic_neckdown,
                            {"automaticNeckdown"}),
        settings_object_field("optimizer", "optimizer", &S::optimizer),
        settings_object_field("scoring", "scoring", &S::scoring),
        settings_object_field("routerScoring", "router_scoring", &S::router_scoring),
        settings_object_field("optimizerScoring", "optimizer_scoring", &S::optimizer_scoring),
        settings_leaf_field("maxThreads", "max_threads", &S::max_threads),
        settings_leaf_field("resultJsonPath", "result_json", &S::result_json_path),
    };
    return fields;
}

} // namespace freerouting::settings
