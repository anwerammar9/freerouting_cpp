/**
 * @file ItemSelectionFilter_test.cpp
 * @brief Unit tests for ItemSelectionFilter
 */

#include "board/ItemSelectionFilter.h"

#include <gtest/gtest.h>

#include "board/BasicBoard.h"
#include "board/Item.h"

namespace freerouting::board {

class ItemSelectionFilterTest : public ::testing::Test
{
protected:
    // Minimal concrete Item to exercise filter() without a full board.
    class TestItem : public Item
    {
    public:
        TestItem(bool p_user_fixed, bool p_select_result)
            : Item({}, 0, 0, 0,
                   p_user_fixed ? FixedState::USER_FIXED : FixedState::NOT_FIXED,
                   nullptr),
              select_result_(p_select_result)
        {
        }

        bool is_selected_by_filter(const ItemSelectionFilter& p_filter) const override
        {
            (void)p_filter;
            return select_result_;
        }

        bool check_fixed_filter(const ItemSelectionFilter& p_filter) const
        {
            return is_selected_by_fixed_filter(p_filter);
        }

    private:
        bool select_result_;
    };
};

TEST_F(ItemSelectionFilterTest, DefaultConstructorSelection)
{
    ItemSelectionFilter filter;
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::TRACES));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::VIAS));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::PINS));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::FIXED));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::UNFIXED));

    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::KEEPOUT));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::VIA_KEEPOUT));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::COMPONENT_KEEPOUT));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::CONDUCTION));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::BOARD_OUTLINE));
}

TEST_F(ItemSelectionFilterTest, SingleChoiceConstructor)
{
    ItemSelectionFilter filter(ItemSelectionFilter::SelectableChoices::TRACES);
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::TRACES));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::FIXED));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::UNFIXED));

    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::VIAS));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::PINS));
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::CONDUCTION));
}

TEST_F(ItemSelectionFilterTest, MultiChoiceConstructor)
{
    ItemSelectionFilter filter({ItemSelectionFilter::SelectableChoices::VIAS,
                                ItemSelectionFilter::SelectableChoices::PINS});
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::VIAS));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::PINS));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::FIXED));
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::UNFIXED));

    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::TRACES));
}

TEST_F(ItemSelectionFilterTest, CopyConstructorPreservesValues)
{
    ItemSelectionFilter original(ItemSelectionFilter::SelectableChoices::VIAS);
    ItemSelectionFilter copy(original);
    for (std::size_t i = 0; i < ItemSelectionFilter::kChoiceCount; ++i)
    {
        auto choice = static_cast<ItemSelectionFilter::SelectableChoices>(i);
        EXPECT_EQ(copy.is_selected(choice), original.is_selected(choice));
    }
}

TEST_F(ItemSelectionFilterTest, SetSelectedTogglesChoice)
{
    ItemSelectionFilter filter;
    filter.set_selected(ItemSelectionFilter::SelectableChoices::TRACES, false);
    EXPECT_FALSE(filter.is_selected(ItemSelectionFilter::SelectableChoices::TRACES));

    filter.set_selected(ItemSelectionFilter::SelectableChoices::CONDUCTION, true);
    EXPECT_TRUE(filter.is_selected(ItemSelectionFilter::SelectableChoices::CONDUCTION));
}

TEST_F(ItemSelectionFilterTest, SelectAllAndDeselectAll)
{
    ItemSelectionFilter filter;
    filter.deselect_all();
    for (std::size_t i = 0; i < ItemSelectionFilter::kChoiceCount; ++i)
    {
        EXPECT_FALSE(filter.is_selected(static_cast<ItemSelectionFilter::SelectableChoices>(i)));
    }

    filter.select_all();
    for (std::size_t i = 0; i < ItemSelectionFilter::kChoiceCount; ++i)
    {
        EXPECT_TRUE(filter.is_selected(static_cast<ItemSelectionFilter::SelectableChoices>(i)));
    }
}

TEST_F(ItemSelectionFilterTest, FilterKeepsOnlySelectedItems)
{
    TestItem selected(false, true);
    TestItem rejected(false, false);

    ItemSelectionFilter filter;
    std::set<Item*> items{&selected, &rejected};
    std::set<Item*> result = filter.filter(items);

    EXPECT_EQ(result.size(), 1);
    EXPECT_NE(result.find(&selected), result.end());
    EXPECT_EQ(result.find(&rejected), result.end());
}

TEST_F(ItemSelectionFilterTest, FilterSkipsNullItems)
{
    TestItem selected(false, true);
    ItemSelectionFilter filter;
    std::set<Item*> items{&selected, nullptr};
    std::set<Item*> result = filter.filter(items);

    EXPECT_EQ(result.size(), 1);
}

TEST_F(ItemSelectionFilterTest, FixedFilterHelperRespectsFixedState)
{
    TestItem fixed_item(true, true);
    TestItem unfixed_item(false, true);

    ItemSelectionFilter only_fixed(ItemSelectionFilter::SelectableChoices::TRACES);
    only_fixed.set_selected(ItemSelectionFilter::SelectableChoices::UNFIXED, false);
    EXPECT_TRUE(fixed_item.check_fixed_filter(only_fixed));
    EXPECT_FALSE(unfixed_item.check_fixed_filter(only_fixed));

    ItemSelectionFilter only_unfixed(ItemSelectionFilter::SelectableChoices::TRACES);
    only_unfixed.set_selected(ItemSelectionFilter::SelectableChoices::FIXED, false);
    EXPECT_FALSE(fixed_item.check_fixed_filter(only_unfixed));
    EXPECT_TRUE(unfixed_item.check_fixed_filter(only_unfixed));
}

TEST_F(ItemSelectionFilterTest, IsUserFixedMatchesFixedState)
{
    TestItem fixed_item(true, true);
    TestItem unfixed_item(false, true);
    EXPECT_TRUE(fixed_item.is_user_fixed());
    EXPECT_FALSE(unfixed_item.is_user_fixed());
}

} // namespace freerouting::board
