#include "settings/RouterSettings.h"

#include <climits>

#include <gtest/gtest.h>

#include "settings/sources/DefaultSettings.h"

namespace freerouting::settings {

TEST(RouterSettingsTest, DefaultConstructedFieldsAreNullWithJavaFallbacks)
{
    RouterSettings settings;
    EXPECT_FALSE(settings.max_threads.has_value());
    EXPECT_FALSE(settings.layers.has_value());
    EXPECT_EQ(settings.get_layer_count(), 0);
    EXPECT_TRUE(settings.get_run_router());
    EXPECT_FALSE(settings.get_run_optimizer());
    EXPECT_TRUE(settings.get_run_fanout());
    EXPECT_FALSE(settings.is_fanout_enabled());
    EXPECT_TRUE(settings.get_vias_allowed());
    EXPECT_FALSE(settings.get_automatic_neckdown());
    EXPECT_EQ(settings.get_via_costs(), 1);
    EXPECT_EQ(settings.get_plane_via_costs(), 1);
    EXPECT_EQ(settings.get_start_ripup_costs(), 1);
    EXPECT_EQ(settings.get_neck_width_um(), 0.0);
    EXPECT_FALSE(settings.is_strict_drc());
    EXPECT_TRUE(settings.get_plane_nets().empty());
    EXPECT_FALSE(settings.is_plane_as_obstacle());
    EXPECT_TRUE(settings.get_trace_costs().empty());
}

TEST(RouterSettingsTest, SetLayerCountResetsLayers)
{
    RouterSettings settings(3);
    EXPECT_EQ(settings.get_layer_count(), 3);
    settings.set_layer_active(1, false);
    settings.set_preferred_direction_is_horizontal(0, true);
    settings.set_bend_cost(2, 20.0);
    EXPECT_EQ((*settings.layers)[2].bend_cost, RouterSettings::MAX_BEND_COST);

    // Same count: the existing layers are reset anyway (Java loop after the reallocation check).
    settings.set_layer_count(3);
    EXPECT_TRUE(settings.is_layer_active(1));
    EXPECT_FALSE((*settings.layers)[0].preferred_direction_horizontal.has_value());
    EXPECT_FALSE((*settings.layers)[2].bend_cost.has_value());
    EXPECT_EQ(*settings.scoring.preferred_direction_trace_cost, (std::vector<double>{1.0, 1.0, 1.0}));
    EXPECT_FALSE(settings.are_board_specific_trace_costs_applied());
}

TEST(RouterSettingsTest, PerLayerAccessorsFollowJava)
{
    RouterSettings settings(2);
    // Null preferred direction: layer % 2 == 1.
    EXPECT_FALSE(settings.get_preferred_direction_is_horizontal(0));
    EXPECT_TRUE(settings.get_preferred_direction_is_horizontal(1));
    EXPECT_FALSE(settings.get_preferred_direction_is_horizontal(2)); // out of range
    EXPECT_FALSE(settings.is_layer_active(-1));

    // Bend cost falls back to the clamped scoring default.
    EXPECT_EQ(settings.get_bend_cost(0), 0.0);
    settings.scoring.default_bend_cost = 12.0;
    EXPECT_EQ(settings.get_bend_cost(0), RouterSettings::MAX_BEND_COST);
    settings.set_bend_cost(0, -1.0);
    EXPECT_EQ(settings.get_bend_cost(0), 0.0);

    settings.set_preferred_direction_trace_costs(0, 0.01);
    settings.set_against_preferred_direction_trace_costs(0, 3.0);
    EXPECT_EQ(settings.get_preferred_direction_trace_costs(0), 0.1);
    EXPECT_EQ(settings.get_against_preferred_direction_trace_costs(0), 3.0);
    EXPECT_TRUE(settings.are_board_specific_trace_costs_applied());
    // Layer 0 is vertical: horizontal movement goes against the preferred direction.
    EXPECT_EQ(settings.get_horizontal_trace_costs(0), 3.0);
    EXPECT_EQ(settings.get_vertical_trace_costs(0), 0.1);
    auto costs = settings.get_trace_costs();
    ASSERT_EQ(costs.size(), 2U);
}

TEST(RouterSettingsTest, TraceCostSetterReallocatesZeroFilledArray)
{
    RouterSettings settings(2);
    settings.scoring.preferred_direction_trace_cost.reset();
    settings.set_preferred_direction_trace_costs(1, 2.0);
    // Java new double[layerCount]: the other entry is 0.
    EXPECT_EQ(*settings.scoring.preferred_direction_trace_cost, (std::vector<double>{0.0, 2.0}));
}

TEST(RouterSettingsTest, CostSettersClampToOne)
{
    RouterSettings settings;
    settings.set_via_costs(0);
    settings.set_plane_via_costs(-3);
    settings.set_start_ripup_costs(250);
    EXPECT_EQ(settings.get_via_costs(), 1);
    EXPECT_EQ(settings.get_plane_via_costs(), 1);
    EXPECT_EQ(settings.get_start_ripup_costs(), 250);
}

TEST(RouterSettingsTest, SetMaxThreadsNormalizesAndSyncs)
{
    RouterSettings settings;
    const int processors = RouterSettings::available_processors();
    settings.set_max_threads(0);
    EXPECT_EQ(settings.max_threads, processors);
    EXPECT_EQ(settings.autorouter.max_threads, processors);
    EXPECT_EQ(settings.optimizer.max_threads, processors);
    settings.set_max_threads(-5);
    EXPECT_EQ(settings.max_threads, RouterSettings::default_max_threads());
    settings.set_max_threads(processors + 100);
    EXPECT_EQ(settings.max_threads, processors);

    RouterSettings nested;
    nested.max_threads = 1;
    EXPECT_EQ(nested.get_autorouter_max_threads(), 1);
    nested.autorouter.max_threads = 0;
    EXPECT_EQ(nested.get_autorouter_max_threads(), processors);
}

TEST(RouterSettingsTest, ValidateNormalizesValues)
{
    RouterSettings settings;
    settings.autorouter.max_passes = 10000;
    settings.trace_pull_tight_accuracy = 0;
    settings.validate();
    EXPECT_EQ(settings.autorouter.max_passes, 0);
    EXPECT_EQ(settings.max_threads, RouterSettings::default_max_threads());
    EXPECT_EQ(settings.trace_pull_tight_accuracy, 500);

    settings.autorouter.max_passes = INT_MAX; // Integer.MAX_VALUE is allowed
    settings.max_threads = -1;
    settings.validate();
    EXPECT_EQ(settings.autorouter.max_passes, INT_MAX);
    EXPECT_EQ(settings.max_threads, RouterSettings::default_max_threads());

    settings.autorouter.max_passes = -1;
    settings.max_threads = RouterSettings::available_processors() + 1;
    settings.validate();
    EXPECT_EQ(settings.autorouter.max_passes, 0);
    EXPECT_EQ(settings.max_threads, RouterSettings::available_processors());
}

TEST(RouterSettingsTest, CloneIsDeepAndDropsResultJsonPath)
{
    RouterSettings settings = sources::DefaultSettings::create_settings();
    settings.set_layer_count(2);
    settings.set_layer_active(1, false);
    settings.result_json_path = "result.json";
    settings.plane_nets = std::vector<std::string>{"GND"};

    RouterSettings copy = settings.clone();
    EXPECT_FALSE(copy.result_json_path.has_value());
    EXPECT_FALSE(copy.is_layer_active(1));
    EXPECT_EQ(copy.get_via_costs(), 50);
    EXPECT_EQ(copy.fanout.max_passes, 20);
    EXPECT_EQ(copy.get_plane_nets(), std::vector<std::string>{"GND"});

    copy.set_layer_active(1, true);
    (*copy.scoring.preferred_direction_trace_cost)[0] = 9.0;
    EXPECT_FALSE(settings.is_layer_active(1));
    EXPECT_EQ((*settings.scoring.preferred_direction_trace_cost)[0], 1.0);
}

TEST(RouterSettingsTest, ApplyNewValuesFromRecursesIntoNestedSettings)
{
    RouterSettings merged = sources::DefaultSettings::create_settings().clone();
    RouterSettings cli;
    cli.optimizer.max_threads = 999;
    cli.autorouter.max_passes = 7;
    cli.scoring.via_costs = 50; // equal to the default: copied but not counted
    cli.strict_drc = true;
    EXPECT_EQ(merged.apply_new_values_from(cli), 3);
    EXPECT_EQ(merged.optimizer.max_threads, 999);
    EXPECT_EQ(merged.autorouter.max_passes, 7);
    EXPECT_EQ(merged.strict_drc, true);
    // Untouched defaults survive.
    EXPECT_EQ(merged.optimizer.max_passes, 100);
    EXPECT_EQ(merged.get_start_ripup_costs(), 100);
}

TEST(RouterSettingsTest, CopyFieldsMergesLayerArrays)
{
    // Target long enough: element-wise merge, source length counted.
    RouterSettings target(3);
    RouterSettings source;
    source.layers = std::vector<LayerSettings>{LayerSettings(false, std::nullopt), LayerSettings()};
    EXPECT_EQ(copy_fields(source, target), 2);
    ASSERT_EQ(target.get_layer_count(), 3);
    EXPECT_FALSE(target.is_layer_active(0));
    EXPECT_TRUE(target.is_layer_active(1));
    EXPECT_TRUE(target.is_layer_active(2));

    // Target shorter: a new array built from fresh elements replaces it.
    RouterSettings short_target(1);
    RouterSettings long_source;
    long_source.layers = std::vector<LayerSettings>{LayerSettings(std::nullopt, true), LayerSettings(false, false)};
    EXPECT_EQ(copy_fields(long_source, short_target), 2);
    ASSERT_EQ(short_target.get_layer_count(), 2);
    // The old element's routable=true is gone: fresh element + source copy.
    EXPECT_FALSE((*short_target.layers)[0].routable.has_value());
    EXPECT_EQ((*short_target.layers)[0].preferred_direction_horizontal, true);
    EXPECT_EQ((*short_target.layers)[1].routable, false);
}

TEST(RouterSettingsTest, FieldTablePaths)
{
    RouterSettings settings(2);
    set_field_value(settings, "optimizer.max_passes", "5");
    set_field_value(settings, "autorouter-max_threads", "2");
    set_field_value(settings, "scoring.via_costs", "33");
    set_field_value(settings, "router_scoring.version", "V1_LEGACY");
    set_field_value(settings, "conduction_is_obstacle", "true");
    set_field_value(settings, "job_timeout", "01:00:00");
    set_field_value(settings, "allowed_via_types", "false");
    set_field_value(settings, "layers.routable", "false, true");
    EXPECT_EQ(settings.optimizer.max_passes, 5);
    EXPECT_EQ(settings.autorouter.max_threads, 2);
    EXPECT_EQ(settings.get_via_costs(), 33);
    EXPECT_EQ(settings.router_scoring.version, RouterScoringVersion::V1_LEGACY);
    EXPECT_TRUE(settings.is_plane_as_obstacle());
    EXPECT_EQ(settings.job_timeout_string, "01:00:00");
    EXPECT_FALSE(settings.get_vias_allowed());
    EXPECT_FALSE(settings.is_layer_active(0));
    EXPECT_TRUE(settings.is_layer_active(1));

    EXPECT_THROW(set_field_value(settings, "optimizer", "5"), std::invalid_argument);
    EXPECT_THROW(set_field_value(settings, "max_threads.x", "5"), std::invalid_argument);
    EXPECT_TRUE(find_settings_field<RouterSettings>("layers")->is_transient);
}

} // namespace freerouting::settings
