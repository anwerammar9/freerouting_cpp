#include "datastructures/ShapeTree.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>

#include "geometry/planar/IntOctagon.h"
#include "logger/FRLogger.h"

namespace freerouting {
namespace datastructures {

using geometry::planar::IntOctagon;
using geometry::planar::RegularTileShape;
using geometry::planar::TileShape;

// ============== ShapeTree::TreeNode ==============

ShapeTree::TreeNode::~TreeNode() = default;

ShapeTree::TreeNode::TreeNode(std::unique_ptr<RegularTileShape> p_bounding_shape, InnerNode* p_parent)
    : bounding_shape(std::move(p_bounding_shape)), parent(p_parent)
{
}

// ============== ShapeTree::InnerNode ==============

ShapeTree::InnerNode::InnerNode(std::unique_ptr<RegularTileShape> p_bounding_shape, InnerNode* p_parent)
    : TreeNode(std::move(p_bounding_shape), p_parent)
{
}

// ============== ShapeTree::Leaf ==============

ShapeTree::Leaf::Leaf(Storable* p_object, int p_index, InnerNode* p_parent,
    std::unique_ptr<RegularTileShape> p_bounding_shape)
    : TreeNode(std::move(p_bounding_shape), p_parent), object(p_object), shape_index_in_object(p_index)
{
}

int ShapeTree::Leaf::compare_to(const Leaf& p_other) const
{
    int result = object->compare_to(*p_other.object);
    if (result == 0)
    {
        result = shape_index_in_object - p_other.shape_index_in_object;
    }
    return result;
}

int ShapeTree::Leaf::distance_to_root() const
{
    int result = 1;
    InnerNode* curr_parent = this->parent;
    while (curr_parent != nullptr && curr_parent->parent != nullptr)
    {
        curr_parent = curr_parent->parent;
        ++result;
    }
    return result;
}

// ============== ShapeTree ==============

ShapeTree::ShapeTree(const geometry::planar::ShapeBoundingDirections& p_directions)
    : bounding_directions(p_directions), root(nullptr), leaf_count(0)
{
}

void ShapeTree::insert(Storable& p_obj)
{
    int shape_count = p_obj.tree_shape_count(*this);
    if (shape_count <= 0)
    {
        return;
    }
    std::vector<std::unique_ptr<Leaf>> leaf_arr;
    leaf_arr.reserve(static_cast<size_t>(shape_count));
    for (int i = 0; i < shape_count; ++i)
    {
        leaf_arr.push_back(insert(p_obj, i));
    }
    p_obj.set_search_tree_entries(std::move(leaf_arr), *this);
}

std::unique_ptr<ShapeTree::Leaf> ShapeTree::insert(Storable& p_object, int p_index)
{
    const TileShape* object_shape = p_object.get_tree_shape(*this, p_index);
    if (object_shape == nullptr)
    {
        return nullptr;
    }
    IntOctagon bounding = object_shape->bounding_shape(bounding_directions);
    if (bounding.is_empty())
    {
        logger::FRLogger::instance().warn("ShapeTree.insert: bounding shape of Storable is empty");
        return nullptr;
    }
    auto bounding_shape = std::make_unique<IntOctagon>(bounding);
    // Construct a new leaf and set it up.
    auto new_leaf = std::make_unique<Leaf>(&p_object, p_index, nullptr, std::move(bounding_shape));
    this->insert(*new_leaf);
    return new_leaf;
}

std::vector<ShapeTree::Leaf*> ShapeTree::to_array() const
{
    std::vector<Leaf*> result;
    if (this->root == nullptr)
    {
        return result;
    }
    result.reserve(static_cast<size_t>(this->leaf_count));
    TreeNode* curr_node = this->root;
    for (;;)
    {
        // go down from curr_node to the left most leaf
        while (dynamic_cast<InnerNode*>(curr_node) != nullptr)
        {
            curr_node = static_cast<InnerNode*>(curr_node)->first_child;
        }
        result.push_back(static_cast<Leaf*>(curr_node));

        // go up until parent.second_child != curr_node, which means we came from first_child
        InnerNode* curr_parent = curr_node->parent;
        while (curr_parent != nullptr && curr_parent->second_child == curr_node)
        {
            curr_node = curr_parent;
            curr_parent = curr_node->parent;
        }
        if (curr_parent == nullptr)
        {
            break;
        }
        curr_node = curr_parent->second_child;
    }
    return result;
}

void ShapeTree::remove(const std::vector<Leaf*>& p_entries)
{
    for (Leaf* entry : p_entries)
    {
        remove_leaf(entry);
    }
}

void ShapeTree::statistics(const std::string& p_message)
{
    std::vector<Leaf*> leaf_arr = this->to_array();
    double cumulative_depth = 0;
    int maximum_depth = 0;
    for (Leaf* leaf : leaf_arr)
    {
        if (leaf != nullptr)
        {
            int distance_to_root = leaf->distance_to_root();
            cumulative_depth += distance_to_root;
            maximum_depth = std::max(maximum_depth, distance_to_root);
        }
    }
    double average_depth = cumulative_depth / static_cast<double>(leaf_arr.size());
    logger::FRLogger::instance().info("MinAreaTree: Entry count: " + std::to_string(leaf_arr.size())
        + " log: " + std::to_string(std::llround(std::log(static_cast<double>(leaf_arr.size()))))
        + " Average depth: " + std::to_string(std::llround(average_depth))
        + " Maximum depth: " + std::to_string(maximum_depth) + " " + p_message);
}

ShapeTree::InnerNode* ShapeTree::create_inner_node(std::unique_ptr<RegularTileShape> p_bounding_shape,
    InnerNode* p_parent)
{
    auto new_node = std::make_unique<InnerNode>(std::move(p_bounding_shape), p_parent);
    InnerNode* raw = new_node.get();
    all_nodes_.push_back(std::move(new_node));
    raw->all_nodes_iterator = std::prev(all_nodes_.end());
    return raw;
}

void ShapeTree::destroy_inner_node(InnerNode* p_node)
{
    all_nodes_.erase(p_node->all_nodes_iterator);
}

} // namespace datastructures
} // namespace freerouting