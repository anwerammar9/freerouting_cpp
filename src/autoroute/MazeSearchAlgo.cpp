#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/AutorouteControl.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "datastructures/Stoppable.h"
#include "logger/FRLogger.h"

#include <cmath>
#include <algorithm>

namespace freerouting::autoroute {
namespace fp = freerouting::geometry::planar;

MazeSearchAlgo::MazeSearchAlgo(board::RoutingBoard* p_board, datastructures::Stoppable* p_stoppable)
    : board_(p_board), stoppable_(p_stoppable)
{
}

MazeSearchAlgo::~MazeSearchAlgo() = default;

std::unique_ptr<MazeSearchAlgo> MazeSearchAlgo::get_instance(board::RoutingBoard* p_board, datastructures::Stoppable* p_stoppable)
{
    return std::make_unique<MazeSearchAlgo>(p_board, p_stoppable);
}

bool MazeSearchAlgo::init(const std::set<board::Item*>& p_start_items,
                          const std::set<board::Item*>& p_dest_items,
                          board::ShapeSearchTree* p_search_tree)
{
    if (p_start_items.empty() || p_dest_items.empty())
    {
        logger::FRLogger::instance().warn("MazeSearchAlgo.init: empty start or destination set");
        return false;
    }

    initialized_ = true;
    search_tree_ = p_search_tree;
    
    start_items_ = p_start_items;
    dest_items_ = p_dest_items;
    
    destination_distance_ = std::make_unique<DestinationDistance>(control_.trace_costs, control_.layer_active, control_.min_normal_via_cost, 0.5);
    
    for (auto* item : p_start_items)
    {
        if (!item) continue;
            
        auto bbox = item->get_bounding_box();
        if (bbox != fp::IntBox::EMPTY)
        {
            int center_x = (bbox.ll.x + bbox.ur.x) / 2;
            int center_y = (bbox.ll.y + bbox.ur.y) / 2;
            
            MazeListElement start_elem = MazeListElement::obtain(
                nullptr, 0,
                nullptr, 0,
                0.0,
                0.0,
                nullptr,
                center_x, center_y,
                false,
                MazeSearchElement::Adjustment::NONE,
                false);
            
            expansion_list_.push(start_elem);
        }
    }
    
    for (auto* item : p_dest_items)
    {
        if (!item) continue;
        auto bbox = item->get_bounding_box();
        if (bbox != fp::IntBox::EMPTY && !item->get_net_nos().empty())
        {
            destination_distance_->join(bbox, item->get_layer());
        }
    }
    
    return true;
}

bool MazeSearchAlgo::is_stop_requested() const
{
    if (stoppable_ && stoppable_->is_stop_requested())
    {
        return true;
    }
    return false;
}

MazeSearchAlgo::Result MazeSearchAlgo::find_connection()
{
    if (!initialized_) return Result();
    
    int iteration = 0;
    while (!expansion_list_.empty())
    {
        iteration++;
        if (is_stop_requested()) break;
        
        auto element = expansion_list_.top();
        expansion_list_.pop();
        
        if (is_destination(element.door))
        {
            Result result;
            result.destination_door = element.door;
            result.section_no_of_destination_door = element.section_no_of_door;
            return result;
        }
        
        expand_to_neighbours(element);
    }
    
    return Result();
}

bool MazeSearchAlgo::is_destination(ExpansionDoor* p_door) const
{
    if (!p_door) return false;
    
    auto* target_door = dynamic_cast<TargetItemExpansionDoor*>(p_door);
    if (target_door)
    {
        for (auto* dest_item : dest_items_)
        {
            if (target_door->item == dest_item)
                return true;
        }
    }
    return false;
}

bool MazeSearchAlgo::occupy_next_element()
{
    if (expansion_list_.empty()) return false;
    auto element = expansion_list_.top();
    expansion_list_.pop();
    expand_to_neighbours(element);
    return !expansion_list_.empty();
}

void MazeSearchAlgo::expand_to_neighbours(const MazeListElement& p_element)
{
    if (p_element.next_room)
    {
        expand_to_room_neighbours(p_element.next_room, p_element.door, p_element.section_no_of_door,
                                   p_element.expansion_value, p_element.room_ripped, p_element.adjustment);
    }
}

void MazeSearchAlgo::expand_to_room_neighbours(ExpansionRoom* p_room, ExpansionDoor* p_backtrack_door,
                                               int p_section_no_of_backtrack_door, double p_expansion_value,
                                               bool p_room_ripped, MazeSearchElement::Adjustment p_adjustment)
{
    if (!p_room) return;

    std::vector<ExpansionDoor*> doors = p_room->get_doors();
    int layer = p_room->get_layer();

    for (auto* door : doors)
    {
        if (door == p_backtrack_door) continue;

        int section_count = door->maze_search_element_count();
        auto door_shape = door->get_shape();
        
        for (int i = 0; i < section_count; ++i)
        {
            auto* maze_elem = door->get_maze_search_element(i);
            if (!maze_elem || maze_elem->is_occupied) continue;

            double dist = 100.0; // Simplified
            
            double new_expansion_value = p_expansion_value + dist;
            double heuristic = destination_distance_->calculate(door_shape, layer);
            double sorting_value = new_expansion_value + heuristic;

            maze_elem->is_occupied = true;
            maze_elem->backtrack_door = p_backtrack_door;
            maze_elem->section_no_of_backtrack_door = p_section_no_of_backtrack_door;
            maze_elem->room_ripped = p_room_ripped;
            maze_elem->adjustment = p_adjustment;

            auto next_room = door->other_room(p_room);
            
            MazeListElement next_elem = MazeListElement::obtain(
                door, i,
                p_backtrack_door, p_section_no_of_backtrack_door,
                new_expansion_value,
                sorting_value,
                next_room,
                (door_shape.ll.x + door_shape.ur.x) / 2.0,
                (door_shape.ll.y + door_shape.ur.y) / 2.0,
                p_room_ripped,
                p_adjustment,
                false);
            
            expansion_list_.push(next_elem);
        }
    }
}

std::vector<ExpansionDoor*> MazeSearchAlgo::reconstruct_path(const Result& p_result)
{
    std::vector<ExpansionDoor*> path;
    if (!p_result.destination_door) return path;

    ExpansionDoor* curr_door = p_result.destination_door;
    int curr_section = p_result.section_no_of_destination_door;

    while (curr_door)
    {
        path.push_back(curr_door);
        auto* maze_elem = curr_door->get_maze_search_element(curr_section);
        if (!maze_elem) break;

        curr_door = maze_elem->backtrack_door;
        curr_section = maze_elem->section_no_of_backtrack_door;
    }

    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace freerouting::autoroute