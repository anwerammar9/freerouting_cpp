#pragma once

#include <vector>
#include <list>
#include <memory>

namespace freerouting::board {

class BasicBoard;
class Item;
class ShapeSearchTree;
class PolylineTrace;

class SearchTreeManager {
public:
    SearchTreeManager(BasicBoard* p_board);
    ~SearchTreeManager();

    void insert(Item* p_item);
    void remove(Item* p_item);

    ShapeSearchTree* get_default_tree() const { return default_tree_; }

    bool validate_entries(Item* p_item) const;

    bool is_clearance_compensation_used() const { return clearance_compensation_used_; }
    void set_clearance_compensation_used(bool p_value);

    void clearance_value_changed();
    void clearance_class_removed(int p_no);

    ShapeSearchTree* get_autoroute_tree(int p_clearance_class_no);

    void reset_compensated_trees();
    void reinsert_tree_items();

private:
    void remove_all_board_items();
    void insert_all_board_items();

    BasicBoard* board_;
    ShapeSearchTree* default_tree_;
    std::vector<ShapeSearchTree*> compensated_search_trees_;
    bool clearance_compensation_used_ = false;
};

} // namespace freerouting::board
