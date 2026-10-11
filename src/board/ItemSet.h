#pragma once

#include <set>

namespace freerouting::board {

class Item;

/**
 * Java TreeSet<Item> order: Item.compareTo returns other.id - this.id, so items sort by
 * descending id. Use ItemSet wherever Java uses TreeSet/SortedSet<Item> or walks the item list;
 * a set ordered by pointer is not even reproducible between runs.
 * The call operator is defined inline in board/Item.h.
 */
struct ItemIdDescending
{
    bool operator()(const Item* p_a, const Item* p_b) const;
};

using ItemSet = std::set<Item*, ItemIdDescending>;
using ConstItemSet = std::set<const Item*, ItemIdDescending>;

} // namespace freerouting::board
