#include "settings/OptimizerSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(OptimizerSettingsTest, TransientEnumsAreCopied)
{
    OptimizerSettings source;
    source.board_update_strategy = autoroute::BoardUpdateStrategy::GLOBAL_OPTIMAL;
    source.item_selection_strategy = autoroute::ItemSelectionStrategy::PRIORITIZED;
    OptimizerSettings target;
    EXPECT_EQ(copy_fields(source, target), 2);
    EXPECT_EQ(target.board_update_strategy, autoroute::BoardUpdateStrategy::GLOBAL_OPTIMAL);
    EXPECT_EQ(target.item_selection_strategy, autoroute::ItemSelectionStrategy::PRIORITIZED);
}

TEST(OptimizerSettingsTest, CopyFieldsScalars)
{
    OptimizerSettings source;
    source.max_passes = 100;
    source.optimization_improvement_threshold = 2.5F;
    source.timeout_string = "300s";
    OptimizerSettings target;
    target.enabled = false;
    EXPECT_EQ(copy_fields(source, target), 3);
    EXPECT_EQ(target.enabled, false);
    EXPECT_EQ(target.max_passes, 100);
    EXPECT_EQ(target.optimization_improvement_threshold, 2.5F);
    EXPECT_EQ(target.timeout_string, "300s");
}

TEST(OptimizerSettingsTest, FieldTable)
{
    OptimizerSettings settings;
    set_field_value(settings, "improvement_threshold", "1.5");
    set_field_value(settings, "item_selection_strategy", "prioritized");
    EXPECT_EQ(settings.optimization_improvement_threshold, 1.5F);
    EXPECT_EQ(settings.item_selection_strategy, autoroute::ItemSelectionStrategy::PRIORITIZED);
    // Constants that only exist in C++ have no Java name.
    EXPECT_THROW(set_field_value(settings, "item_selection_strategy", "RANDOM"), std::invalid_argument);
    EXPECT_THROW(set_field_value(settings, "board_update_strategy", "GREEDY"), std::invalid_argument);

    const auto* strategy = find_settings_field<OptimizerSettings>("board_update_strategy");
    ASSERT_NE(strategy, nullptr);
    EXPECT_TRUE(strategy->is_transient);
    EXPECT_FALSE(find_settings_field<OptimizerSettings>("max_passes")->is_transient);
}

} // namespace freerouting::settings
