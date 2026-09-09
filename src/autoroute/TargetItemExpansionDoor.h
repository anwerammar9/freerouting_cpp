#pragma once

#include <memory>
#include "autoroute/ExpandableObject.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace autoroute {

class Item;
class ShapeSearchTree;

/**
 * @brief An expansion door leading to a start or destination item of the autoroute algorithm
 * 
 * This class implements the ExpandableObject interface and represents a door
 * that connects to a target item (either start or destination) in the autorouting
 * algorithm. It manages the shape intersection and maze search elements.
 */
class TargetItemExpansionDoor : public ExpandableObject {
public:
    /**
     * @brief Creates a new instance of TargetItemExpansionDoor
     * 
     * @param p_item The target item
     * @param p_tree_entry_no The tree entry number
     * @param p_room The complete expansion room
     * @param p_search_tree The shape search tree
     */
    TargetItemExpansionDoor(std::shared_ptr<Item> p_item, 
                           int p_tree_entry_no, 
                           std::shared_ptr<CompleteExpansionRoom> p_room, 
                           std::shared_ptr<ShapeSearchTree> p_search_tree);

    // ExpandableObject interface implementation
    std::shared_ptr<TileShape> get_shape() const override;
    int get_dimension() const override;
    std::shared_ptr<CompleteExpansionRoom> other_room(std::shared_ptr<CompleteExpansionRoom> p_room) const override;
    std::shared_ptr<MazeSearchElement> get_maze_search_element(int p_no) const override;
    int maze_search_element_count() const override;
    void reset() override;

    /**
     * @brief Check if this is a destination door (vs start door)
     * 
     * @return true if this is a destination door
     */
    bool is_destination_door() const;

    // Accessors
    std::shared_ptr<Item> get_item() const { return item_; }
    int get_tree_entry_no() const { return tree_entry_no_; }
    std::shared_ptr<CompleteExpansionRoom> get_room() const { return room_; }

private:
    std::shared_ptr<Item> item_;
    const int tree_entry_no_;
    std::shared_ptr<CompleteExpansionRoom> room_;
    std::shared_ptr<TileShape> shape_;
    std::shared_ptr<MazeSearchElement> maze_search_info_;
};

} // namespace autoroute
} // namespace freerouting
