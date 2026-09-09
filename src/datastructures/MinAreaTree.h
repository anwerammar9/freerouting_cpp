#pragma once

/**
 * Port of the Java MinAreaTree (src/datastructures/MinAreaTree.java).
 * Binary search tree for shapes in the plane. The shapes are stored in the
 * leaves of the tree. The algorithm for storing a new shape is as following.
 * Starting from the root go to the child, so that the increase of the bounding
 * shape of that child is minimal after adding the new shape, until you reach a
 * leaf. The use of ShapeDirections to calculate the bounding shape is for
 * historical reasons (coming from a Kd-Tree). Instead, any algorithm to
 * calculate a bounding shape of two input shapes can be used. The algorithm
 * would of course also work for higher dimensions.
 */

#include <set>
#include <vector>

#include "datastructures/ShapeTree.h"

namespace freerouting::geometry::planar {
class RegularTileShape;
class ShapeBoundingDirections;
} // namespace freerouting::geometry::planar

namespace freerouting {
namespace datastructures {

class MinAreaTree : public ShapeTree
{
public:
    /**
     * Ordering of leaf entries mirroring the Java TreeSet in the overlaps
     * method (ordered by Leaf.compareTo).
     */
    struct LeafLess
    {
        bool operator()(const Leaf* p_a, const Leaf* p_b) const { return p_a->compare_to(*p_b) < 0; }
    };

    /**
     * Constructor with a fixed set of directions defining the keys and the
     * surrounding shapes.
     */
    explicit MinAreaTree(const geometry::planar::ShapeBoundingDirections& p_directions);

    // Keep the base insert(Storable&) visible next to the leaf insert override.
    using ShapeTree::insert;

    /**
     * Calculates the objects in this tree, which overlap with p_shape.
     */
    std::set<Leaf*, LeafLess> overlaps(const geometry::planar::RegularTileShape& p_shape);

    void insert(Leaf& p_leaf) override;

    void remove_leaf(Leaf* p_leaf) override;

private:
    Leaf* position_locate(TreeNode* p_curr_node, const Leaf& p_leaf_to_insert);

    /**
     * Stack of tree nodes used for the iterative overlap query. The Java
     * implementation uses an ArrayStack<TreeNode>; a plain vector is used
     * here because the existing ArrayStack port has an unchecked first-push.
     */
    std::vector<TreeNode*> node_stack;
};

} // namespace datastructures
} // namespace freerouting