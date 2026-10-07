#include "settings/RouterSettings.h"

#include "logger/FRLogger.h"

namespace freerouting::settings {

RouterSettings::RouterSettings()
    : RouterSettings(0)
{
}

RouterSettings::RouterSettings(int p_layer_count)
    : enabled_(true)
    , algorithm_("freerouting-router")
    , max_passes_(9999)
    , vias_allowed_(true)
    , automatic_neckdown_(true)
    , max_threads_(1)
    , optimizer_(std::make_unique<RouterOptimizerSettings>())
    , scoring_(std::make_unique<RouterScoringSettings>())
{
    set_layer_count(p_layer_count);
}

RouterSettings::RouterSettings(const RouterSettings& other)
    : enabled_(other.enabled_)
    , algorithm_(other.algorithm_)
    , max_passes_(other.max_passes_)
    , layer_active_(other.layer_active_)
    , preferred_direction_horizontal_(other.preferred_direction_horizontal_)
    , vias_allowed_(other.vias_allowed_)
    , automatic_neckdown_(other.automatic_neckdown_)
    , max_threads_(other.max_threads_)
    , optimizer_(std::make_unique<RouterOptimizerSettings>(*other.optimizer_))
    , scoring_(std::make_unique<RouterScoringSettings>(*other.scoring_))
{
}

RouterSettings& RouterSettings::operator=(const RouterSettings& other)
{
    if (this != &other) {
        enabled_ = other.enabled_;
        algorithm_ = other.algorithm_;
        max_passes_ = other.max_passes_;
        layer_active_ = other.layer_active_;
        preferred_direction_horizontal_ = other.preferred_direction_horizontal_;
        vias_allowed_ = other.vias_allowed_;
        automatic_neckdown_ = other.automatic_neckdown_;
        max_threads_ = other.max_threads_;
        optimizer_ = std::make_unique<RouterOptimizerSettings>(*other.optimizer_);
        scoring_ = std::make_unique<RouterScoringSettings>(*other.scoring_);
    }
    return *this;
}

void RouterSettings::set_layer_count(int p_count)
{
    layer_active_.resize(p_count, true);
    preferred_direction_horizontal_.resize(p_count, true);
}

bool RouterSettings::is_layer_active(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
        return false;
    return layer_active_[p_layer];
}

void RouterSettings::set_layer_active(int p_layer, bool p_active)
{
    if (p_layer >= 0 && p_layer < static_cast<int>(layer_active_.size()))
    {
        layer_active_[p_layer] = p_active;
    }
}

bool RouterSettings::get_preferred_direction_is_horizontal(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(preferred_direction_horizontal_.size()))
    {
        return true;
    }
    return preferred_direction_horizontal_[p_layer];
}

double RouterSettings::get_preferred_direction_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "RouterSettings.get_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    if (!scoring_ || scoring_->preferredDirectionTraceCost.empty()
        || p_layer >= static_cast<int>(scoring_->preferredDirectionTraceCost.size()))
    {
        return 1.0;
    }
    return scoring_->preferredDirectionTraceCost[p_layer];
}

double RouterSettings::get_against_preferred_direction_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "RouterSettings.get_against_preferred_direction_trace_costs: layer out of range");
        return 0;
    }
    if (!scoring_ || scoring_->undesiredDirectionTraceCost.empty()
        || p_layer >= static_cast<int>(scoring_->undesiredDirectionTraceCost.size()))
    {
        return 1.0;
    }
    return scoring_->undesiredDirectionTraceCost[p_layer];
}

double RouterSettings::get_horizontal_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "RouterSettings.get_horizontal_trace_costs: layer out of range");
        return 0;
    }
    if (get_preferred_direction_is_horizontal(p_layer))
    {
        return get_preferred_direction_trace_costs(p_layer);
    }
    return get_against_preferred_direction_trace_costs(p_layer);
}

double RouterSettings::get_vertical_trace_costs(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "RouterSettings.get_vertical_trace_costs: layer out of range");
        return 0;
    }
    if (get_preferred_direction_is_horizontal(p_layer))
    {
        return get_against_preferred_direction_trace_costs(p_layer);
    }
    return get_preferred_direction_trace_costs(p_layer);
}

std::vector<autoroute::ExpansionCostFactor> RouterSettings::get_trace_costs() const
{
    if (!scoring_ || scoring_->preferredDirectionTraceCost.empty())
    {
        return {};
    }
    std::vector<autoroute::ExpansionCostFactor> result(scoring_->preferredDirectionTraceCost.size());
    for (int i = 0; i < static_cast<int>(result.size()); ++i)
    {
        result[i] = autoroute::ExpansionCostFactor(
            get_horizontal_trace_costs(i), get_vertical_trace_costs(i));
    }
    return result;
}

} // namespace freerouting::settings