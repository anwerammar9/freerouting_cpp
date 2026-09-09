#pragma once

#include "autoroute/AutorouteControl.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Point.h"
#include <vector>
#include <optional>

namespace freerouting::autoroute {

class DestinationDistance
{
public:
    DestinationDistance(const std::vector<ExpansionCostFactor>& p_trace_costs,
                        const std::vector<bool>& p_layer_active,
                        double p_min_normal_via_cost,
                        double p_min_cheap_via_cost);

    void join(const geometry::planar::IntBox& p_box, int p_layer);

    double calculate(const geometry::planar::FloatPoint& p_point, int p_layer) const;
    double calculate(const geometry::planar::IntBox& p_box, int p_layer) const;
    double calculate_cheap_distance(const geometry::planar::IntBox& p_box, int p_layer);

private:
    std::vector<ExpansionCostFactor> trace_costs_;
    std::vector<bool> layer_active_;
    int layer_count_ = 0;
    int active_layer_count_ = 0;
    double min_cheap_via_cost_ = 0.0;
    mutable double min_normal_via_cost_ = 0.0; // Mutable for calculate_cheap_distance temp swap

    double min_component_side_trace_cost_ = 0.0;
    double max_component_side_trace_cost_ = 0.0;
    double min_solder_side_trace_cost_ = 0.0;
    double max_solder_side_trace_cost_ = 0.0;
    double max_inner_side_trace_cost_ = 0.0;
    
    double min_component_inner_trace_cost_ = 0.0;
    double min_solder_inner_trace_cost_ = 0.0;
    double min_component_solder_inner_trace_cost_ = 0.0;

    std::optional<geometry::planar::IntBox> component_side_box_;
    std::optional<geometry::planar::IntBox> solder_side_box_;
    std::optional<geometry::planar::IntBox> inner_side_box_;

    bool box_is_empty_ = true;
    bool component_side_box_is_empty_ = true;
    bool solder_side_box_is_empty_ = true;
    bool inner_side_box_is_empty_ = true;
};

} // namespace freerouting::autoroute
