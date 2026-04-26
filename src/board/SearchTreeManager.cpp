#include "board/SearchTreeManager.h"
#include "board/BasicBoard.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"
#include <algorithm>

namespace freerouting::board {

SearchTreeManager::SearchTreeManager(BasicBoard* p_board)
    : board_(p_board)
{
    // C++ ShapeSearchTree takes only 2 arguments for now
    default_tree_ = new ShapeSearchTree(p_board, 0);
    compensated_search_trees_.push_back(default_tree_);
}

SearchTreeManager::~SearchTreeManager()
{
    for (auto tree : compensated_search_trees_) {
        delete tree;
    }
}

void SearchTreeManager::insert(Item* p_item)
{
    for (auto curr_tree : compensated_search_trees_) {
        curr_tree->insert(p_item);
    }
    p_item->set_on_board(true);
}

void SearchTreeManager::remove(Item* p_item)
{
    if (!p_item->is_on_board()) {
        return;
    }
    for (auto curr_tree : compensated_search_trees_) {
        // Need get_search_tree_entries in Item.h
        // curr_tree->remove(p_item->get_search_tree_entries(curr_tree));
    }
    // p_item->clear_search_tree_entries();
    p_item->set_on_board(false);
}

bool SearchTreeManager::validate_entries(Item* p_item) const
{
    bool result = true;
    for (auto curr_tree : compensated_search_trees_) {
        if (!curr_tree->validate_entries(p_item)) {
            result = false;
        }
    }
    return result;
}

void SearchTreeManager::set_clearance_compensation_used(bool p_value)
{
    if (clearance_compensation_used_ == p_value) {
        return;
    }
    clearance_compensation_used_ = p_value;
    remove_all_board_items();
    
    for (auto tree : compensated_search_trees_) {
        delete tree;
    }
    compensated_search_trees_.clear();
    
    int compensated_clearance_class_no = p_value ? 1 : 0;
    default_tree_ = new ShapeSearchTree(board_, compensated_clearance_class_no);
    compensated_search_trees_.push_back(default_tree_);
    insert_all_board_items();
}

void SearchTreeManager::remove_all_board_items()
{
}

void SearchTreeManager::insert_all_board_items()
{
}

ShapeSearchTree* SearchTreeManager::get_autoroute_tree(int p_clearance_class_no)
{
    for (auto curr_tree : compensated_search_trees_) {
        if (curr_tree->get_compensated_clearance_class_no() == p_clearance_class_no) {
            return curr_tree;
        }
    }
    
    auto curr_autoroute_tree = new ShapeSearchTree(board_, p_clearance_class_no);
    compensated_search_trees_.push_back(curr_autoroute_tree);
    
    // Reinsert items...
    return curr_autoroute_tree;
}

void SearchTreeManager::reset_compensated_trees()
{
    compensated_search_trees_.erase(
        std::remove_if(compensated_search_trees_.begin(), compensated_search_trees_.end(),
                       [this](ShapeSearchTree* t) { return t != default_tree_; }),
        compensated_search_trees_.end());
}

void SearchTreeManager::reinsert_tree_items()
{
    remove_all_board_items();
    insert_all_board_items();
}

void SearchTreeManager::clearance_value_changed()
{
}

void SearchTreeManager::clearance_class_removed(int p_no)
{
}

} // namespace freerouting::board
