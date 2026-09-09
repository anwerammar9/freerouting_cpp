#include "autoroute/ItemAutorouteInfo.h"
#include "board/Item.h"

namespace freerouting::autoroute {

ItemAutorouteInfo::ItemAutorouteInfo(board::Item* p_item)
    : item_(p_item) {
}

ItemAutorouteInfo::~ItemAutorouteInfo() {
    // ObstacleExpansionRoom cleanup if they are not smart pointers
}

ObstacleExpansionRoom* ItemAutorouteInfo::get_expansion_room(int p_index, board::ShapeSearchTree* p_autoroute_tree) {
    // Stub for now
    return nullptr;
}

void ItemAutorouteInfo::reset_doors() {
    for (auto* room : expansion_room_arr_) {
        if (room) {
            // room->reset_doors();
        }
    }
}

} // namespace freerouting::autoroute
