#pragma once

#include "autoroute/MazeSearchElement.h"
#include "autoroute/ExpandableObject.h"
#include "geometry/planar/IntBox.h"
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

class CompleteFreeSpaceExpansionRoom : public CompleteExpansionRoom
{
public:
    CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::IntBox& p_shape);
    virtual ~CompleteFreeSpaceExpansionRoom() = default;

    geometry::planar::IntBox get_shape() const override;

private:
    geometry::planar::IntBox shape_;
};

class ObstacleExpansionRoom : public CompleteExpansionRoom
{
public:
    ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, int p_layer, const geometry::planar::IntBox& p_shape);
    virtual ~ObstacleExpansionRoom() = default;

    geometry::planar::IntBox get_shape() const override;
    board::Item* get_item() const { return item_; }
    int get_index_in_item() const { return index_in_item_; }

private:
    board::Item* item_;
    int index_in_item_;
    geometry::planar::IntBox shape_;
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
    virtual ~IncompleteFreeSpaceExpansionRoom() = default;
};

class ExpansionDoor : public ExpandableObject
{
public:
    ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room, int p_dimension);

    ExpansionRoom* other_room(ExpansionRoom* p_room) override;
    CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) override;

    int get_dimension() const override { return dimension_; }
    geometry::planar::IntBox get_shape() const override;

    int maze_search_element_count() const override;
    MazeSearchElement* get_maze_search_element(int p_no) override;
    void reset() override;

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
    TargetItemExpansionDoor(CompleteExpansionRoom* p_room, board::Item* p_item, int p_tree_entry_no);
    virtual ~TargetItemExpansionDoor() = default;

    board::Item* item = nullptr;
    int tree_entry_no = 0;
    bool is_target_door() const { return true; }

    ExpansionRoom* get_room() const { return room_; }

private:
    ExpansionRoom* room_;
};

} // namespace freerouting::autoroute
