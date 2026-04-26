#include "autoroute/DestinationDistance.h"
#include <limits>
#include <algorithm>

namespace freerouting::autoroute {

DestinationDistance::DestinationDistance(const std::vector<ExpansionCostFactor>& p_trace_costs,
                                         const std::vector<bool>& p_layer_active,
                                         double p_min_normal_via_cost,
                                         double p_min_cheap_via_cost)
    : trace_costs_(p_trace_costs),
      layer_active_(p_layer_active),
      min_cheap_via_cost_(p_min_cheap_via_cost),
      min_normal_via_cost_(p_min_normal_via_cost)
{
    layer_count_ = static_cast<int>(layer_active_.size());
    active_layer_count_ = 0;
    for (int i = 0; i < layer_count_; ++i)
    {
        if (layer_active_[i])
        {
            active_layer_count_++;
        }
    }

    if (layer_count_ > 0 && layer_active_[0])
    {
        if (trace_costs_[0].horizontal < trace_costs_[0].vertical)
        {
            min_component_side_trace_cost_ = trace_costs_[0].horizontal;
            max_component_side_trace_cost_ = trace_costs_[0].vertical;
        }
        else
        {
            min_component_side_trace_cost_ = trace_costs_[0].vertical;
            max_component_side_trace_cost_ = trace_costs_[0].horizontal;
        }
    }

    if (layer_count_ > 0 && layer_active_[layer_count_ - 1])
    {
        const auto& curr_trace_cost = trace_costs_[layer_count_ - 1];
        if (curr_trace_cost.horizontal < curr_trace_cost.vertical)
        {
            min_solder_side_trace_cost_ = curr_trace_cost.horizontal;
            max_solder_side_trace_cost_ = curr_trace_cost.vertical;
        }
        else
        {
            min_solder_side_trace_cost_ = curr_trace_cost.vertical;
            max_solder_side_trace_cost_ = curr_trace_cost.horizontal;
        }
    }

    max_inner_side_trace_cost_ = std::min(max_component_side_trace_cost_, max_solder_side_trace_cost_);
    for (int i = 1; i < layer_count_ - 1; ++i)
    {
        if (!layer_active_[i])
            continue;
        
        double curr_max_cost = std::max(trace_costs_[i].horizontal, trace_costs_[i].vertical);
        max_inner_side_trace_cost_ = std::min(max_inner_side_trace_cost_, curr_max_cost);
    }

    min_component_inner_trace_cost_ = std::min(min_component_side_trace_cost_, max_inner_side_trace_cost_);
    min_solder_inner_trace_cost_ = std::min(min_solder_side_trace_cost_, max_inner_side_trace_cost_);
    min_component_solder_inner_trace_cost_ = std::min(min_component_inner_trace_cost_, min_solder_inner_trace_cost_);
}

void DestinationDistance::join(const geometry::planar::IntBox& p_box, int p_layer)
{
    if (p_layer == 0)
    {
        if (component_side_box_) component_side_box_.emplace(component_side_box_->union_with(p_box));
        else component_side_box_.emplace(p_box);
        component_side_box_is_empty_ = false;
    }
    else if (p_layer == layer_count_ - 1)
    {
        if (solder_side_box_) solder_side_box_.emplace(solder_side_box_->union_with(p_box));
        else solder_side_box_.emplace(p_box);
        solder_side_box_is_empty_ = false;
    }
    else
    {
        if (inner_side_box_) inner_side_box_.emplace(inner_side_box_->union_with(p_box));
        else inner_side_box_.emplace(p_box);
        inner_side_box_is_empty_ = false;
    }
    box_is_empty_ = false;
}

double DestinationDistance::calculate(const geometry::planar::FloatPoint& p_point, int p_layer) const
{
    return calculate(p_point.surrounding_box(), p_layer);
}

static double box_weighted_distance(const geometry::planar::IntBox& box1, const geometry::planar::IntBox& box2, double h_cost, double v_cost) {
    double dx = 0;
    if (box1.ll.x > box2.ur.x) dx = box1.ll.x - box2.ur.x;
    else if (box1.ur.x < box2.ll.x) dx = box2.ll.x - box1.ur.x;

    double dy = 0;
    if (box1.ll.y > box2.ur.y) dy = box1.ll.y - box2.ur.y;
    else if (box1.ur.y < box2.ll.y) dy = box2.ll.y - box1.ur.y;

    return dx * h_cost + dy * v_cost;
}

double DestinationDistance::calculate(const geometry::planar::IntBox& p_box, int p_layer) const
{
    if (box_is_empty_)
        return std::numeric_limits<double>::max();

    auto calc_deltas = [](const geometry::planar::IntBox& box1, const geometry::planar::IntBox& box2) {
        double delta_x = 0;
        if (box1.ll.x > box2.ur.x) delta_x = box1.ll.x - box2.ur.x;
        else if (box1.ur.x < box2.ll.x) delta_x = box2.ll.x - box1.ur.x;

        double delta_y = 0;
        if (box1.ll.y > box2.ur.y) delta_y = box1.ll.y - box2.ur.y;
        else if (box1.ur.y < box2.ll.y) delta_y = box2.ll.y - box1.ur.y;

        return std::make_pair(delta_x, delta_y);
    };

    auto [comp_dx, comp_dy] = component_side_box_ ? calc_deltas(p_box, *component_side_box_) : std::make_pair(0.0, 0.0);
    double comp_max_d = std::max(comp_dx, comp_dy);
    double comp_min_d = std::min(comp_dx, comp_dy);

    auto [sold_dx, sold_dy] = solder_side_box_ ? calc_deltas(p_box, *solder_side_box_) : std::make_pair(0.0, 0.0);
    double sold_max_d = std::max(sold_dx, sold_dy);
    double sold_min_d = std::min(sold_dx, sold_dy);

    auto [inner_dx, inner_dy] = inner_side_box_ ? calc_deltas(p_box, *inner_side_box_) : std::make_pair(0.0, 0.0);
    double inner_max_d = std::max(inner_dx, inner_dy);
    double inner_min_d = std::min(inner_dx, inner_dy);

    double result = std::numeric_limits<double>::max();

    if (p_layer == 0)
    {
        if (!component_side_box_is_empty_ && component_side_box_)
            result = box_weighted_distance(p_box, *component_side_box_, trace_costs_[0].horizontal, trace_costs_[0].vertical);

        if (active_layer_count_ <= 1) return result;

        double tmp = min_solder_side_trace_cost_ < min_component_side_trace_cost_ ?
            min_solder_side_trace_cost_ * sold_max_d + min_component_side_trace_cost_ * sold_min_d + min_normal_via_cost_ :
            min_component_side_trace_cost_ * sold_max_d + min_solder_side_trace_cost_ * sold_min_d + min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = comp_max_d + comp_min_d * min_component_inner_trace_cost_ + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        if (active_layer_count_ == 2) return result;

        tmp = inner_max_d + inner_min_d * min_component_inner_trace_cost_ + min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = sold_max_d + min_component_solder_inner_trace_cost_ * sold_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = comp_max_d + comp_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        if (active_layer_count_ == 3) return result;

        tmp = inner_max_d + inner_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = sold_max_d + sold_min_d + 3 * min_normal_via_cost_;
        result = std::min(result, tmp);

        return result;
    }

    if (p_layer == layer_count_ - 1)
    {
        if (!solder_side_box_is_empty_ && solder_side_box_)
            result = box_weighted_distance(p_box, *solder_side_box_, trace_costs_[p_layer].horizontal, trace_costs_[p_layer].vertical);

        double tmp = min_component_side_trace_cost_ < min_solder_side_trace_cost_ ?
            min_component_side_trace_cost_ * comp_max_d + min_solder_side_trace_cost_ * comp_min_d + min_normal_via_cost_ :
            min_solder_side_trace_cost_ * comp_max_d + min_component_side_trace_cost_ * comp_min_d + min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = sold_max_d + sold_min_d * min_solder_inner_trace_cost_ + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        if (active_layer_count_ <= 2) return result;

        tmp = inner_min_d * min_solder_inner_trace_cost_ + inner_max_d + min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = comp_max_d + min_component_solder_inner_trace_cost_ * comp_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = sold_max_d + sold_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        if (active_layer_count_ == 3) return result;

        tmp = inner_max_d + inner_min_d + 2 * min_normal_via_cost_;
        result = std::min(result, tmp);

        tmp = comp_max_d + comp_min_d + 3 * min_normal_via_cost_;
        result = std::min(result, tmp);

        return result;
    }

    if (!inner_side_box_is_empty_ && inner_side_box_)
        result = box_weighted_distance(p_box, *inner_side_box_, trace_costs_[p_layer].horizontal, trace_costs_[p_layer].vertical);

    double tmp = inner_max_d + inner_min_d + min_normal_via_cost_;
    result = std::min(result, tmp);

    tmp = comp_max_d + comp_min_d * min_component_inner_trace_cost_ + min_normal_via_cost_;
    result = std::min(result, tmp);

    tmp = sold_max_d + sold_min_d * min_solder_inner_trace_cost_ + min_normal_via_cost_;
    result = std::min(result, tmp);

    tmp = comp_max_d + comp_min_d + 2 * min_normal_via_cost_;
    result = std::min(result, tmp);

    tmp = sold_max_d + sold_min_d + 2 * min_normal_via_cost_;
    result = std::min(result, tmp);

    return result;
}

double DestinationDistance::calculate_cheap_distance(const geometry::planar::IntBox& p_box, int p_layer)
{
    double saved = min_normal_via_cost_;
    min_normal_via_cost_ = min_cheap_via_cost_;
    double result = calculate(p_box, p_layer);
    min_normal_via_cost_ = saved;
    return result;
}

} // namespace freerouting::autoroute
