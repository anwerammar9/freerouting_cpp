#ifndef SETTINGS_ROUTER_SETTINGS_H
#define SETTINGS_ROUTER_SETTINGS_H

#include <string>
#include <vector>
#include <memory>
#include "autoroute/ExpansionCostFactor.h"
#include "settings/RouterScoringSettings.h"

namespace freerouting::settings {

class RouterOptimizerSettings
{
public:
    bool enabled = true;
    int iterations = 3;
};

class RouterSettings
{
public:
    RouterSettings();
    explicit RouterSettings(int p_layer_count);
    RouterSettings(const RouterSettings& other);
    RouterSettings& operator=(const RouterSettings& other);

    void set_layer_count(int p_count);

    bool is_enabled() const { return enabled_; }
    void set_enabled(bool p_value) { enabled_ = p_value; }

    const std::string& get_algorithm() const { return algorithm_; }
    void set_algorithm(const std::string& p_value) { algorithm_ = p_value; }

    int get_max_passes() const { return max_passes_; }
    void set_max_passes(int p_value) { max_passes_ = p_value; }

    bool is_layer_active(int p_layer) const;
    bool get_layer_active(int p_layer) const { return is_layer_active(p_layer); }
    void set_layer_active(int p_layer, bool p_active);

    bool is_vias_allowed() const { return vias_allowed_; }
    bool get_vias_allowed() const { return vias_allowed_; }
    void set_vias_allowed(bool p_value) { vias_allowed_ = p_value; }

    bool is_automatic_neckdown() const { return automatic_neckdown_; }
    bool get_automatic_neckdown() const { return automatic_neckdown_; }
    void set_automatic_neckdown(bool p_value) { automatic_neckdown_ = p_value; }

    int get_max_threads() const { return max_threads_; }
    void set_max_threads(int p_value) { max_threads_ = p_value; }

    /**
     * Returns whether the autorouter should run. Port of Java
     * RouterSettings.getRunRouter(): Java dereferences the nullable
     * AutorouterSettings.enabled and defaults to true.
     */
    bool get_run_router() const { return run_router_; }
    /** Sets whether the autorouter should run. Java RouterSettings.setRunRouter. */
    void set_run_router(bool p_value) { run_router_ = p_value; }

    /**
     * Returns whether the post-routing optimizer should run. Port of Java
     * RouterSettings.getRunOptimizer(): Java dereferences the nullable
     * OptimizerSettings.enabled and defaults to false.
     */
    bool get_run_optimizer() const { return run_optimizer_; }
    /** Sets whether the post-routing optimizer should run. Java RouterSettings.setRunOptimizer. */
    void set_run_optimizer(bool p_value) { run_optimizer_ = p_value; }

    /**
     * Returns the cost assigned to regular vias. Port of Java
     * RouterSettings.getViaCosts(). Java returns 1 when the scoring settings
     * do not carry a via cost; the C++ RouterScoringSettings stores a plain
     * int (default 50), which is returned as the null stand-in.
     */
    int get_via_costs() const { return scoring_ ? scoring_->via_costs : 1; }
    /** Sets the cost assigned to regular vias; the value is at least 1. Java setViaCosts. */
    void set_via_costs(int p_value);

    /**
     * Returns the cost assigned to vias connecting to a plane. Port of Java
     * RouterSettings.getPlaneViaCosts().
     */
    int get_plane_via_costs() const { return scoring_ ? scoring_->plane_via_costs : 1; }
    /** Sets the cost assigned to vias connecting to a plane; the value is at least 1. Java setPlaneViaCosts. */
    void set_plane_via_costs(int p_value);

    /**
     * Returns the starting cost of a ripup operation. Port of Java
     * RouterSettings.getStartRipupCosts(). Java dereferences the nullable
     * Integer startRipupCosts of the scoring settings, so an unset value
     * evaluates to 1; the C++ settings carry a plain int, which defaults to
     * 100.
     */
    int get_start_ripup_costs() const { return scoring_ ? scoring_->start_ripup_costs : 1; }
    /** Sets the starting cost of a ripup operation; the value is at least 1. Java setStartRipupCosts. */
    void set_start_ripup_costs(int p_value);

    /**
     * Returns true, if the preferred routing direction of p_layer is
     * horizontal. Port of Java RouterSettings.getPreferredDirectionIsHorizontal():
     * out-of-range layers yield false and an unset layer defaults to layer % 2 == 1.
     */
    bool get_preferred_direction_is_horizontal(int p_layer) const;
    /** Sets the preferred routing direction of one layer. Java setPreferredDirectionIsHorizontal. */
    void set_preferred_direction_is_horizontal(int p_layer, bool p_value);

    /**
     * Returns the preferred-direction trace cost for one layer. Port of Java
     * RouterSettings.getPreferredDirectionTraceCosts(): out-of-range layers
     * yield 0, a missing scoring array yields 1.0.
     */
    double get_preferred_direction_trace_costs(int p_layer) const;
    /** Sets the preferred-direction trace cost of one layer; the value is at least 0.1. Java setPreferredDirectionTraceCosts. */
    void set_preferred_direction_trace_costs(int p_layer, double p_value);

    /**
     * Returns the against-preferred-direction trace cost for one layer. Port
     * of Java RouterSettings.getAgainstPreferredDirectionTraceCosts().
     */
    double get_against_preferred_direction_trace_costs(int p_layer) const;
    /** Sets the against-preferred-direction trace cost of one layer; the value is at least 0.1. Java setAgainstPreferredDirectionTraceCosts. */
    void set_against_preferred_direction_trace_costs(int p_layer, double p_value);

    /**
     * Returns the trace cost for horizontal movement on one layer. Port of
     * Java RouterSettings.getHorizontalTraceCosts().
     */
    double get_horizontal_trace_costs(int p_layer) const;

    /**
     * Returns the trace cost for vertical movement on one layer. Port of
     * Java RouterSettings.getVerticalTraceCosts().
     */
    double get_vertical_trace_costs(int p_layer) const;

    /**
     * Returns the per-layer horizontal and vertical trace cost factors. Port
     * of Java RouterSettings.getTraceCosts(): an empty array is returned when
     * the scoring settings carry no per-layer preferred-direction costs.
     */
    std::vector<autoroute::ExpansionCostFactor> get_trace_costs() const;

    /**
     * Returns the effective bend cost for one layer. Port of Java
     * RouterSettings.getBendCost(int). The C++ settings do not carry a per
     * layer bend cost yet, so 0.0 is returned for every valid layer (the
     * maze search then adds no bend cost penalty).
     */
    double get_bend_cost(int /*p_layer*/) const { return 0.0; } // Stub

    /**
     * Returns the accuracy of the pull tight algorithm. Port of Java
     * RouterSettings.tracePullTightAccuracy, which is a nullable Integer with
     * no default value; the C++ settings do not carry it yet, so the value
     * Java falls back to for an unset accuracy (500) is used.
     */
    int get_trace_pull_tight_accuracy() const { return trace_pull_tight_accuracy_; }

    void set_trace_pull_tight_accuracy(int p_value) { trace_pull_tight_accuracy_ = p_value; }

    std::string to_string() const { return "RouterSettings"; }

private:
    bool enabled_ = true;
    std::string algorithm_ = "freerouting-router";
    int max_passes_ = 9999;
    std::vector<bool> layer_active_;
    std::vector<bool> preferred_direction_horizontal_;
    bool run_router_ = true;
    bool run_optimizer_ = false;
    bool vias_allowed_ = true;
    bool automatic_neckdown_ = true;
    int max_threads_ = 1;
    int trace_pull_tight_accuracy_ = 500;

    std::unique_ptr<RouterOptimizerSettings> optimizer_;
    std::unique_ptr<RouterScoringSettings> scoring_;
};

} // namespace freerouting::settings

#endif // SETTINGS_ROUTER_SETTINGS_H