#include "board/ItemSearchTreesInfo.h"

#include <utility>

namespace freerouting::board {

std::vector<datastructures::ShapeTree::Leaf*> ItemSearchTreesInfo::get_tree_entries(const datastructures::ShapeTree& p_tree) const
{
    std::vector<datastructures::ShapeTree::Leaf*> result;
    if (tree_list_.empty())
    {
        return result;
    }
    for (const SearchTreeInfo& info : tree_list_)
    {
        if (info.tree == &p_tree)
        {
            result.reserve(info.entry_list.size());
            for (const std::unique_ptr<datastructures::ShapeTree::Leaf>& leaf : info.entry_list)
            {
                result.push_back(leaf.get());
            }
            break;
        }
    }
    return result;
}

void ItemSearchTreesInfo::set_tree_entries(std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree)
{
    for (SearchTreeInfo& info : tree_list_)
    {
        if (info.tree == &p_tree)
        {
            info.entry_list = std::move(p_entries);
            return;
        }
    }
    SearchTreeInfo new_info;
    new_info.tree = &p_tree;
    new_info.entry_list = std::move(p_entries);
    tree_list_.push_back(std::move(new_info));
}

std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> ItemSearchTreesInfo::take_tree_entries(datastructures::ShapeTree& p_tree)
{
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> result;
    if (tree_list_.empty())
    {
        return result;
    }
    for (SearchTreeInfo& info : tree_list_)
    {
        if (info.tree == &p_tree)
        {
            result = std::move(info.entry_list);
            break;
        }
    }
    return result;
}

const std::vector<std::shared_ptr<geometry::planar::TileShape>>* ItemSearchTreesInfo::get_precalculated_tree_shapes(const datastructures::ShapeTree& p_tree) const
{
    for (const SearchTreeInfo& info : tree_list_)
    {
        if (info.tree == &p_tree)
        {
            return &info.precalculated_tree_shapes;
        }
    }
    return nullptr;
}

void ItemSearchTreesInfo::set_precalculated_tree_shapes(std::vector<std::shared_ptr<geometry::planar::TileShape>> p_shapes, const datastructures::ShapeTree& p_tree)
{
    for (SearchTreeInfo& info : tree_list_)
    {
        if (info.tree == &p_tree)
        {
            info.precalculated_tree_shapes = std::move(p_shapes);
            return;
        }
    }
    SearchTreeInfo new_info;
    new_info.tree = &p_tree;
    new_info.precalculated_tree_shapes = std::move(p_shapes);
    tree_list_.push_back(std::move(new_info));
}

} // namespace freerouting::board