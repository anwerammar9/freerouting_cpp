#include "board/ItemSelectionFilter.h"

#include "board/Item.h"

namespace freerouting::board {

ItemSelectionFilter::ItemSelectionFilter()
{
    values_.fill(true);
    values_[to_index(SelectableChoices::KEEPOUT)] = false;
    values_[to_index(SelectableChoices::VIA_KEEPOUT)] = false;
    values_[to_index(SelectableChoices::COMPONENT_KEEPOUT)] = false;
    values_[to_index(SelectableChoices::CONDUCTION)] = false;
    values_[to_index(SelectableChoices::BOARD_OUTLINE)] = false;
}

ItemSelectionFilter::ItemSelectionFilter(SelectableChoices p_item_type)
{
    values_.fill(false);
    values_[to_index(p_item_type)] = true;
    values_[to_index(SelectableChoices::FIXED)] = true;
    values_[to_index(SelectableChoices::UNFIXED)] = true;
}

ItemSelectionFilter::ItemSelectionFilter(const std::vector<SelectableChoices>& p_item_types)
{
    values_.fill(false);
    for (SelectableChoices choice : p_item_types)
    {
        values_[to_index(choice)] = true;
    }
    values_[to_index(SelectableChoices::FIXED)] = true;
    values_[to_index(SelectableChoices::UNFIXED)] = true;
}

void ItemSelectionFilter::set_selected(SelectableChoices p_choice, bool p_value)
{
    values_[to_index(p_choice)] = p_value;
}

void ItemSelectionFilter::select_all()
{
    values_.fill(true);
}

void ItemSelectionFilter::deselect_all()
{
    values_.fill(false);
}

bool ItemSelectionFilter::is_selected(SelectableChoices p_choice) const
{
    return values_[to_index(p_choice)];
}

ItemSet ItemSelectionFilter::filter(const ItemSet& p_items) const
{
    ItemSet result;
    for (Item* curr_item : p_items)
    {
        if (curr_item != nullptr && curr_item->is_selected_by_filter(*this))
        {
            result.insert(curr_item);
        }
    }
    return result;
}

} // namespace freerouting::board
