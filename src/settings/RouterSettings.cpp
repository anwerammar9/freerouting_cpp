#include "settings/RouterSettings.h"

#include <algorithm>

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
    , run_router_(other.run_router_)
    , run_optimizer_(other.run_optimizer_)
    , vias_allowed_(other.vias_allowed_)
    , automatic_neckdown_(other.automatic_neckdown_)
    , max_threads_(other.max_threads_)
    , trace_pull_tight_accuracy_(other.trace_pull_tight_accuracy_)
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
        run_router_ = other.run_router_;
        run_optimizer_ = other.run_optimizer_;
        vias_allowed_ = other.vias_allowed_;
        automatic_neckdown_ = other.automatic_neckdown_;
        max_threads_ = other.max_threads_;
        trace_pull_tight_accuracy_ = other.trace_pull_tight_accuracy_;
        optimizer_ = std::make_unique<RouterOptimizerSettings>(*other.optimizer_);
        scoring_ = std::make_unique<RouterScoringSettings>(*other.scoring_);
    }
    return *this;
}

void RouterSettings::set_layer_count(int p_count)
{
    // Java parity: RouterSettings.setLayerCount resets the per-layer settings
    // (routable = true, preferred direction = layer % 2 == 1, bend cost = null)
    // and initializes the per-layer trace cost arrays to 1.0.
    layer_active_.assign(p_count, true);
    preferred_direction_horizontal_.resize(p_count);
    for (int i = 0; i < p_count; ++i)
    {
        preferred_direction_horizontal_[i] = (i % 2 == 1);
    }
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    scoring_->preferredDirectionTraceCost.assign(p_count, 1.0);
    scoring_->undesiredDirectionTraceCost.assign(p_count, 1.0);
}

bool RouterSettings::is_layer_active(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.get_layer_active: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(layer_active_.empty() ? 0 : static_cast<int>(layer_active_.size()) - 1) + "]");
        return false;
    }
    return layer_active_[p_layer];
}

void RouterSettings::set_layer_active(int p_layer, bool p_active)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.set_layer_active: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(layer_active_.empty() ? 0 : static_cast<int>(layer_active_.size()) - 1) + "]");
        return;
    }
    layer_active_[p_layer] = p_active;
}

bool RouterSettings::get_preferred_direction_is_horizontal(int p_layer) const
{
    if (p_layer < 0 || p_layer >= static_cast<int>(preferred_direction_horizontal_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.get_preferred_direction_is_horizontal: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(preferred_direction_horizontal_.empty() ? 0 : static_cast<int>(preferred_direction_horizontal_.size()) - 1) + "]");
        return false;
    }
    return preferred_direction_horizontal_[p_layer];
}

void RouterSettings::set_preferred_direction_is_horizontal(int p_layer, bool p_value)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(preferred_direction_horizontal_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.set_preferred_direction_is_horizontal: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(preferred_direction_horizontal_.empty() ? 0 : static_cast<int>(preferred_direction_horizontal_.size()) - 1) + "]");
        return;
    }
    preferred_direction_horizontal_[p_layer] = p_value;
}

void RouterSettings::set_via_costs(int p_value)
{
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    scoring_->via_costs = std::max(p_value, 1);
}

void RouterSettings::set_plane_via_costs(int p_value)
{
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    scoring_->plane_via_costs = std::max(p_value, 1);
}

void RouterSettings::set_start_ripup_costs(int p_value)
{
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    scoring_->start_ripup_costs = std::max(p_value, 1);
}

void RouterSettings::set_preferred_direction_trace_costs(int p_layer, double p_value)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.set_preferred_direction_trace_costs: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(layer_active_.empty() ? 0 : static_cast<int>(layer_active_.size()) - 1) + "]");
        return;
    }
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    if (scoring_->preferredDirectionTraceCost.size() != layer_active_.size())
    {
        scoring_->preferredDirectionTraceCost.assign(layer_active_.size(), 1.0);
    }
    scoring_->preferredDirectionTraceCost[p_layer] = std::max(p_value, 0.1);
}

void RouterSettings::set_against_preferred_direction_trace_costs(int p_layer, double p_value)
{
    if (p_layer < 0 || p_layer >= static_cast<int>(layer_active_.size()))
    {
        logger::FRLogger::instance().warn(
            "AutorouteSettings.set_against_preferred_direction_trace_costs: layer " + std::to_string(p_layer)
            + " out of range [0.." + std::to_string(layer_active_.empty() ? 0 : static_cast<int>(layer_active_.size()) - 1) + "]");
        return;
    }
    if (!scoring_)
    {
        scoring_ = std::make_unique<RouterScoringSettings>();
    }
    if (scoring_->undesiredDirectionTraceCost.size() != layer_active_.size())
    {
        scoring_->undesiredDirectionTraceCost.assign(layer_active_.size(), 1.0);
    }
    scoring_->undesiredDirectionTraceCost[p_layer] = std::max(p_value, 0.1);
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