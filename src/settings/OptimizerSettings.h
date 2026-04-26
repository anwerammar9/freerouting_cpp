#pragma once

#include <string>

namespace freerouting {
namespace settings {

class RouterOptimizerSettings
{
public:
    RouterOptimizerSettings() = default;

    bool enabled = true;
    int iterations = 3;
    double convergence_threshold = 0.01;

    std::string to_string() const;
};

class RouterScoringSettings
{
public:
    RouterScoringSettings() = default;

    double via_cost = 1.0;
    double corner_cost = 1.0;
    double length_cost = 0.01;

    std::string to_string() const;
};

} // namespace settings
} // namespace freerouting