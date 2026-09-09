#ifndef AUTOROUTE_EXPANSION_COST_FACTOR_H
#define AUTOROUTE_EXPANSION_COST_FACTOR_H

namespace freerouting::autoroute {

struct ExpansionCostFactor {
    double horizontal;
    double vertical;

    ExpansionCostFactor(double p_horizontal = 1.0, double p_vertical = 1.0)
        : horizontal(p_horizontal), vertical(p_vertical) {}
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_EXPANSION_COST_FACTOR_H
