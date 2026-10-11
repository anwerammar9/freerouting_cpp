#include "board/LogicalPart.h"

#include <gtest/gtest.h>

namespace freerouting::board {

TEST(LogicalPartTest, AddSortsPinsByIndexAndNumbersParts)
{
    LogicalParts parts;
    std::vector<LogicalPart::PartPin> pins = {
        {2, "3", "A", 1, "out", 0},
        {0, "1", "A", 1, "in1", 7},
        {1, "2", "A", 1, "in2", 7},
    };
    LogicalPart* first = parts.add("NAND", pins);
    LogicalPart* second = parts.add("Buffer", {});
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->get_id(), 1);
    EXPECT_EQ(second->get_id(), 2);
    EXPECT_EQ(parts.count(), 2);
    ASSERT_EQ(first->pin_count(), 3);
    EXPECT_EQ(first->get_pin(0)->pin_name, "1");
    EXPECT_EQ(first->get_pin(1)->gate_pin_name, "in2");
    EXPECT_EQ(first->get_pin(2)->pin_index, 2);
    EXPECT_EQ(first->get_pin(3), nullptr);
    EXPECT_EQ(first->get_pin(-1), nullptr);
}

TEST(LogicalPartTest, LookupByNameIsCaseInsensitive)
{
    LogicalParts parts;
    parts.add("NAND", {});
    EXPECT_NE(parts.get("nand"), nullptr);
    EXPECT_EQ(parts.get("nor"), nullptr);
    EXPECT_EQ(parts.get(1)->get_name(), "NAND");
    EXPECT_EQ(parts.get(2), nullptr);
}

} // namespace freerouting::board
