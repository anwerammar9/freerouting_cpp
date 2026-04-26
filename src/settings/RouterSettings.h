#ifndef SETTINGS_ROUTER_SETTINGS_H
#define SETTINGS_ROUTER_SETTINGS_H

#include <string>
#include <vector>
#include <memory>
#include "autoroute/ExpansionCostFactor.h"

namespace freerouting::settings {

class RouterOptimizerSettings
{
public:
    bool enabled = true;
    int iterations = 3;
};

class RouterScoringSettings
{
public:
    double via_cost = 1.0;
    double corner_cost = 1.0;
    double length_cost = 0.01;
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

    int get_via_costs() const { return 100; } // Stub
    std::vector<autoroute::ExpansionCostFactor> get_trace_cost_arr() const { return {}; } // Stub

    std::string to_string() const { return "RouterSettings"; }

private:
    bool enabled_ = true;
    std::string algorithm_ = "freerouting-router";
    int max_passes_ = 9999;
    std::vector<bool> layer_active_;
    std::vector<bool> preferred_direction_horizontal_;
    bool vias_allowed_ = true;
    bool automatic_neckdown_ = true;
    int max_threads_ = 1;

    std::unique_ptr<RouterOptimizerSettings> optimizer_;
    std::unique_ptr<RouterScoringSettings> scoring_;
};

} // namespace freerouting::settings

#endif // SETTINGS_ROUTER_SETTINGS_H