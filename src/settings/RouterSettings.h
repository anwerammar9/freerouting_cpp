#ifndef SETTINGS_ROUTER_SETTINGS_H
#define SETTINGS_ROUTER_SETTINGS_H

#include <optional>
#include <string>
#include <vector>

#include "autoroute/ExpansionCostFactor.h"
#include "settings/AutorouterSettings.h"
#include "settings/FanoutSettings.h"
#include "settings/LayerSettings.h"
#include "settings/OptimizerScoreSettings.h"
#include "settings/OptimizerSettings.h"
#include "settings/RouterScoreSettings.h"
#include "settings/RoutingCostSettings.h"
#include "settings/SettingsFieldTable.h"

namespace freerouting::settings {

/**
 * Mutable router configuration assembled from the configured settings sources. Mirrors Java
 * settings/RouterSettings.java: every public field is nullable (std::optional) and null means
 * "not set by this source"; SettingsMerger merges sources with apply_new_values_from. The nested
 * settings objects are never null in Java (the constructor creates them) and are held by value.
 * The GUI PropertyChangeSupport is not ported.
 */
class RouterSettings
{
public:
    // Current algorithm identifier and legacy compatibility token.
    static constexpr const char* ALGORITHM_CURRENT = "freerouting-router";
    static constexpr const char* ALGORITHM_V19 = "freerouting-router-v19";
    static constexpr double MIN_BEND_COST = 0.0;
    static constexpr double MAX_BEND_COST = 9.9;

    // ---- Java public fields, in Java declaration order ----
    /** JSON "autorouter". */
    AutorouterSettings autorouter;
    /** Configuration for the SMD-pin fanout pre-pass. JSON "fanout". */
    FanoutSettings fanout;
    /** JSON "copper_to_edge_clearance_um". */
    std::optional<double> copper_to_edge_clearance_um;
    /** JSON "hole_clearance_um". */
    std::optional<double> hole_clearance_um;
    /** Net names treated as power-plane nets. JSON "plane_nets". */
    std::optional<std::vector<std::string>> plane_nets;
    /** Conduction areas block foreign traces. JSON "plane_as_obstacle" (+ 3 alternates). */
    std::optional<bool> plane_as_obstacle;
    /** Opt-in width necking in micrometers; 0/absent = off. JSON "neck_width_um". */
    std::optional<double> neck_width_um;
    /** Rip up connections that introduce clearance violations. JSON "strict_drc". */
    std::optional<bool> strict_drc;
    /** JSON "job_timeout". */
    std::optional<std::string> job_timeout_string;
    /** Per-layer settings. Java transient. JSON "layers". */
    std::optional<std::vector<LayerSettings>> layers;
    /** The accuracy of the pull tight algorithm. JSON "trace_pull_tight_accuracy". */
    std::optional<int> trace_pull_tight_accuracy;
    /** JSON "allowed_via_types". */
    std::optional<bool> vias_allowed;
    /** Lower the trace width at static pins automatically. JSON "automatic_neckdown". */
    std::optional<bool> automatic_neckdown;
    /** JSON "optimizer". */
    OptimizerSettings optimizer;
    /** JSON "scoring". */
    RoutingCostSettings scoring;
    /** JSON "router_scoring". */
    RouterScoreSettings router_scoring;
    /** JSON "optimizer_scoring". */
    OptimizerScoreSettings optimizer_scoring;
    /** JSON "max_threads". */
    std::optional<int> max_threads;
    /** Optional path of a machine-readable routing result manifest. JSON "result_json". */
    std::optional<std::string> result_json_path;

    /** Java RouterSettings(): all fields null, nested settings objects empty. */
    RouterSettings() = default;

    /**
     * C++ convenience: RouterSettings() followed by set_layer_count(p_layer_count). Java builds the
     * equivalent with `new RouterSettings()` + `setLayerCount(n)` (the RouterSettings(RoutingBoard)
     * constructor additionally applies the board-specific optimizations, T307).
     */
    explicit RouterSettings(int p_layer_count);

    /**
     * Java RouterSettings.clone(). Java parity: clone() does not copy resultJsonPath, so the
     * clone's result_json_path is null. The C++ copy constructor copies every field.
     */
    RouterSettings clone() const;

    /** Java applyNewValuesFrom: copies the explicitly set (non-null) values of p_settings. */
    int apply_new_values_from(const RouterSettings& p_settings);

    /** Java validate(): normalizes autorouter.max_passes, max_threads and trace_pull_tight_accuracy. */
    void validate();

    /** Java defaultMaxThreads(): max(1, available processors - 1). */
    static int default_max_threads();
    /** Java normalizeMaxThreads(Integer). */
    static int normalize_max_threads(std::optional<int> p_value);
    /** Java Runtime.availableProcessors() (at least 1). */
    static int available_processors();

    // ---- layer count ----
    /** Java getLayerCount(): the length of the layers array, 0 if null. */
    int get_layer_count() const;
    /** Java setLayerCount: (re)creates the layers and resets the per-layer cost arrays to 1.0. */
    void set_layer_count(int p_count);

    /** Java areBoardSpecificTraceCostsApplied(). */
    bool are_board_specific_trace_costs_applied() const { return board_specific_trace_costs_applied_.value_or(false); }

    // ---- autorouter ----
    /** Java setMaxPasses. */
    void set_max_passes(std::optional<int> p_value) { autorouter.max_passes = p_value; }
    /** C++ accessor of autorouter.maxPasses (Java reads the field); null reads as 0 (= no limit). */
    int get_max_passes() const { return autorouter.max_passes.value_or(0); }

    /** Java setMaxThreads: normalizes the value and syncs autorouter/optimizer max_threads. */
    void set_max_threads(std::optional<int> p_value);
    /** C++ accessor of the flat maxThreads field (Java reads the field); null reads as default_max_threads(). */
    int get_max_threads() const { return max_threads.value_or(default_max_threads()); }
    /** Java getAutorouterMaxThreads(). */
    int get_autorouter_max_threads() const;

    /** Java setJobTimeoutString. */
    void set_job_timeout_string(std::optional<std::string> p_value) { job_timeout_string = std::move(p_value); }

    /** Java setEnabled (autorouter.enabled). */
    void set_enabled(std::optional<bool> p_value) { autorouter.enabled = p_value; }
    /** C++ accessor equal to Java getRunRouter(). */
    bool is_enabled() const { return get_run_router(); }

    /** Java setAlgorithm (autorouter.algorithm). */
    void set_algorithm(std::optional<std::string> p_value) { autorouter.algorithm = std::move(p_value); }
    /** C++ accessor of autorouter.algorithm; null reads as "". */
    std::string get_algorithm() const { return autorouter.algorithm.value_or(""); }

    /** Java getRunRouter(): autorouter.enabled, true when null. */
    bool get_run_router() const { return autorouter.enabled.value_or(true); }
    /** Java setRunRouter. */
    void set_run_router(bool p_value) { autorouter.enabled = p_value; }

    // ---- optimizer / fanout ----
    /** Java getRunOptimizer(): optimizer.enabled, false when null. */
    bool get_run_optimizer() const { return optimizer.enabled.value_or(false); }
    /** Java setRunOptimizer. */
    void set_run_optimizer(bool p_value) { optimizer.enabled = p_value; }
    /** Java setOptimizerEnabled. */
    void set_optimizer_enabled(std::optional<bool> p_value) { optimizer.enabled = p_value; }

    /** Java getRunFanout(): fanout.enabled, true when null. */
    bool get_run_fanout() const { return fanout.enabled.value_or(true); }
    /** Java isFanoutEnabled(): fanout.enabled, false when null. */
    bool is_fanout_enabled() const { return fanout.enabled.value_or(false); }
    /** Java setFanoutEnabled. */
    void set_fanout_enabled(std::optional<bool> p_value) { fanout.enabled = p_value; }

    // ---- scalar flags ----
    /** Java getNeckWidthUm(): the necking width, 0 when null or not positive. */
    double get_neck_width_um() const;
    /** Java isStrictDrc(). */
    bool is_strict_drc() const { return strict_drc.value_or(false); }

    /** Java getViasAllowed(): true when null. */
    bool get_vias_allowed() const { return vias_allowed.value_or(true); }
    bool is_vias_allowed() const { return get_vias_allowed(); }
    /** Java setViasAllowed. */
    void set_vias_allowed(std::optional<bool> p_value) { vias_allowed = p_value; }

    /** Java getAutomaticNeckdown(): false when null. */
    bool get_automatic_neckdown() const { return automatic_neckdown.value_or(false); }
    bool is_automatic_neckdown() const { return get_automatic_neckdown(); }
    /** Java setAutomaticNeckdown. */
    void set_automatic_neckdown(bool p_value) { automatic_neckdown = p_value; }

    /**
     * Java reads the Integer field tracePullTightAccuracy directly (unboxing it, which throws for
     * null). Java parity: a null value reads as 500, the DefaultSettings / validate() value.
     */
    int get_trace_pull_tight_accuracy() const { return trace_pull_tight_accuracy.value_or(500); }
    void set_trace_pull_tight_accuracy(int p_value) { trace_pull_tight_accuracy = p_value; }

    /** Java getPlaneNets(): a copy, empty when null. */
    std::vector<std::string> get_plane_nets() const { return plane_nets.value_or(std::vector<std::string>{}); }
    /** Java setPlaneNets. */
    void set_plane_nets(std::optional<std::vector<std::string>> p_value) { plane_nets = std::move(p_value); }
    /** Java getPlaneAsObstacle(): the nullable value. */
    std::optional<bool> get_plane_as_obstacle() const { return plane_as_obstacle; }
    /** Java isPlaneAsObstacle(). */
    bool is_plane_as_obstacle() const { return plane_as_obstacle.value_or(false); }
    /** Java setPlaneAsObstacle. */
    void set_plane_as_obstacle(std::optional<bool> p_value) { plane_as_obstacle = p_value; }

    // ---- costs ----
    /** Java getViaCosts(): 1 when scoring.viaCosts is null. */
    int get_via_costs() const { return scoring.via_costs.value_or(1); }
    /** Java setViaCosts: at least 1. */
    void set_via_costs(int p_value);
    /** Java getPlaneViaCosts(): 1 when null. */
    int get_plane_via_costs() const { return scoring.plane_via_costs.value_or(1); }
    /** Java setPlaneViaCosts: at least 1. */
    void set_plane_via_costs(int p_value);
    /** Java getStartRipupCosts(): 1 when null. */
    int get_start_ripup_costs() const { return scoring.start_ripup_costs.value_or(1); }
    /** Java setStartRipupCosts: at least 1. */
    void set_start_ripup_costs(int p_value);

    // ---- per layer ----
    /** Java getLayerActive(int): out of range -> false (with a warning), null routable -> true. */
    bool is_layer_active(int p_layer) const;
    bool get_layer_active(int p_layer) const { return is_layer_active(p_layer); }
    /** Java setLayerActive(int, boolean). */
    void set_layer_active(int p_layer, bool p_active);

    /** Java setBendCost(int, double): clamped to [MIN_BEND_COST, MAX_BEND_COST]. */
    void set_bend_cost(int p_layer, double p_value);
    /** Java getBendCost(int): the layer value, else the clamped scoring.default_bend_cost, else 0. */
    double get_bend_cost(int p_layer) const;

    /** Java getPreferredDirectionIsHorizontal(int): out of range -> false, null -> layer % 2 == 1. */
    bool get_preferred_direction_is_horizontal(int p_layer) const;
    /** Java setPreferredDirectionIsHorizontal(int, boolean). */
    void set_preferred_direction_is_horizontal(int p_layer, bool p_value);

    /** Java getPreferredDirectionTraceCosts(int): out of range -> 0, missing cost -> 1.0. */
    double get_preferred_direction_trace_costs(int p_layer) const;
    /** Java setPreferredDirectionTraceCosts(int, double): at least 0.1. */
    void set_preferred_direction_trace_costs(int p_layer, double p_value);

    /** Java getAgainstPreferredDirectionTraceCosts(int). */
    double get_against_preferred_direction_trace_costs(int p_layer) const;
    /** Java setAgainstPreferredDirectionTraceCosts(int, double): at least 0.1. */
    void set_against_preferred_direction_trace_costs(int p_layer, double p_value);

    /** Java getHorizontalTraceCosts(int). */
    double get_horizontal_trace_costs(int p_layer) const;
    /** Java getVerticalTraceCosts(int). */
    double get_vertical_trace_costs(int p_layer) const;

    /** Java getTraceCosts(): empty when scoring.preferred_direction_trace_cost is null. */
    std::vector<autoroute::ExpansionCostFactor> get_trace_costs() const;

    std::string to_string() const { return "RouterSettings"; }

    /** Java ReflectionUtil.copyFields(source, target) for RouterSettings. */
    friend int copy_fields(const RouterSettings& p_source, RouterSettings& p_target);

private:
    /**
     * Java private transient boardSpecificTraceCostsApplied: true once the per-layer trace costs
     * were initialized from the board (or set explicitly) and must not be overwritten.
     */
    std::optional<bool> board_specific_trace_costs_applied_;
};

int copy_fields(const RouterSettings& p_source, RouterSettings& p_target);

template <>
const std::vector<SettingsField<RouterSettings>>& settings_fields<RouterSettings>();

} // namespace freerouting::settings

#endif // SETTINGS_ROUTER_SETTINGS_H
