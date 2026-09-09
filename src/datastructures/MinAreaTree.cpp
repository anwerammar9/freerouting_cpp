#include "datastructures/MinAreaTree.h"

#include <memory>

#include "geometry/planar/RegularTileShape.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace datastructures {

using geometry::planar::RegularTileShape;

// ============== MinAreaTree ==============

MinAreaTree::MinAreaTree(const geometry::planar::ShapeBoundingDirections& p_directions)
    : ShapeTree(p_directions)
{
}

std::set<MinAreaTree::Leaf*, MinAreaTree::LeafLess> MinAreaTree::overlaps(const RegularTileShape& p_shape)
{
    std::set<Leaf*, LeafLess> found_overlaps;
    if (this->root == nullptr)
    {
        return found_overlaps;
    }
    this->node_stack.clear();
    this->node_stack.push_back(this->root);
    TreeNode* curr_node = nullptr;
    for (;;)
    {
        if (this->node_stack.empty())
        {
            break;
        }
        curr_node = this->node_stack.back();
        this->node_stack.pop_back();
        if (curr_node == nullptr)
        {
            continue;
        }
        if (curr_node->bounding_shape->intersects(p_shape))
        {
            if (Leaf* leaf = dynamic_cast<Leaf*>(curr_node))
            {
                found_overlaps.insert(leaf);
            }
            else
            {
                InnerNode* inner = static_cast<InnerNode*>(curr_node);
                this->node_stack.push_back(inner->first_child);
                this->node_stack.push_back(inner->second_child);
            }
        }
    }
    return found_overlaps;
}

void MinAreaTree::insert(Leaf& p_leaf)
{
    ++this->leaf_count;

    // Tree is empty - just insert the new leaf.
    if (root == nullptr)
    {
        root = &p_leaf;
        return;
    }

    // Non-empty tree - do a recursive location for leaf replacement.
    Leaf* leaf_to_replace = position_locate(root, p_leaf);

    // Construct a new node - whenever a leaf is added so is a new node.
    std::unique_ptr<RegularTileShape> new_bounds = p_leaf.bounding_shape->union_with(*leaf_to_replace->bounding_shape);
    InnerNode* curr_parent = leaf_to_replace->parent;
    InnerNode* new_node = this->create_inner_node(std::move(new_bounds), curr_parent);

    if (leaf_to_replace->parent != nullptr)
    {
        // Replace the pointer from the parent to the leaf with our new node.
        if (leaf_to_replace == curr_parent->first_child)
        {
            curr_parent->first_child = new_node;
        }
        else
        {
            curr_parent->second_child = new_node;
        }
    }
    // Update the parent pointers of the old leaf and new leaf to point to new node.
    leaf_to_replace->parent = new_node;
    p_leaf.parent = new_node;

    // Insert the children in any order.
    new_node->first_child = leaf_to_replace;
    new_node->second_child = &p_leaf;

    if (root == leaf_to_replace)
    {
        root = new_node;
    }
}

MinAreaTree::Leaf* MinAreaTree::position_locate(TreeNode* p_curr_node, const Leaf& p_leaf_to_insert)
{
    TreeNode* curr_node = p_curr_node;

    while (dynamic_cast<Leaf*>(curr_node) == nullptr)
    {
        InnerNode* curr_inner_node = static_cast<InnerNode*>(curr_node);
        curr_inner_node->bounding_shape =
            p_leaf_to_insert.bounding_shape->union_with(*curr_inner_node->bounding_shape);

        // Choose the child, so that the area increase of that child after taking the union
        // with the shape of p_leaf_to_insert is minimal.

        RegularTileShape* first_child_shape = curr_inner_node->first_child->bounding_shape.get();
        std::unique_ptr<RegularTileShape> union_with_first_child_shape =
            p_leaf_to_insert.bounding_shape->union_with(*first_child_shape);
        double first_area_increase = union_with_first_child_shape->area() - first_child_shape->area();

        RegularTileShape* second_child_shape = curr_inner_node->second_child->bounding_shape.get();
        std::unique_ptr<RegularTileShape> union_with_second_child_shape =
            p_leaf_to_insert.bounding_shape->union_with(*second_child_shape);
        double second_area_increase = union_with_second_child_shape->area() - second_child_shape->area();

        if (first_area_increase <= second_area_increase)
        {
            curr_node = curr_inner_node->first_child;
        }
        else
        {
            curr_node = curr_inner_node->second_child;
        }
    }
    return static_cast<Leaf*>(curr_node);
}

void MinAreaTree::remove_leaf(Leaf* p_leaf)
{
    if (p_leaf == nullptr)
    {
        return;
    }
    // remove the leaf node
    InnerNode* parent = p_leaf->parent;
    p_leaf->bounding_shape.reset();
    p_leaf->parent = nullptr;
    p_leaf->object = nullptr;
    --this->leaf_count;
    if (parent == nullptr)
    {
        // tree gets empty
        root = nullptr;
        return;
    }
    // find the other leaf of the parent
    TreeNode* other_leaf = nullptr;
    if (parent->second_child == p_leaf)
    {
        other_leaf = parent->first_child;
    }
    else if (parent->first_child == p_leaf)
    {
        other_leaf = parent->second_child;
    }
    else
    {
        logger::FRLogger::instance().warn("MinAreaTree.remove_leaf: parent inconsistent");
    }
    if (other_leaf == nullptr)
    {
        return;
    }
    // link the other leaf to the grand_parent and remove the parent node
    InnerNode* grand_parent = parent->parent;
    other_leaf->parent = grand_parent;
    if (grand_parent == nullptr)
    {
        // only one leaf left in the tree
        root = other_leaf;
    }
    else
    {
        if (grand_parent->second_child == parent)
        {
            grand_parent->second_child = other_leaf;
        }
        else if (grand_parent->first_child == parent)
        {
            grand_parent->first_child = other_leaf;
        }
        else
        {
            logger::FRLogger::instance().warn("MinAreaTree.remove_leaf: grand_parent inconsistent");
        }
    }
    parent->parent = nullptr;
    parent->first_child = nullptr;
    parent->second_child = nullptr;
    parent->bounding_shape.reset();
    this->destroy_inner_node(parent);

    // recalculate the bounding shapes of the ancestors
    // as long as it gets smaller after removing p_leaf
    InnerNode* node_to_recalculate = grand_parent;
    while (node_to_recalculate != nullptr)
    {
        std::unique_ptr<RegularTileShape> new_bounds =
            node_to_recalculate->second_child->bounding_shape->union_with(*node_to_recalculate->first_child->bounding_shape);
        if (new_bounds->contains(*node_to_recalculate->bounding_shape))
        {
            // the new bounds are not smaller, no further recalculate necessary
            break;
        }
        node_to_recalculate->bounding_shape = std::move(new_bounds);
        node_to_recalculate = node_to_recalculate->parent;
    }
}

} // namespace datastructures
} // namespace freerouting