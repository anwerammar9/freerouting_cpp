#pragma once

/**
 * Port of the Java ShapeTree (src/datastructures/ShapeTree.java).
 * Abstract binary search tree for shapes in the plane. The shapes are stored
 * in the leaves of the tree. Objects to be stored in the tree must implement
 * the interface ShapeTree::Storable.
 */

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace freerouting::geometry::planar {
class RegularTileShape;
class ShapeBoundingDirections;
class TileShape;
} // namespace freerouting::geometry::planar

namespace freerouting {
namespace datastructures {

class ShapeTree
{
public:
    class Leaf;
    class TreeNode;
    class InnerNode;

    class Storable
    {
    public:
        virtual ~Storable() = default;

        /**
         * Number of shapes of an object to store in p_tree.
         */
        virtual int tree_shape_count(const ShapeTree& p_tree) const = 0;

        /**
         * Gets the shape of this object with index p_index stored in p_tree.
         * Returns nullptr, if this object has no shape with that index.
         */
        virtual const geometry::planar::TileShape* get_tree_shape(const ShapeTree& p_tree, int p_index) const = 0;

        /**
         * Stores the entries in the ShapeTrees of this object for better
         * performance while for example deleting tree entries. Called only by
         * insert methods of class ShapeTree. The tree transfers the ownership
         * of the leaves to the storable, which keeps them alive as long as it
         * references them (mirroring the Java contract, where the storable
         * stores the Leaf[] array).
         */
        virtual void set_search_tree_entries(std::vector<std::unique_ptr<Leaf>> p_entries, ShapeTree& p_tree) = 0;

        /**
         * Compares this object with p_other to obtain a total order of the
         * stored objects. Port of the Java Comparable<Object> compareTo.
         */
        virtual int compare_to(const Storable& p_other) const = 0;
    };

    /**
     * Information of a single object stored in a tree.
     */
    class TreeEntry
    {
    public:
        TreeEntry(Storable* p_object, int p_shape_index_in_object)
            : object(p_object), shape_index_in_object(p_shape_index_in_object)
        {
        }

        Storable* object;
        int shape_index_in_object;
    };

    /**
     * Common functionality of inner nodes and leaf nodes.
     */
    class TreeNode
    {
    public:
        TreeNode() = default;
        TreeNode(std::unique_ptr<geometry::planar::RegularTileShape> p_bounding_shape, InnerNode* p_parent);
        virtual ~TreeNode();

        std::unique_ptr<geometry::planar::RegularTileShape> bounding_shape;
        InnerNode* parent = nullptr;
    };

    /**
     * Description of an inner node of the tree, which implements a fork to its
     * two children.
     */
    class InnerNode : public TreeNode
    {
    public:
        InnerNode(std::unique_ptr<geometry::planar::RegularTileShape> p_bounding_shape, InnerNode* p_parent);

        TreeNode* first_child = nullptr;
        TreeNode* second_child = nullptr;

        /**
         * Position of this node in the ownership list of its ShapeTree, used
         * to release the node when it is removed from the tree. Maintained by
         * ShapeTree::create_inner_node.
         */
        std::list<std::unique_ptr<TreeNode>>::iterator all_nodes_iterator;
    };

    /**
     * Description of a leaf of the tree, where the geometric information is
     * stored.
     */
    class Leaf : public TreeNode
    {
    public:
        Leaf(Storable* p_object, int p_index, InnerNode* p_parent,
            std::unique_ptr<geometry::planar::RegularTileShape> p_bounding_shape);

        /**
         * Actual object stored.
         */
        Storable* object;

        /**
         * Index of the shape in the object.
         */
        int shape_index_in_object;

        int compare_to(const Leaf& p_other) const;

        /**
         * Returns the number of nodes between this leaf and the root of the
         * tree.
         */
        int distance_to_root() const;
    };

    /**
     * Creates a new instance of ShapeTree.
     */
    explicit ShapeTree(const geometry::planar::ShapeBoundingDirections& p_directions);

    virtual ~ShapeTree() = default;

    ShapeTree(const ShapeTree&) = delete;
    ShapeTree& operator=(const ShapeTree&) = delete;

    /**
     * Inserts all shapes of p_obj into the tree.
     */
    void insert(Storable& p_obj);

    /**
     * Inserts the leaves of this tree into an array.
     */
    std::vector<Leaf*> to_array() const;

    /**
     * Removes all entries of p_obj in the tree.
     */
    void remove(const std::vector<Leaf*>& p_entries);

    /**
     * Returns the number of entries stored in the tree.
     */
    int size() const { return leaf_count; }

    /**
     * Outputs some statistic information about the tree.
     */
    void statistics(const std::string& p_message);

    /**
     * Creates the inner node in the ownership list of this tree and returns a
     * raw pointer to it.
     */
    InnerNode* create_inner_node(std::unique_ptr<geometry::planar::RegularTileShape> p_bounding_shape,
        InnerNode* p_parent);

    /**
     * Removes p_node from the ownership list of this tree.
     */
    void destroy_inner_node(InnerNode* p_node);

protected:
    /**
     * Inserts a shape - creates a new leaf with a bounding shape.
     */
    std::unique_ptr<Leaf> insert(Storable& p_object, int p_index);

    virtual void insert(Leaf& p_leaf) = 0;

    virtual void remove_leaf(Leaf* p_leaf) = 0;

    /**
     * The fixed directions for calculating bounding RegularTileShapes of
     * shapes to store in this tree.
     */
    const geometry::planar::ShapeBoundingDirections& bounding_directions;

    /**
     * Root node - initially nullptr.
     */
    TreeNode* root = nullptr;

    /**
     * The number of entries stored in the tree.
     */
    int leaf_count = 0;

private:
    /**
     * Ownership of all inner nodes created by this tree.
     */
    std::list<std::unique_ptr<TreeNode>> all_nodes_;
};

} // namespace datastructures
} // namespace freerouting