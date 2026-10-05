#include "autoroute/ItemAutorouteInfo.h"

#include <algorithm>

#include "autoroute/ExpansionRoom.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

ItemAutorouteInfo::ItemAutorouteInfo(board::Item* p_item)
    : item_(p_item) {
}

ItemAutorouteInfo::~ItemAutorouteInfo() {
    // ObstacleExpansionRoom cleanup if they are not smart pointers
}

ObstacleExpansionRoom* ItemAutorouteInfo::get_expansion_room(int p_index,
    board::ShapeSearchTree* p_autoroute_tree)
{
    // Port of Java ItemAutorouteInfo.getExpansionRoom(): the array of rooms is
    // resized when the tree shape count of the item has changed, so that a
    // modified item keeps the rooms of its unchanged shapes.
    if (item_ == nullptr || p_autoroute_tree == nullptr)
    {
        return nullptr;
    }

    const size_t current_shape_count = static_cast<size_t>(item_->tree_shape_count(*p_autoroute_tree));
    if (expansion_room_arr_.size() != current_shape_count)
    {
        expansion_room_arr_.resize(current_shape_count, nullptr);
    }

    if (p_index < 0 || static_cast<size_t>(p_index) >= expansion_room_arr_.size())
    {
        logger::FRLogger::instance().warn(
            "ItemAutorouteInfo::get_expansion_room: index " + std::to_string(p_index)
            + " out of range [0, " + std::to_string(expansion_room_arr_.size()) + ")");
        return nullptr;
    }

    if (expansion_room_arr_[static_cast<size_t>(p_index)] == nullptr)
    {
        expansion_room_arr_[static_cast<size_t>(p_index)] =
            new ObstacleExpansionRoom(item_, p_index, p_autoroute_tree);
    }
    return expansion_room_arr_[static_cast<size_t>(p_index)];
}

void ItemAutorouteInfo::reset_doors()
{
    // Port of Java ItemAutorouteInfo.resetDoors(): the doors of the rooms are
    // reset before the item is autorouted for the next connection.
    for (auto* room : expansion_room_arr_)
    {
        if (room != nullptr)
        {
            room->reset_doors();
        }
    }
}

} // namespace freerouting::autoroute
