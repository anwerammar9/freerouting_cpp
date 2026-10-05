#ifndef AUTOROUTE_AUTOROUTE_CONTROL_H
#define AUTOROUTE_AUTOROUTE_CONTROL_H

#include <optional>
#include <string>
#include <vector>
#include "autoroute/ExpansionCostFactor.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {
class RoutingBoard;
}

namespace freerouting::rules {
class ViaRule;
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

    /**
     * Rebuilds the via info masks, the via radii and the via costs for the
     * input board, via costs and net. Port of Java
     * AutorouteControl.rebuildViaInfo().
     */
    void rebuild_via_info(board::RoutingBoard* p_board, int p_via_costs, int p_net_no);

    void set_layer_count(int p_count);

    bool is_layer_active(int p_layer) const;
    int get_trace_half_width(int p_layer) const;
    double get_via_cost(int p_from_layer, int p_to_layer) const;

    settings::RouterSettings* settings = nullptr;

    /** The horizontal and vertical trace costs on each layer. */
    std::vector<ExpansionCostFactor> trace_costs;

    /** The bend cost on each layer. Port of Java AutorouteControl.bendCosts. */
    std::vector<double> bend_costs;

    bool with_neckdown = false;
    std::vector<bool> layer_active;
    int layer_count = 0;
    std::vector<int> trace_half_width;
    std::vector<int> compensated_trace_half_width;
    std::vector<double> via_radius_arr;
    std::vector<ViaCost> add_via_costs;
    /** The currently used clearance class for traces in the autoroute algorithm. */
    int trace_clearance_class_no = 1;
    /**
     * The possible (partial) vias, which can be used by the autorouter. Not
     * owned by the control, mirroring the Java reference to the via rule of
     * the current net class. Port of Java AutorouteControl.viaRule.
     */
    const rules::ViaRule* via_rule = nullptr;
    bool vias_allowed = true;
    bool attach_smd_allowed = false;
    double min_normal_via_cost = 1.0;
    bool ripup_allowed = false;
    int ripup_costs = 1000;
    int ripup_pass_no = 1;
    bool is_fanout = false;
    /**
     * Name of the pin a fanout route starts at. Used only by the fanout
     * diagnostics of the maze search and the connection inserter.
     * Port of Java AutorouteControl.fanoutStartPinName.
     */
    std::string fanout_start_pin_name;
    /**
     * Center of the pin a fanout route starts at. Only used by the fanout
     * filter of the maze expansion list. Port of Java
     * AutorouteControl.fanoutStartPinCenter.
     */
    std::optional<geometry::planar::FloatPoint> fanout_start_pin_center;
    /**
     * First layer of the pin a fanout route starts at. Only used by the
     * fanout filter of the maze expansion list. Port of Java
     * AutorouteControl.fanoutStartPinLayer.
     */
    int fanout_start_pin_layer = -1;
    /**
     * Minimum physical escape wire length of the fanout pass in micrometres.
     * Vias are not placed closer than this distance from the fanout pin center.
     * The C++ RouterSettings has no fanout settings yet, so the Java default of
     * the maze search (500 um) is used when the value is not engaged.
     * Port of Java MazeSearchEngine `minEscapeLengthMm * 1000`.
     */
    std::optional<double> min_escape_length_um;
    /**
     * Maximum physical escape wire length of the fanout pass in micrometres.
     * The search tree expansion and the via placement of a fanout pass are
     * restricted to this distance. Same default situation as
     * min_escape_length_um (Java default 3000 um).
     * Port of Java MazeSearchEngine `maxEscapeLengthMm * 1000`.
     */
    std::optional<double> max_escape_length_um;
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