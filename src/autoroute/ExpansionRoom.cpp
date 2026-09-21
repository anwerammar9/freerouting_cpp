#include "autoroute/ExpansionRoom.h"
#include "geometry/planar/FloatLine.h"
#include "geometry/planar/IntBox.h"
#include "board/Item.h"
#include <cmath>
#include <algorithm>

namespace freerouting::autoroute {

CompleteExpansionRoom::CompleteExpansionRoom(int p_layer, int p_room_id)
    : layer_(p_layer), room_id_(p_room_id)
{
}

void CompleteExpansionRoom::add_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        doors_.push_back(p_door);
    }
}

std::vector<ExpansionDoor*> CompleteExpansionRoom::get_doors() const
{
    return doors_;
}

void CompleteExpansionRoom::clear_doors()
{
    doors_.clear();
}

void CompleteExpansionRoom::reset_doors()
{
    for (auto* door : doors_)
    {
        if (door)
        {
            door->reset();
        }
    }
}

bool CompleteExpansionRoom::door_exists(ExpansionRoom* p_other) const
{
    for (auto* door : doors_)
    {
        if (door)
        {
            if (door->other_room(p_other) != nullptr)
            {
                return true;
            }
        }
    }
    return false;
}

bool CompleteExpansionRoom::remove_door(ExpandableObject* p_door)
{
    auto it = std::find(doors_.begin(), doors_.end(), static_cast<ExpansionDoor*>(p_door));
    if (it != doors_.end())
    {
        doors_.erase(it);
        return true;
    }
    return false;
}

geometry::planar::IntBox CompleteExpansionRoom::get_shape() const
{
    return geometry::planar::IntBox::INFINITE;
}

void CompleteExpansionRoom::add_target_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        target_doors_.push_back(p_door);
    }
}

CompleteFreeSpaceExpansionRoom::CompleteFreeSpaceExpansionRoom(int p_layer, int p_room_id, const geometry::planar::IntBox& p_shape)
    : CompleteExpansionRoom(p_layer, p_room_id), shape_(p_shape)
{
}

geometry::planar::IntBox CompleteFreeSpaceExpansionRoom::get_shape() const
{
    return shape_;
}

int CompleteFreeSpaceExpansionRoom::tree_shape_count(const datastructures::ShapeTree& p_tree) const
{
    (void)p_tree;
    return 1;
}

const geometry::planar::TileShape* CompleteFreeSpaceExpansionRoom::get_tree_shape(
    const datastructures::ShapeTree& p_tree, int p_index) const
{
    (void)p_tree;
    if (p_index != 0)
    {
        return nullptr;
    }
    return &shape_;
}

void CompleteFreeSpaceExpansionRoom::set_search_tree_entries(
    std::vector<std::unique_ptr<datastructures::ShapeTree::Leaf>> p_entries, datastructures::ShapeTree& p_tree)
{
    (void)p_tree;
    tree_entries_ = std::move(p_entries);
}

int CompleteFreeSpaceExpansionRoom::compare_to(const datastructures::ShapeTree::Storable& p_other) const
{
    // Mirrors Java CompleteFreeSpaceExpansionRoom.compareTo(): rooms are
    // ordered by reversed id before any non-room object.
    const CompleteFreeSpaceExpansionRoom* other =
        dynamic_cast<const CompleteFreeSpaceExpansionRoom*>(&p_other);
    if (other == nullptr)
    {
        return -1;
    }
    return other->room_id_ - room_id_;
}

void CompleteFreeSpaceExpansionRoom::remove_from_tree(board::ShapeSearchTree& p_tree)
{
    std::vector<datastructures::ShapeTree::Leaf*> raw_entries;
    raw_entries.reserve(tree_entries_.size());
    for (auto& entry : tree_entries_)
    {
        raw_entries.push_back(entry.get());
    }
    static_cast<datastructures::ShapeTree&>(p_tree).remove(raw_entries);
    tree_entries_.clear();
}

ObstacleExpansionRoom::ObstacleExpansionRoom(board::Item* p_item, int p_index_in_item, int p_layer, const geometry::planar::IntBox& p_shape)
    : CompleteExpansionRoom(p_layer, 0), item_(p_item), index_in_item_(p_index_in_item), shape_(p_shape)
{
}

geometry::planar::IntBox ObstacleExpansionRoom::get_shape() const
{
    return shape_;
}

ExpansionDoor::ExpansionDoor(ExpansionRoom* p_first_room, ExpansionRoom* p_second_room, int p_dimension)
    : first_room_(p_first_room), second_room_(p_second_room), dimension_(p_dimension)
{
}

ExpansionRoom* ExpansionDoor::other_room(ExpansionRoom* p_room)
{
    if (p_room == first_room_)
        return second_room_;
    if (p_room == second_room_)
        return first_room_;
    return nullptr;
}

CompleteExpansionRoom* ExpansionDoor::other_room(CompleteExpansionRoom* p_room)
{
    ExpansionRoom* result = other_room(static_cast<ExpansionRoom*>(p_room));
    return dynamic_cast<CompleteExpansionRoom*>(result);
}

geometry::planar::IntBox ExpansionDoor::get_shape() const
{
    if (!first_room_ || !second_room_)
        return geometry::planar::IntBox::EMPTY;

    geometry::planar::IntBox first_shape = first_room_->get_shape();
    geometry::planar::IntBox second_shape = second_room_->get_shape();

    return first_shape.intersection(second_shape);
}

int ExpansionDoor::maze_search_element_count() const
{
    return static_cast<int>(section_arr_.size());
}

MazeSearchElement* ExpansionDoor::get_maze_search_element(int p_no)
{
    if (p_no >= 0 && p_no < static_cast<int>(section_arr_.size()))
    {
        return &section_arr_[p_no];
    }
    return nullptr;
}

void ExpansionDoor::reset()
{
    for (auto& elem : section_arr_)
    {
        elem.reset();
    }
}

void ExpansionDoor::allocate_sections(int p_count)
{
    section_arr_.resize(p_count);
}

std::vector<geometry::planar::FloatLine> ExpansionDoor::get_section_segments(double p_offset)
{
    double offset = p_offset + 2;

    geometry::planar::IntBox door_shape = get_shape();
    if (door_shape == geometry::planar::IntBox::EMPTY)
    {
        return {};
    }

    const double c_max_door_section_width = 10 * offset;

    if (dimension_ == 1)
    {
        int64_t dx = door_shape.ur.x - door_shape.ll.x;
        int64_t dy = door_shape.ur.y - door_shape.ll.y;
        int64_t length = std::sqrt(dx * dx + dy * dy);
        int section_count = static_cast<int>(length / c_max_door_section_width) + 1;

        allocate_sections(section_count);

        std::vector<geometry::planar::FloatLine> result;
        for (int i = 0; i < section_count; ++i)
        {
            double t1 = static_cast<double>(i) / section_count;
            double t2 = static_cast<double>(i + 1) / section_count;

            geometry::planar::FloatPoint a(door_shape.ll.x + dx * t1, door_shape.ll.y + dy * t1);
            geometry::planar::FloatPoint b(door_shape.ll.x + dx * t2, door_shape.ll.y + dy * t2);

            result.emplace_back(a, b);
        }
        return result;
    }
    else
    {
        int section_count = 1;
        allocate_sections(section_count);

        geometry::planar::FloatPoint center = geometry::planar::FloatPoint((door_shape.ll.x + door_shape.ur.x) / 2.0,
                                         (door_shape.ll.y + door_shape.ur.y) / 2.0);

        std::vector<geometry::planar::FloatLine> result;
        result.emplace_back(center, center);
        return result;
    }
}

IncompleteExpansionRoom::IncompleteExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape)
    : layer_(p_layer), room_id_(0), shape_(p_shape)
{
}

void IncompleteExpansionRoom::add_door(ExpansionDoor* p_door)
{
    if (p_door)
    {
        doors_.push_back(p_door);
    }
}

std::vector<ExpansionDoor*> IncompleteExpansionRoom::get_doors() const
{
    return doors_;
}

void IncompleteExpansionRoom::clear_doors()
{
    doors_.clear();
}

void IncompleteExpansionRoom::reset_doors()
{
    for (auto* door : doors_)
    {
        if (door)
        {
            door->reset();
        }
    }
}

bool IncompleteExpansionRoom::door_exists(ExpansionRoom* p_other) const
{
    for (auto* door : doors_)
    {
        if (door)
        {
            if (door->other_room(p_other) != nullptr)
            {
                return true;
            }
        }
    }
    return false;
}

bool IncompleteExpansionRoom::remove_door(ExpandableObject* p_door)
{
    auto it = std::find(doors_.begin(), doors_.end(), static_cast<ExpansionDoor*>(p_door));
    if (it != doors_.end())
    {
        doors_.erase(it);
        return true;
    }
    return false;
}

IncompleteFreeSpaceExpansionRoom::IncompleteFreeSpaceExpansionRoom(ExpansionRoom* p_source_room, int p_layer, const geometry::planar::IntBox& p_shape)
    : IncompleteExpansionRoom(p_source_room, p_layer, p_shape)
{
}

TargetItemExpansionDoor::TargetItemExpansionDoor(CompleteExpansionRoom* p_room, board::Item* p_item, int p_tree_entry_no)
    : ExpansionDoor(p_room, nullptr, 0), room_(p_room)
{
    item = p_item;
    tree_entry_no = p_tree_entry_no;
}

} // namespace freerouting::autoroute