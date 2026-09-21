#pragma once

/**
 * Port of the Java ShapeSearchTree (src/board/ShapeSearchTree.java).
 * Elementary geometric search functions making direct use of the MinAreaTree
 * in the package datastructures. The stored objects are class Item, which
 * implements the ShapeTree::Storable interface.
 */

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "datastructures/MinAreaTree.h"

namespace freerouting::geometry::planar {
class IntBox;
class IntOctagon;
class ShapeBoundingDirections;
class TileShape;
} // namespace freerouting::geometry::planar

namespace freerouting::board {

class Item;
class BasicBoard;
class SearchTreeObject;

/**
 * Total order on tree objects mirroring the Java TreeSet ordering, which uses
 * the Storable compareTo (for board items: reversed id number order).
 */
struct TreeStorableLess
{
    bool operator()(const ::freerouting::datastructures::ShapeTree::Storable* p_a,
        const ::freerouting::datastructures::ShapeTree::Storable* p_b) const
    {
        if (p_a == p_b)
        {
            return false;
        }
        if (p_a == nullptr)
        {
            return true;
        }
        if (p_b == nullptr)
        {
            return false;
        }
        return p_a->compare_to(*p_b) < 0;
    }
};

class ShapeSearchTree : public freerouting::datastructures::MinAreaTree
{
public:
    using Leaf = freerouting::datastructures::ShapeTree::Leaf;
    using LeafLess = freerouting::datastructures::MinAreaTree::LeafLess;
    using TreeEntry = freerouting::datastructures::ShapeTree::TreeEntry;

    /**
     * Creates a new ShapeSearchTree with forty-five degree bounding
     * directions. p_compensated_clearance_class_no is the clearance class
     * number for which the shapes of this tree are compensated. If
     * p_compensated_clearance_class_no = 0, the shapes are not compensated.
     */
    ShapeSearchTree(BasicBoard* p_board, int p_compensated_clearance_class_no);

    ShapeSearchTree(const ::freerouting::geometry::planar::ShapeBoundingDirections& p_directions, BasicBoard* p_board,
        int p_compensated_clearance_class_no);
    ~ShapeSearchTree() override = default;

    void insert(Item* p_item);
    void insert(Item* p_item, int p_shape_index);

    /**
     * Inserts the shape of p_object into the tree. Mirrors Java
     * ShapeSearchTree.insert(ShapeTree.Storable), used to store complete
     * expansion rooms in the autoroute search tree.
     */
    void insert(SearchTreeObject& p_object);

    void remove(Item* p_item);

    std::vector<Item*> find_nearby(const ::freerouting::geometry::planar::IntBox& p_box, int p_layer, int p_from_x, int p_from_y);
    std::set<Leaf*, LeafLess> overlaps(const ::freerouting::geometry::planar::IntBox& p_box);
    std::set<Leaf*, LeafLess> overlaps(const ::freerouting::geometry::planar::IntOctagon& p_oct);

    /**
     * Returns the clearance compensation value of p_clearance_class_no to the
     * clearance compensation class of this search tree on layer p_layer.
     * Returns 0, if no clearance compensation is used for this tree.
     */
    int clearance_compensation_value(int p_clearance_class_no, int p_layer) const;

    int get_compensated_clearance_class_no() const { return compensated_clearance_class_no_; }

    bool is_clearance_compensation_used() const { return compensated_clearance_class_no_ > 0; }

    // ================= Ported Java query layer =================

    /**
     * Puts all tree entries overlapping with p_shape on layer p_layer into
     * p_tree_entries. If p_layer < 0, the layer is ignored.
     */
    void overlapping_tree_entries(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer,
        std::vector<TreeEntry>& p_tree_entries);

    /**
     * Puts all tree entries overlapping with p_shape on layer p_layer into
     * p_tree_entries, ignoring entries whose object contains a net number of
     * p_ignore_net_nos. If p_layer < 0, the layer is ignored.
     */
    void overlapping_tree_entries(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer,
        const std::vector<int>& p_ignore_net_nos, std::vector<TreeEntry>& p_tree_entries);

    /**
     * Returns all tree entries overlapping with p_shape on layer p_layer.
     */
    std::vector<TreeEntry> overlapping_tree_entries(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer);

    /**
     * Puts all objects stored in the tree overlapping with p_shape on layer
     * p_layer into p_obstacles. If p_layer < 0, the layer is ignored.
     */
    void overlapping_objects(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer,
        const std::vector<int>& p_ignore_net_nos, std::set<SearchTreeObject*, TreeStorableLess>& p_obstacles);

    /**
     * Returns all objects stored in the tree overlapping with p_shape on layer
     * p_layer. If p_layer < 0, the layer is ignored.
     */
    std::set<SearchTreeObject*, TreeStorableLess> overlapping_objects(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer);

    /**
     * Returns all tree entries overlapping with p_shape on layer p_layer
     * inclusive clearance. p_clearance_class is the index in the clearance
     * matrix describing the required clearance restrictions against other
     * items. May also return items which are nearly overlapping but do not
     * overlap with exact calculation. If p_layer < 0, the layer is ignored.
     */
    std::vector<TreeEntry> overlapping_tree_entries_with_clearance(const ::freerouting::geometry::planar::TileShape& p_shape,
        int p_layer, const std::vector<int>& p_ignore_net_nos, int p_cl_type);

    /**
     * Puts all objects overlapping with p_shape on layer p_layer inclusive
     * clearance into p_obstacles. If p_layer < 0, the layer is ignored.
     */
    void overlapping_objects_with_clearance(const ::freerouting::geometry::planar::TileShape& p_shape, int p_layer,
        const std::vector<int>& p_ignore_net_nos, int p_cl_type, std::set<SearchTreeObject*, TreeStorableLess>& p_obstacles);

    /**
     * Returns all items overlapping with p_shape on layer p_layer inclusive
     * clearance. If p_layer < 0, the layer is ignored.
     */
    std::set<Item*, TreeStorableLess> overlapping_items_with_clearance(const ::freerouting::geometry::planar::TileShape& p_shape,
        int p_layer, const std::vector<int>& p_ignore_net_nos, int p_clearance_class);

    /**
     * Removes all entries from this tree.
     */
    void clear();

    bool validate_entries(Item* p_item) const;

    std::string to_string() const;

private:
    /**
     * Exact clearance check: looks up all tree entries so that inserting an
     * item with shape p_shape, net number p_ignore_net_nos and clearance type
     * p_cl_type on layer p_layer would produce a clearance violation. Used only
     * internally, the clearance compensation is not taken into account.
     */
    void overlapping_tree_entries_with_clearance_exact(const ::freerouting::geometry::planar::TileShape& p_shape,
        int p_layer, const std::vector<int>& p_ignore_net_nos, int p_cl_type, std::vector<TreeEntry>& p_obstacle_entries);

    /**
     * Polymorphic intersection of two tile shapes. Exact for regular tile
     * shapes (IntBox/IntOctagon); falls back to a bounding octagon check for
     * arbitrary shapes.
     */
    static bool shapes_intersect(const ::freerouting::geometry::planar::TileShape& p_a,
        const ::freerouting::geometry::planar::TileShape& p_b);

    /**
     * Polymorphic enlargement of a tile shape. Returns nullptr for unsupported
     * shape types.
     */
    static std::unique_ptr<::freerouting::geometry::planar::TileShape> enlarge_tile(
        const ::freerouting::geometry::planar::TileShape& p_shape, int p_offset);

    BasicBoard* board_;
    int compensated_clearance_class_no_;
};

class ShapeSearchTree90Degree : public ShapeSearchTree
{
public:
    explicit ShapeSearchTree90Degree(BasicBoard* p_board, int p_compensated_clearance_class_no);
    ~ShapeSearchTree90Degree() override = default;
};

class ShapeSearchTree45Degree : public ShapeSearchTree
{
public:
    explicit ShapeSearchTree45Degree(BasicBoard* p_board, int p_compensated_clearance_class_no);
    ~ShapeSearchTree45Degree() override = default;
};

} // namespace freerouting::board