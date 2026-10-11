#include "settings/sources/DefaultSettings.h"

#include <climits>

#include <gtest/gtest.h>

namespace freerouting::settings::sources {

TEST(DefaultSettingsTest, SourceIdentity)
{
    DefaultSettings source;
    EXPECT_EQ(source.get_source_name(), "Default Settings");
    EXPECT_EQ(source.get_priority(), 0);
    ASSERT_NE(source.get_settings(), nullptr);
}

TEST(DefaultSettingsTest, RouterDefaults)
{
    const RouterSettings settings = DefaultSettings::create_settings();
    EXPECT_EQ(settings.autorouter.enabled, true);
    EXPECT_EQ(settings.autorouter.algorithm, "freerouting-router");
    EXPECT_EQ(settings.autorouter.max_passes, 0);
    EXPECT_EQ(settings.autorouter.max_items, INT_MAX);
    EXPECT_EQ(settings.job_timeout_string, "12:00:00");
    EXPECT_EQ(settings.trace_pull_tight_accuracy, 500);
    EXPECT_EQ(settings.vias_allowed, true);
    EXPECT_EQ(settings.automatic_neckdown, true);
    EXPECT_EQ(settings.max_threads, std::max(1, RouterSettings::available_processors() - 1));
    EXPECT_EQ(settings.autorouter.max_threads, settings.max_threads);
    EXPECT_EQ(settings.copper_to_edge_clearance_um, 250.0);
    EXPECT_EQ(settings.hole_clearance_um, 0.0);
    ASSERT_TRUE(settings.plane_nets.has_value());
    EXPECT_TRUE(settings.plane_nets->empty());
    EXPECT_EQ(settings.plane_as_obstacle, false);
    EXPECT_EQ(settings.strict_drc, false);
    // Layer-count-dependent arrays stay null.
    EXPECT_FALSE(settings.layers.has_value());
    EXPECT_FALSE(settings.scoring.preferred_direction_trace_cost.has_value());
    EXPECT_FALSE(settings.result_json_path.has_value());
}

TEST(DefaultSettingsTest, FanoutOptimizerAndScoringDefaults)
{
    const RouterSettings settings = DefaultSettings::create_settings();
    EXPECT_EQ(settings.fanout.max_passes, 20);
    EXPECT_EQ(settings.fanout.max_milliseconds_per_pin, 10000LL);
    EXPECT_EQ(settings.fanout.min_escape_length_mm, 2.5);
    EXPECT_EQ(settings.fanout.max_escape_length_mm, 4.5);
    EXPECT_EQ(settings.fanout.pin_sorting_order, "outer_first");
    EXPECT_EQ(settings.fanout.fallback_to_board_vias, true);

    EXPECT_EQ(settings.optimizer.max_passes, 100);
    EXPECT_EQ(settings.optimizer.optimization_improvement_threshold, 2.5F);
    EXPECT_EQ(settings.optimizer.board_update_strategy, autoroute::BoardUpdateStrategy::GLOBAL_OPTIMAL);
    EXPECT_EQ(settings.optimizer.item_selection_strategy, autoroute::ItemSelectionStrategy::SEQUENTIAL);
    EXPECT_EQ(settings.optimizer.trace_ripup_cost_factor, 0.6F);
    EXPECT_EQ(settings.optimizer.max_consecutive_failures_pass1, 12);

    EXPECT_EQ(settings.get_via_costs(), 50);
    EXPECT_EQ(settings.get_plane_via_costs(), 5);
    EXPECT_EQ(settings.get_start_ripup_costs(), 100);
    EXPECT_EQ(settings.scoring.unrouted_net_penalty, 5'000'000.0F);
    EXPECT_EQ(settings.scoring.default_bend_cost, 0.0);
    EXPECT_EQ(settings.router_scoring.version, RouterScoringVersion::V2_CONTINUOUS);
    EXPECT_FLOAT_EQ(*settings.router_scoring.unrouted_first_half_weight, 1000.0F / 3.0F);
    EXPECT_EQ(settings.optimizer_scoring.version, OptimizerScoringVersion::V2_LOWER_BOUND);
    EXPECT_EQ(settings.optimizer_scoring.excess_via_weight, 2000.0F);
}

} // namespace freerouting::settings::sources
