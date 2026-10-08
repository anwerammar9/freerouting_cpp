#include "board/SearchTreeManager.h"
#include "board/BasicBoard.h"
#include "board/Item.h"
#include "board/ShapeSearchTree.h"
#include <algorithm>

#include "logger/FRLogger.h"

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
        curr_tree->remove(p_item);
    }
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
    
    // All board items were removed from every tree above.
    for (auto tree : compensated_search_trees_) {
        delete tree;
    }
    compensated_search_trees_.clear();
    
    int compensated_clearance_class_no = p_value ? 1 : 0;
    default_tree_ = new ShapeSearchTree(board_, compensated_clearance_class_no);
    compensated_search_trees_.push_back(default_tree_);
    insert_all_board_items();
}


ShapeSearchTree* SearchTreeManager::get_autoroute_tree(int p_clearance_class_no)
{
    // Port of Java SearchTreeManager.getAutorouteTree. The C++ ShapeSearchTree
    // covers the Java 45/90 degree subclasses itself.
    for (auto curr_tree : compensated_search_trees_)
    {
        if (curr_tree->get_compensated_clearance_class_no() == p_clearance_class_no)
        {
            return curr_tree;
        }
    }
    auto* curr_autoroute_tree = new ShapeSearchTree(board_, p_clearance_class_no);
    compensated_search_trees_.push_back(curr_autoroute_tree);
    if (board_ != nullptr)
    {
        for (Item* curr_item : board_->get_items())
        {
            curr_autoroute_tree->insert(curr_item);
        }
    }
    return curr_autoroute_tree;
}

void SearchTreeManager::discard_tree(ShapeSearchTree* p_tree)
{
    if (board_ != nullptr)
    {
        for (Item* curr_item : board_->get_items())
        {
            p_tree->remove(curr_item);
        }
    }
    delete p_tree;
}

void SearchTreeManager::reset_compensated_trees()
{
    // Java: compensatedSearchTrees.removeIf(t -> t != defaultTree)
    std::vector<ShapeSearchTree*> kept;
    for (auto curr_tree : compensated_search_trees_)
    {
        if (curr_tree == default_tree_)
        {
            kept.push_back(curr_tree);
        }
        else
        {
            discard_tree(curr_tree);
        }
    }
    compensated_search_trees_ = std::move(kept);
}

void SearchTreeManager::reinsert_tree_items()
{
    remove_all_board_items();
    // Removing clears the tree entries but not the precalculated tree shapes,
    // so they are dropped explicitly (Java parity).
    if (board_ != nullptr)
    {
        for (Item* curr_item : board_->get_items())
        {
            curr_item->clear_derived_data();
        }
    }
    insert_all_board_items();
}

void SearchTreeManager::clearance_value_changed()
{
    // Java: delete all trees except the default tree.
    std::vector<ShapeSearchTree*> kept;
    for (auto curr_tree : compensated_search_trees_)
    {
        if (curr_tree->get_compensated_clearance_class_no() == default_tree_->get_compensated_clearance_class_no())
        {
            kept.push_back(curr_tree);
        }
        else
        {
            discard_tree(curr_tree);
        }
    }
    compensated_search_trees_ = std::move(kept);
    if (clearance_compensation_used_)
    {
        remove_all_board_items();
        insert_all_board_items();
    }
}

void SearchTreeManager::clearance_class_removed(int p_no)
{
    if (p_no == default_tree_->get_compensated_clearance_class_no())
    {
        logger::FRLogger::instance().warn("SearchtreeManager.clearance_class_removed: unable to remove default tree");
        return;
    }
    std::vector<ShapeSearchTree*> kept;
    for (auto curr_tree : compensated_search_trees_)
    {
        if (curr_tree->get_compensated_clearance_class_no() == p_no)
        {
            discard_tree(curr_tree);
        }
        else
        {
            kept.push_back(curr_tree);
        }
    }
    compensated_search_trees_ = std::move(kept);
}

void SearchTreeManager::remove_all_board_items()
{
    if (board_ == nullptr)
    {
        logger::FRLogger::instance().warn("SearchtreeManager.remove_all_board_items: board is null");
        return;
    }
    for (Item* curr_item : board_->get_items())
    {
        remove(curr_item);
    }
}

void SearchTreeManager::insert_all_board_items()
{
    if (board_ == nullptr)
    {
        logger::FRLogger::instance().warn("SearchtreeManager.insert_all_board_items: board is null");
        return;
    }
    for (Item* curr_item : board_->get_items())
    {
        curr_item->clear_derived_data();
        insert(curr_item);
    }
}

} // namespace freerouting::board
