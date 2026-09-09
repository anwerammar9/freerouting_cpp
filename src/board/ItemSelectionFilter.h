#pragma once

#include <array>
#include <cstddef>
#include <set>
#include <vector>

namespace freerouting::board {

class Item;

/**
 * @brief Filter for selecting items on the board.
 *
 * Faithful port of app.freerouting.board.ItemSelectionFilter.
 * The Java implementation stores one boolean per SelectableChoices value.
 */
class ItemSelectionFilter
{
public:
    /**
     * @brief The possible choices in the filter.
     *
     * Order matches the Java enum ordinal values.
     */
    enum class SelectableChoices
    {
        TRACES,
        VIAS,
        PINS,
        CONDUCTION,
        KEEPOUT,
        VIA_KEEPOUT,
        COMPONENT_KEEPOUT,
        BOARD_OUTLINE,
        FIXED,
        UNFIXED
    };

    static constexpr std::size_t kChoiceCount = 10;

    /**
     * @brief Creates a new filter with all item types selected,
     * except KEEPOUT, VIA_KEEPOUT, COMPONENT_KEEPOUT, CONDUCTION and BOARD_OUTLINE.
     */
    ItemSelectionFilter();

    /**
     * @brief Creates a new filter with only p_item_type selected
     * (plus FIXED and UNFIXED, as in Java).
     */
    explicit ItemSelectionFilter(SelectableChoices p_item_type);

    /**
     * @brief Creates a new filter with only p_item_types selected
     * (plus FIXED and UNFIXED, as in Java).
     */
    explicit ItemSelectionFilter(const std::vector<SelectableChoices>& p_item_types);

    ItemSelectionFilter(const ItemSelectionFilter& p_other) = default;
    ItemSelectionFilter& operator=(const ItemSelectionFilter& p_other) = default;

    /**
     * @brief Selects or deselects an item type.
     */
    void set_selected(SelectableChoices p_choice, bool p_value);

    /**
     * @brief Selects all item types.
     */
    void select_all();

    /**
     * @brief Deselects all item types.
     */
    void deselect_all();

    /**
     * @brief Returns true if the input item type is selected.
     */
    bool is_selected(SelectableChoices p_choice) const;

    /**
     * @brief Filters a collection of items with this filter.
     *
     * @note Ordering deviation: Java returns a TreeSet ordered by
     * Item.compareTo; here items are ordered by pointer value.
     */
    std::set<Item*> filter(const std::set<Item*>& p_items) const;

private:
    static constexpr std::size_t to_index(SelectableChoices p_choice)
    {
        return static_cast<std::size_t>(p_choice);
    }

    std::array<bool, kChoiceCount> values_{};
};

} // namespace freerouting::board
