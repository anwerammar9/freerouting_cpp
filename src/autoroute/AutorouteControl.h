#ifndef AUTOROUTE_AUTOROUTE_CONTROL_H
#define AUTOROUTE_AUTOROUTE_CONTROL_H

#include <vector>
#include <string>
#include "autoroute/ExpansionCostFactor.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::settings {
class RouterSettings;
}

namespace freerouting::autoroute {

struct ViaCost {
    std::vector<int> to_layer;
    ViaCost(int p_layer_count = 0) : to_layer(p_layer_count, 0) {}
};

struct ViaMask {
    int from_layer;
    int to_layer;
    bool attach_smd_allowed;

    ViaMask(int p_from_layer = 0, int p_to_layer = 0, bool p_attach_smd_allowed = false)
        : from_layer(p_from_layer), to_layer(p_to_layer), attach_smd_allowed(p_attach_smd_allowed) {}
};

/**
 * Structure for controlling the autoroute algorithm.
 */
class AutorouteControl {
public:
    AutorouteControl();
    AutorouteControl(board::RoutingBoard* p_board, int p_net_no, settings::RouterSettings* p_settings);
    ~AutorouteControl();

    void init_net(int p_net_no, board::RoutingBoard* p_board, int p_via_costs);

    void set_layer_count(int p_count);

    bool is_layer_active(int p_layer) const;
    int get_trace_half_width(int p_layer) const;
    double get_via_cost(int p_from_layer, int p_to_layer) const;

    settings::RouterSettings* settings = nullptr;
    std::vector<ExpansionCostFactor> trace_costs;
    bool with_neckdown = false;
    std::vector<bool> layer_active;
    int layer_count = 0;
    std::vector<int> trace_half_width;
    std::vector<int> compensated_trace_half_width;
    std::vector<double> via_radius_arr;
    std::vector<ViaCost> add_via_costs;
    int trace_clearance_class_no = 1;
    bool vias_allowed = true;
    bool attach_smd_allowed = false;
    double min_normal_via_cost = 1.0;
    bool ripup_allowed = false;
    int ripup_costs = 1000;
    int ripup_pass_no = 1;
    bool is_fanout = false;
    bool remove_unconnected_vias = true;
    int net_no = 0;
    int active_layer = 0;
    int via_clearance_class = 1;
    std::vector<ViaMask> via_info_arr;
    int via_lower_bound = 0;
    int via_upper_bound = 0;
    double max_via_radius = 0;
    int tidy_region_width = 1000000; // Simplified Integer.MAX_VALUE
    int pull_tight_accuracy = 500;
    int max_shove_trace_recursion_depth = 20;
    int max_shove_via_recursion_depth = 5;
    int max_spring_over_recursion_depth = 5;
    double min_cheap_via_cost = 0.8;

    std::string to_string() const { return "AutorouteControl"; }
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_AUTOROUTE_CONTROL_H