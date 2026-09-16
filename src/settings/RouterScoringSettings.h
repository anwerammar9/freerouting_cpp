#pragma once

#include <vector>

namespace freerouting::settings {

/**
 * Settings used to calculate the score/cost of a board.
 * Mirrors Java RouterScoringSettings.java.
 */
class RouterScoringSettings
{
public:
    // The cost of 1 mm of trace length if the trace is routed in the preferred
    // direction, defined for each layer.
    std::vector<double> preferredDirectionTraceCost;
    // The cost of 1 mm of trace length if the trace is routed in the undesired
    // direction, defined for each layer.
    std::vector<double> undesiredDirectionTraceCost;
    // The cost of 1 mm of trace length if the trace is routed in the preferred
    // direction.
    double defaultPreferredDirectionTraceCost = 1.0;
    // The cost of 1 mm of trace length if the trace is routed in the undesired
    // direction.
    double defaultUndesiredDirectionTraceCost = 2.5;
    // The cost of a via.
    int via_costs = 50;
    // The cost of a via if the via is placed on a plane.
    int plane_via_costs = 5;
    // The starting cost of a ripup operation. It might be increased during the
    // auto-routing process.
    int start_ripup_costs = 100;
    // The penalty for an unrouted net.
    float unroutedNetPenalty = 4000;
    // The penalty for a clearance violation.
    float clearanceViolationPenalty = 1000;
    // The penalty for a bend.
    float bendPenalty = 10;
};

} // namespace freerouting::settings