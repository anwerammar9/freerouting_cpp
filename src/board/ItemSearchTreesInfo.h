#pragma once

/**
 * Port of the Java ItemSearchTreesInfo (src/board/ItemSearchTreesInfo.java).
 * Stores the tree entries (leaves) and the precalculated tree shapes of an
 * Item for each search tree the item is inserted into.
 */

#include <memory>
#include <vector>

#include "datastructures/ShapeTree.h"
#include "geometry/planar/TileShape.h"

namespace freerouting::board {

class ItemSearchTreesInfo
{
public:
    /**
     * Returns the tree entries of this object in p_tree as raw pointers,
     * or an empty list, if no entries are stored for that tree.
     */
    std::vector<datastructures::ShapeTree::Leaf*> get_tree_entries(const datastructures::ShapeTree& p_tree) const;

    /**
     * Stores p_entries as the new tree entries of this object in p_tree.
     */
    void set_tree_entries(std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree);

    /**
     * Moves the tree entries of this object in p_tree out of this object,
     * leaving an empty list behind.
     */
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> take_tree_entries(datastructures::ShapeTree& p_tree);

    /**
     * Returns the precalculated tree shapes of this object in p_tree,
     * or nullptr, if they are not calculated for that tree.
     */
    const std::vector<std::shared_ptr<geometry::planar::TileShape>>* get_precalculated_tree_shapes(const datastructures::ShapeTree& p_tree) const;

    /**
     * Stores p_shapes as the precalculated tree shapes of this object in p_tree.
     */
    void set_precalculated_tree_shapes(std::vector<std::shared_ptr<geometry::planar::TileShape>> p_shapes, const datastructures::ShapeTree& p_tree);

private:
    struct SearchTreeInfo
    {
        const datastructures::ShapeTree* tree = nullptr;
        std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> entry_list;
        std::vector<std::shared_ptr<geometry::planar::TileShape>> precalculated_tree_shapes;
    };

    std::vector<SearchTreeInfo> tree_list_;
};

} // namespace freerouting::board