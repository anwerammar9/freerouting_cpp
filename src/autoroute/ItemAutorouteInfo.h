#ifndef ITEM_AUTOROUTE_INFO_H
#define ITEM_AUTOROUTE_INFO_H

#include <vector>
#include <memory>

namespace freerouting::board {
class Item;
class ShapeSearchTree;
}

namespace freerouting::autoroute {

class Connection;
class ObstacleExpansionRoom;

/**
 * Temporary data stored in board Items used in the autoroute algorithm
 */
class ItemAutorouteInfo {
public:
    ItemAutorouteInfo(board::Item* p_item);
    ~ItemAutorouteInfo();

    bool is_start_info() const { return start_info_; }
    void set_start_info(bool p_value) { start_info_ = p_value; }

    Connection* get_precalculated_connection() const { return precalculated_connection_; }
    void set_precalculated_connection(Connection* p_connection) { precalculated_connection_ = p_connection; }

    ObstacleExpansionRoom* get_expansion_room(int p_index, board::ShapeSearchTree* p_autoroute_tree);
    void reset_doors();

private:
    board::Item* item_;
    bool start_info_ = false;
    Connection* precalculated_connection_ = nullptr;
    std::vector<ObstacleExpansionRoom*> expansion_room_arr_;
};

} // namespace freerouting::autoroute

#endif // ITEM_AUTOROUTE_INFO_H
