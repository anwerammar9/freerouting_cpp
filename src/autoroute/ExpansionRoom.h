#pragma once

#include "autoroute/MazeSearchElement.h"
#include "autoroute/ExpandableObject.h"
#include "board/SearchTreeObject.h"
#include "datastructures/ShapeTree.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/Simplex.h"
#include <memory>
#include <vector>
#include <set>

namespace freerouting::geometry::planar {
class TileShape;
class FloatLine;
}

namespace freerouting::board {
class RoutingBoard;
class Item;
class ShapeSearchTree;
}

namespace freerouting::autoroute {

class ExpansionDoor;

class ExpansionRoom
{
public:
    virtual ~ExpansionRoom() = default;

    virtual void add_door(ExpansionDoor* p_door) = 0;
    virtual std::vector<ExpansionDoor*> get_doors() const = 0;
    virtual void clear_doors() = 0;
    virtual void reset_doors() = 0;
    virtual bool door_exists(ExpansionRoom* p_other) const = 0;
    virtual bool remove_door(ExpandableObject* p_door) = 0;
    virtual geometry::planar::IntBox get_shape() const = 0;
    virtual int get_layer() const = 0;
    virtual int get_room_id() const = 0;

    /**
     * Returns the shape of this room as a Simplex. The default converts the
     * bounding-box shape; room implementations carrying an exact Simplex (or
     * contained shape) override it. Port of Java getShape() on the room
     * hierarchy, which returns a TileShape.
     */
    virtual geometry::planar::Simplex get_shape_tile() const { return get_shape().to_Simplex(); }
};

class CompleteExpansionRoom : public ExpansionRoom
{
public:
    CompleteExpansionRoom(int p_layer, int p_room_id);
    virtual ~CompleteExpansionRoom() = default;

    void add_door(ExpansionDoor* p_door) override;
    std::vector<ExpansionDoor*> get_doors() const override;
    void clear_doors() override;
    void reset_doors() override;
    bool door_exists(ExpansionRoom* p_other) const override;
    bool remove_door(ExpandableObject* p_door) override;
    geometry::planar::IntBox get_shape() const override;
    int get_layer() const override { return layer_; }
    int get_room_id() const override { return room_id_; }

    std::vector<ExpansionDoor*>& target_doors() { return target_doors_; }
    const std::vector<ExpansionDoor*>& target_doors() const { return target_doors_; }

    void add_target_door(ExpansionDoor* p_door);
    bool all_doors_calculated() const { return doors_calculated_; }
    void set_doors_calculated(bool p_value) { doors_calculated_ = p_value; }

protected:
    int layer_;
    int room_id_;
    std::vector<ExpansionDoor*> doors_;
    std::vector<ExpansionDoor*> target_doors_;
    bool doors_calculated_ = false;
};

class CompleteFreeSpaceExpansionRoom : public CompleteExpansionRoom, public board::SearchTreeObject
{
public:
    CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::IntBox& p_shape);
    CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::Simplex& p_shape);
    virtual ~CompleteFreeSpaceExpansionRoom() = default;

    geometry::planar::IntBox get_shape() const override;
    geometry::planar::Simplex get_shape_tile() const override { return shape_tile_; }

    /**
     * Replaces the exact shape of this room. Used when a room is enlarged while
     * an edge is removed on an empty region (tryRemoveEdge). Mirrors the Java
     * setShapeOnTile() on CompleteFreeSpaceExpansionRoom.
     */
    void set_shape_tile(const geometry::planar::Simplex& p_shape) { shape_tile_ = p_shape; }

    // ================= ShapeTree::Storable =================
    int tree_shape_count(const datastructures::ShapeTree& p_tree) const override;
    const geometry::planar::TileShape* get_tree_shape(const datastructures::ShapeTree& p_tree, int p_index) const override;
    void set_search_tree_entries(std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries,
        datastructures::ShapeTree& p_tree) override;
    int compare_to(const datastructures::ShapeTree::Storable& p_other) const override;

    // ================= board::SearchTreeObject =================
    bool is_obstacle(int p_net_no) const override { return true; }
    bool is_trace_obstacle(int p_net_no) const override { return true; }
    int shape_layer(int p_index) const override { return get_layer(); }
    bool is_free_space_room() const override { return true; }
    int get_id() const override { return room_id_; }

    /**
     * Removes the tree entries of this room from p_tree. Mirrors
     * Java CompleteFreeSpaceExpansionRoom.removeFromTree().
     */
    void remove_from_tree(board::ShapeSearchTree& p_tree);

    /**
     * Marks this room as overlapping with net dependent objects. In this case
     * it cannot be retained when the net number changes in autorouting.
     * Mirrors Java CompleteFreeSpaceExpansionRoom.setNetDependent().
     */
    void set_net_dependent() { room_is_net_dependent_ = true; }
    bool is_net_dependent() const { return room_is_net_dependent_; }

    /**
     * Creates the doors from this room to the connectable items of p_net_no
     * overlapping this room which are represented in the autoroute search
     * tree by p_own_net_object. The created doors are stored as target doors.
     * Port of Java CompleteFreeSpaceExpansionRoom.calculateTargetDoors().
     *
     * The C++ rooms carry IntBox shapes only, so the door candidates are the
     * net items stored under the given tree entry whose tree shape intersects
     * the room shape. Divergence: Java tests the exact trace connection shape
     * of the (dead) C++ `Connectable` interface; here a connectable object is
     * any routeable item (trace, drill or conduction area) and its connection
     * shape is approximated by its tree shape.
     */
    void calculate_target_doors(const datastructures::ShapeTree::TreeEntry& p_own_net_object,
        int p_net_no, board::ShapeSearchTree* p_autoroute_tree);

private:
    geometry::planar::Simplex shape_tile_;
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> tree_entries_;
    bool room_is_net_dependent_ = false;
};

class ObstacleExpansionRoom : public CompleteExpansionRoom
{
public:
    ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, int p_layer, const geometry::planar::IntBox& p_shape);

    /**
     * Constructs the expansion room of the p_index_in_item-th tree shape of
     * p_item. The layer and the shape are taken from the tree shape, the room
     * id is set to (item id << 10) | index. Port of the Java
     * ObstacleExpansionRoom(Item, int, ShapeSearchTree) constructor.
     */
    ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, board::ShapeSearchTree* p_shape_tree);
    virtual ~ObstacleExpansionRoom() = default;

    geometry::planar::IntBox get_shape() const override;
    geometry::planar::Simplex get_shape_tile() const override { return shape_tile_; }
    board::Item* get_item() const { return item_; }
    int get_index_in_item() const { return index_in_item_; }

    /**
     * Tries to create a 2-dimensional door between this room and p_other.
     * A door is only created for routable items sharing a net; two rooms of
     * the same item are only connected if their shapes are neighbouring along
     * a trace polyline. Returns true if a door was created. Port of Java
     * ObstacleExpansionRoom.createOverlapDoor().
     */
    bool create_overlap_door(ObstacleExpansionRoom* p_other);

private:
    board::Item* item_;
    int index_in_item_;
    geometry::planar::IntBox shape_;
    geometry::planar::Simplex shape_tile_;
};

class IncompleteExpansionRoom : public ExpansionRoom
{
public:
    IncompleteExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape);
    virtual ~IncompleteExpansionRoom() = default;

    void add_door(ExpansionDoor* p_door) override;
    std::vector<ExpansionDoor*> get_doors() const override;
    void clear_doors() override;
    void reset_doors() override;
    bool door_exists(ExpansionRoom* p_other) const override;
    bool remove_door(ExpandableObject* p_door) override;
    geometry::planar::IntBox get_shape() const override { return shape_; }
    int get_layer() const override { return layer_; }
    int get_room_id() const override { return room_id_; }

private:
    int layer_;
    int room_id_;
    const geometry::planar::IntBox shape_;
    std::vector<ExpansionDoor*> doors_;
};

class IncompleteFreeSpaceExpansionRoom : public IncompleteExpansionRoom
{
public:
    IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape);

    /**
     * Creates an incomplete free space expansion room with an exact Simplex
     * shape and a contained shape. Mirrors the Java constructor
     * IncompleteFreeSpaceExpansionRoom(TileShape, int, TileShape).
     */
    IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer,
        const geometry::planar::Simplex& p_shape, const geometry::planar::Simplex& p_contained_shape);
    virtual ~IncompleteFreeSpaceExpansionRoom() = default;

    geometry::planar::IntBox get_shape() const override;
    geometry::planar::Simplex get_shape_tile() const override { return shape_tile_; }
    geometry::planar::Simplex get_contained_shape_tile() const { return contained_shape_tile_; }
    void set_shape_tile(const geometry::planar::Simplex& p_shape) { shape_tile_ = p_shape; }
    void set_contained_shape_tile(const geometry::planar::Simplex& p_shape) { contained_shape_tile_ = p_shape; }

    /**
     * Returns a unique identification number for this room: a stable hash of
     * the room shape and the room layer. Port of Java
     * IncompleteFreeSpaceExpansionRoom.getId().
     */
    int get_room_id() const override
    {
        return static_cast<int>(31 * get_shape().get_id() + get_layer());
    }

private:
    geometry::planar::Simplex shape_tile_;
    geometry::planar::Simplex contained_shape_tile_;
};

class ExpansionDoor : public ExpandableObject
{
public:
    ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room, int p_dimension);

    /**
     * Creates a door between p_first_room and p_second_room with the dimension
     * of the intersection of the room shapes. Port of the Java
     * ExpansionDoor(ExpansionRoom, ExpansionRoom) constructor.
     */
    ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room);

    ExpansionRoom* other_room(ExpansionRoom* p_room);
    CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) override;

    int get_dimension() const override { return dimension_; }
    geometry::planar::IntBox get_shape() const override;

    int maze_search_element_count() const override;
    MazeSearchElement* get_maze_search_element(int p_no) override;
    void reset() override;

    /**
     * Returns a unique identification number for this door: a stable,
     * order-independent combination of the room ids of both sides. Port of
     * Java ExpansionDoor.getId().
     */
    int64_t get_id() const override;

    std::vector<geometry::planar::FloatLine> get_section_segments(double p_offset);

    ExpansionRoom* first_room_ = nullptr;
    ExpansionRoom* second_room_ = nullptr;

private:
    void allocate_sections(int p_count);

    int dimension_ = 1;
    std::vector<MazeSearchElement> section_arr_;
};

class TargetItemExpansionDoor : public ExpansionDoor
{
public:
    /**
     * Creates the target door of p_room to p_item. The door shape is the
     * intersection of the bounding box of the p_tree_entry_no-th tree shape
     * of p_item with the shape of p_room. Mirrors the shape computation of
     * the Java TargetItemExpansionDoor constructor (the C++ IntBox rooms
     * cannot represent the exact Simplex intersection).
     */
    TargetItemExpansionDoor(CompleteExpansionRoom* p_room, board::Item* p_item, int p_tree_entry_no,
        board::ShapeSearchTree* p_tree);
    virtual ~TargetItemExpansionDoor() = default;

geometry::planar::IntBox get_shape() const override;

    /**
     * A target door has exactly one maze search element, which the Java
     * TargetItemExpansionDoor creates in its constructor. Port of Java
     * TargetItemExpansionDoor.mazeSearchElementCount(),
     * getMazeSearchElement() and reset().
     */
    int maze_search_element_count() const override { return 1; }
    MazeSearchElement* get_maze_search_element(int p_no) override;
    void reset() override;

    /**
     * Returns a unique identification number for this target door: a hash of
     * the target item id and the room id. Port of Java
     * TargetItemExpansionDoor.getId().
     */
    int64_t get_id() const override;

    board::Item* item = nullptr;
    int tree_entry_no = 0;
    bool is_target_door() const { return true; }

    /**
     * Returns true if the target item of this door is a destination item of
     * the current autoroute connection. Port of Java
     * TargetItemExpansionDoor.isDestinationDoor(): the C++
     * ItemAutorouteInfo has no isDestination(), so the inverse of
     * is_start_info() is used, exactly as in Java.
     */
    bool is_destination_door() const;

    ExpansionRoom* get_room() const { return room_; }

    /**
     * The complete room of this target door. Port of the Java
     * TargetItemExpansionDoor.room field, which is a CompleteExpansionRoom.
     */
    CompleteExpansionRoom* get_complete_room() const { return room_; }

private:
    CompleteExpansionRoom* room_;
    geometry::planar::IntBox shape_;
    MazeSearchElement maze_search_info_;
};

} // namespace freerouting::autoroute
