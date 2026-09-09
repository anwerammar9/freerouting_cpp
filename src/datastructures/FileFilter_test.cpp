#include "datastructures/FileFilter.h"

#include <gtest/gtest.h>

namespace freerouting::datastructures {

TEST(FileFilterTest, AcceptsConfiguredExtensionsCaseInsensitively)
{
    FileFilter filter({"dsn", "ses"});

    EXPECT_TRUE(filter.accept("board.dsn"));
    EXPECT_TRUE(filter.accept("routes.SES"));
    EXPECT_FALSE(filter.accept("board.rules"));
}

TEST(FileFilterTest, RejectsExtensionlessAndTrailingDotFiles)
{
    FileFilter filter({"dsn"});

    EXPECT_FALSE(filter.accept("board"));
    EXPECT_FALSE(filter.accept("board."));
}

TEST(FileFilterTest, AcceptsDirectories)
{
    FileFilter filter({"dsn"});

    EXPECT_TRUE(filter.accept("."));
}

TEST(FileFilterTest, FormatsDescription)
{
    EXPECT_EQ(FileFilter({"dsn"}).get_description(), "Files with the extensions .dsn");
    EXPECT_EQ(FileFilter({"dsn", "ses"}).get_description(),
              "Files with the extensions .dsn or .ses");
    EXPECT_EQ(FileFilter({"dsn", "ses", "rules"}).get_description(),
              "Files with the extensions .dsn, .ses or .rules");
}

} // namespace freerouting::datastructures
