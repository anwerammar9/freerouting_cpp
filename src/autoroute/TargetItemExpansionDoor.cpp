#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/Simplex.h"

namespace freerouting {
namespace autoroute {

TargetItemExpansionDoor::TargetItemExpansionDoor(
    std::shared_ptr<Item> p_item, 
    int p_tree_entry_no, 
    std::shared_ptr<CompleteExpansionRoom> p_room, 
    std::shared_ptr<ShapeSearchTree> p_search_tree)
    : item_(std::move(p_item))
    , tree_entry_no_(p_tree_entry_no)
    , room_(std::move(p_room))
    , maze_search_info_(std::make_shared<MazeSearchElement>())
{
    if (!room_) {
        shape_ = Simplex::EMPTY;
    } else {
        auto item_shape = item_->get_tree_shape(p_search_tree, tree_entry_no_);
        shape_ = item_shape->intersection(room_->get_shape());
    }
}

std::shared_ptr<TileShape> TargetItemExpansionDoor::get_shape() const {
    return shape_;
}

int TargetItemExpansionDoor::get_dimension() const {
    return 2;
}

std::shared_ptr<CompleteExpansionRoom> TargetItemExpansionDoor::other_room(
    std::shared_ptr<CompleteExpansionRoom> p_room) const {
    // Target item expansion doors don't have other rooms
    return nullptr;
}

std::shared_ptr<MazeSearchElement> TargetItemExpansionDoor::get_maze_search_element(int p_no) const {
    // Always return the same maze search element (only one element)
    return maze_search_info_;
}

int TargetItemExpansionDoor::maze_search_element_count() const {
    return 1;
}

void TargetItemExpansionDoor::reset() {
    maze_search_info_->reset();
}

bool TargetItemExpansionDoor::is_destination_door() const {
    auto item_info = item_->get_autoroute_info();
    return !item_info->is_start_info();
}

} // namespace autoroute
} // namespace freerouting
