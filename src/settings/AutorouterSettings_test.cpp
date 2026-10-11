#include "settings/AutorouterSettings.h"

#include <gtest/gtest.h>

namespace freerouting::settings {

TEST(AutorouterSettingsTest, NullSourceFieldsAreNotCopied)
{
    AutorouterSettings source;
    AutorouterSettings target;
    target.max_passes = 7;
    target.algorithm = "x";
    EXPECT_EQ(copy_fields(source, target), 0);
    EXPECT_EQ(target.max_passes, 7);
    EXPECT_EQ(target.algorithm, "x");
}

TEST(AutorouterSettingsTest, BoxedFieldsAreCopiedAndCounted)
{
    AutorouterSettings source;
    source.enabled = false;
    source.max_passes = 0; // a boxed 0 is "explicitly set", unlike a primitive default
    source.algorithm = "freerouting-router";
    AutorouterSettings target;
    target.enabled = true;
    EXPECT_EQ(copy_fields(source, target), 3);
    EXPECT_EQ(target.enabled, false);
    EXPECT_EQ(target.max_passes, 0);
    EXPECT_EQ(target.algorithm, "freerouting-router");
}

TEST(AutorouterSettingsTest, StringArrayCopyRules)
{
    // Non-empty source array is copied over a non-empty target.
    AutorouterSettings source;
    source.ignore_net_classes = std::vector<std::string>{"gnd"};
    AutorouterSettings target;
    target.ignore_net_classes = std::vector<std::string>{"a", "b"};
    EXPECT_EQ(copy_fields(source, target), 1);
    EXPECT_EQ(*target.ignore_net_classes, std::vector<std::string>{"gnd"});

    // Empty source array is copied onto a null target ...
    AutorouterSettings empty_source;
    empty_source.ignore_net_classes = std::vector<std::string>{};
    AutorouterSettings null_target;
    EXPECT_EQ(copy_fields(empty_source, null_target), 1);
    ASSERT_TRUE(null_target.ignore_net_classes.has_value());
    EXPECT_TRUE(null_target.ignore_net_classes->empty());

    // ... but not onto a non-null target.
    EXPECT_EQ(copy_fields(empty_source, target), 0);
    EXPECT_EQ(*target.ignore_net_classes, std::vector<std::string>{"gnd"});
}

TEST(AutorouterSettingsTest, FieldTableUsesSerializedNames)
{
    AutorouterSettings settings;
    set_field_value(settings, "max_passes", "12");
    set_field_value(settings, "maxThreads", "3");
    set_field_value(settings, "ignore_net_classes", " gnd , vcc ");
    EXPECT_EQ(settings.max_passes, 12);
    EXPECT_EQ(settings.max_threads, 3);
    EXPECT_EQ(*settings.ignore_net_classes, (std::vector<std::string>{"gnd", "vcc"}));

    const auto* field = find_settings_field<AutorouterSettings>("save_intermediate_stages");
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->get(settings), std::nullopt);
    settings.save_intermediate_stages = true;
    EXPECT_EQ(field->get(settings), "true");
}

} // namespace freerouting::settings
