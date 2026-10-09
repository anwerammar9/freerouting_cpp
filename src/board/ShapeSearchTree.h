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
class DrillItem;
class BoardOutline;
class ObstacleArea;
class BasicBoard;
class SearchTreeObject;
class Pin;
class PolylineTrace;

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

    // Keep the base MinAreaTree/ShapeTree insert() overloads visible next to
    // the Item/Storable insert overloads declared below.
    using MinAreaTree::insert;

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

    /**
     * The tree shapes of a drill item: the bounding box / octagon / tile (by
     * the board's angle restriction) of each layer shape, enlarged by the
     * clearance compensation and the drill hole clearance delta. Port of Java
     * ShapeSearchTree.calculateTreeShapes(DrillItem).
     */
    std::vector<std::shared_ptr<::freerouting::geometry::planar::TileShape>> calculate_tree_shapes(
        const DrillItem& p_drill_item) const;

    /**
     * The tree shapes of the board outline: the convex pieces of the keepout
     * outside the outline, or thin shapes along every outline line, on every
     * layer. Port of Java ShapeSearchTree.calculateTreeShapes(BoardOutline).
     */
    std::vector<std::shared_ptr<::freerouting::geometry::planar::TileShape>> calculate_tree_shapes(
        const BoardOutline& p_board_outline) const;

    /**
     * The tree shapes of an obstacle/conduction area: the convex pieces of the
     * area, enlarged by the clearance compensation and divided into sections
     * of at most 50000 (or 500 mil with a host CAD). Port of Java
     * ShapeSearchTree.calculateTreeShapes(ObstacleArea).
     */
    std::vector<std::shared_ptr<::freerouting::geometry::planar::TileShape>> calculate_tree_shapes(
        const ObstacleArea& p_obstacle_area) const;

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
     * Parameters of a room to be completed by the search tree. Mirrors the
     * arguments of the Java ShapeSearchTree.completeShape(IncompleteFreeSpaceExpansionRoom,
     * int, SearchTreeObject, TileShape). p_shape may be null, in which case
     * the room covers the whole board box.
     */
    struct CompleteShapeInput
    {
        const ::freerouting::geometry::planar::TileShape* shape = nullptr;
        const ::freerouting::geometry::planar::TileShape* contained_shape = nullptr;
        int layer = -1;
        int net_no = 0;
        SearchTreeObject* ignore_object = nullptr;
        const ::freerouting::geometry::planar::TileShape* ignore_shape = nullptr;
    };

    /**
     * One free space section of a completed room. shape is a maximal TileShape
     * not intersecting the interior of a net dependent obstacle; contained_shape
     * is the part of the input contained shape inside shape.
     */
    struct CompleteShapePiece
    {
        std::unique_ptr<::freerouting::geometry::planar::TileShape> shape;
        std::unique_ptr<::freerouting::geometry::planar::TileShape> contained_shape;
    };

    /**
     * Calculates free space rooms contained in the shape of p_input, which may
     * overlap only with objects of the p_input.net_no on p_input.layer.
     * p_input.contained_shape will be contained in the shape of the result.
     * If that is not possible, several rooms are returned whose shapes intersect
     * with p_input.contained_shape. If p_input.ignore_shape != null, objects of
     * type CompleteFreeSpaceExpansionRoom whose intersection with the room shape
     * is contained in p_input.ignore_shape are ignored. Port of Java
     * ShapeSearchTree.completeShape.
     */
    std::vector<CompleteShapePiece> complete_shape(const CompleteShapeInput& p_input);

    /**
     * Changes the shape with index p_shape_index of p_item to p_new_shape and
     * updates the entry in the tree. Port of Java
     * ShapeSearchTree.changeItemShape(Item, int, TileShape).
     */
    void change_item_shape(Item* p_item, int p_shape_index,
        std::shared_ptr<::freerouting::geometry::planar::TileShape> p_new_shape);

    /**
     * Reduces the first or last shape of p_trace at p_tie_pin, so that the
     * autorouter algorithm can find a connection for a different net. Port of
     * Java ShapeSearchTree.reduceTraceShapeAtTiePin(Pin, PolylineTrace).
     */
    void reduce_trace_shape_at_tie_pin(Pin* p_tie_pin, PolylineTrace* p_trace);

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

    /**
     * Exact intersection of two tile shapes. Both shapes are converted to
     * Simplices, intersected and simplified to the tightest representation.
     */
    static std::unique_ptr<::freerouting::geometry::planar::TileShape> intersection_of_two_shapes(
        const ::freerouting::geometry::planar::TileShape& p_first,
        const ::freerouting::geometry::planar::TileShape& p_second);

    /**
     * Restrains the shape of p_room to a TileShape which does not intersect
     * with the interior of p_obstacle_shape. p_room.contained_shape will be
     * contained in the shape of the result. If that is not possible, several
     * pieces are appended to p_result whose shapes intersect with
     * p_room.contained_shape. Port of Java
     * ShapeSearchTree.restrainShape(IncompleteFreeSpaceExpansionRoom, TileShape).
     */
    static void restrain_shape(const CompleteShapePiece& p_room,
        const ::freerouting::geometry::planar::TileShape& p_obstacle_shape,
        std::vector<CompleteShapePiece>& p_result);

    /**
     * Makes sure that on each layer there are more than 1 free space rooms, even
     * if there are no objects on the layer. Port of Java
     * ShapeSearchTree.divideLargeRoom(...).
     */
    static void divide_large_room(const ::freerouting::geometry::planar::IntBox& p_board_box,
        std::vector<CompleteShapePiece>& p_rooms);

    /**
     * Conservatively checks whether p_outer contains p_inner. Exact for regular
     * tile shapes; returns false for arbitrary shapes so that expansion rooms
     * are only ignored if containment is guaranteed. Used to decide whether an
     * already completed free space room may be reused as ignore shape.
     */
    static bool shape_contains(const ::freerouting::geometry::planar::TileShape& p_outer,
        const ::freerouting::geometry::planar::TileShape& p_inner);

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