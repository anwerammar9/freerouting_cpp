#include "autoroute/Sorted45DegreeRoomNeighbours.h"

#include <algorithm>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "autoroute/SortedRoomNeighbours.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "board/ShapeSearchTree.h"
#include "geometry/planar/Limits.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

namespace fp = geometry::planar;

namespace {

/**
 * Java IntOctagon.isEmpty() is an identity check against the canonical EMPTY
 * octagon, while the C++ IntOctagon::is_empty() reports every 1- or
 * 0-dimensional octagon as empty. Java dimension() of an octagon is derived
 * from the 8 border values only, so the two Java methods are reproduced here to
 * keep the neighbour calculation of the 45 degree rooms identical to Java.
 */
bool java_is_empty(const fp::IntOctagon& p_oct)
{
    return p_oct == fp::IntOctagon::EMPTY;
}

/** Port of Java IntOctagon.dimension(). */
int java_dimension(const fp::IntOctagon& p_oct)
{
    if (p_oct.right > p_oct.left && p_oct.top > p_oct.bottom && p_oct.right_bottom > p_oct.left_top
        && p_oct.right_top > p_oct.left_bottom)
    {
        return 2;
    }
    if (p_oct.right == p_oct.left && p_oct.top == p_oct.bottom)
    {
        return 0;
    }
    return 1;
}

/** Human readable description of a bounding box, used in the trace messages. */
std::string describe_bounds(const fp::IntBox& p_bounds)
{
    return "IntBox" + p_bounds.to_string();
}

/**
 * Name of the physical shape class of p_shape, used in the trace messages
 * (Java getClass().getSimpleName()).
 */
std::string describe_shape_type(const fp::Simplex& p_shape)
{
    return p_shape.is_IntBox() ? "IntBox" : (p_shape.is_IntOctagon() ? "IntOctagon" : "Simplex");
}

} // namespace

// ============================== Sorted45DegreeRoomNeighbour ==============================

Sorted45DegreeRoomNeighbour::Sorted45DegreeRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
    const fp::IntOctagon& p_shape, const fp::IntOctagon& p_intersection, const fp::IntOctagon& p_room_shape,
    std::array<bool, 8>* p_edge_interior_touches_obstacle)
    : search_tree_object(p_search_tree_object)
    , shape(p_shape)
    , intersection(p_intersection)
    , room_shape_(p_room_shape)
    , edge_interior_touches_obstacle_(p_edge_interior_touches_obstacle)
{
    // The first touching side is the first side of the room shape, where the
    // neighbour shape touches. If the room shape is contained in the neighbour
    // shape, no side is touched at all.
    if (intersection.bottom == room_shape_.bottom && intersection.left_bottom > room_shape_.left_bottom)
    {
        first_touching_side = 0;
    }
    else if (intersection.right_bottom == room_shape_.right_bottom && intersection.bottom > room_shape_.bottom)
    {
        first_touching_side = 1;
    }
    else if (intersection.right == room_shape_.right && intersection.right_bottom < room_shape_.right_bottom)
    {
        first_touching_side = 2;
    }
    else if (intersection.right_top == room_shape_.right_top && intersection.right < room_shape_.right)
    {
        first_touching_side = 3;
    }
    else if (intersection.top == room_shape_.top && intersection.right_top < room_shape_.right_top)
    {
        first_touching_side = 4;
    }
    else if (intersection.left_top == room_shape_.left_top && intersection.top < room_shape_.top)
    {
        first_touching_side = 5;
    }
    else if (intersection.left == room_shape_.left && intersection.left_top > room_shape_.left_top)
    {
        first_touching_side = 6;
    }
    else if (intersection.left_bottom == room_shape_.left_bottom && intersection.left > room_shape_.left)
    {
        first_touching_side = 7;
    }
    else
    {
        // the room shape may be contained in the neighbour shape
        first_touching_side = -1;
        last_touching_side = -1;
        return;
    }

    if (intersection.left_bottom == room_shape_.left_bottom && intersection.bottom > room_shape_.bottom)
    {
        last_touching_side = 7;
    }
    else if (intersection.left == room_shape_.left && intersection.left_bottom > room_shape_.left_bottom)
    {
        last_touching_side = 6;
    }
    else if (intersection.left_top == room_shape_.left_top && intersection.left > room_shape_.left)
    {
        last_touching_side = 5;
    }
    else if (intersection.top == room_shape_.top && intersection.left_top > room_shape_.left_top)
    {
        last_touching_side = 4;
    }
    else if (intersection.right_top == room_shape_.right_top && intersection.top < room_shape_.top)
    {
        last_touching_side = 3;
    }
    else if (intersection.right == room_shape_.right && intersection.right_top < room_shape_.right_top)
    {
        last_touching_side = 2;
    }
    else if (intersection.right_bottom == room_shape_.right_bottom && intersection.right < room_shape_.right)
    {
        last_touching_side = 1;
    }
    else if (intersection.bottom == room_shape_.bottom && intersection.right_bottom < room_shape_.right_bottom)
    {
        last_touching_side = 0;
    }
    else
    {
        // the room shape may be contained in the neighbour shape
        last_touching_side = -1;
        return;
    }

    mark_touching_sides();
}

void Sorted45DegreeRoomNeighbour::mark_touching_sides()
{
    if (edge_interior_touches_obstacle_ == nullptr)
    {
        return;
    }
    int next_side_no = first_touching_side;
    for (;;)
    {
        int current_side_index = next_side_no;
        next_side_no = (next_side_no + 1) % 8;
        if (!(*edge_interior_touches_obstacle_)[current_side_index])
        {
            bool touch_only_at_corner = false;
            if (current_side_index == first_touching_side)
            {
                if (intersection.corner(current_side_index) == room_shape_.corner(next_side_no))
                {
                    touch_only_at_corner = true;
                }
            }
            if (current_side_index == last_touching_side)
            {
                if (intersection.corner(next_side_no) == room_shape_.corner(current_side_index))
                {
                    touch_only_at_corner = true;
                }
            }
            if (!touch_only_at_corner)
            {
                (*edge_interior_touches_obstacle_)[current_side_index] = true;
            }
        }
        if (current_side_index == last_touching_side)
        {
            break;
        }
    }
}

int Sorted45DegreeRoomNeighbour::compare_to(const Sorted45DegreeRoomNeighbour& p_other) const
{
    if (first_touching_side > p_other.first_touching_side)
    {
        return 1;
    }
    if (first_touching_side < p_other.first_touching_side)
    {
        return -1;
    }

    // now the first touch of this and other is at the same side
    const fp::IntOctagon& is1 = intersection;
    const fp::IntOctagon& is2 = p_other.intersection;
    int64_t cmp_value = 0;

    switch (first_touching_side)
    {
    case 0:
        cmp_value = is1.corner_x(0) - is2.corner_x(0);
        break;
    case 1:
        cmp_value = is1.corner_x(1) - is2.corner_x(1);
        break;
    case 2:
        cmp_value = is1.corner_y(2) - is2.corner_y(2);
        break;
    case 3:
        cmp_value = is1.corner_y(3) - is2.corner_y(3);
        break;
    case 4:
        cmp_value = is2.corner_x(4) - is1.corner_x(4);
        break;
    case 5:
        cmp_value = is2.corner_x(5) - is1.corner_x(5);
        break;
    case 6:
        cmp_value = is2.corner_y(6) - is1.corner_y(6);
        break;
    case 7:
        cmp_value = is2.corner_y(7) - is1.corner_y(7);
        break;
    default:
        logger::FRLogger::instance().warn(
            "Sorted45DegreeRoomNeighbour::compare_to: first_touching_side out of range");
        return 0;
    }

    if (cmp_value == 0)
    {
        // The first touching points of this neighbour and other with the room
        // shape are equal. Compare the last touching points.
        int this_touching_side_diff = (last_touching_side - first_touching_side + 8) % 8;
        int other_touching_side_diff = (p_other.last_touching_side - p_other.first_touching_side + 8) % 8;
        if (this_touching_side_diff > other_touching_side_diff)
        {
            return 1;
        }
        if (this_touching_side_diff < other_touching_side_diff)
        {
            return -1;
        }
        // now the last touch of this and other is at the same side
        switch (last_touching_side)
        {
        case 0:
            cmp_value = is1.corner_x(1) - is2.corner_x(1);
            break;
        case 1:
            cmp_value = is1.corner_x(2) - is2.corner_x(2);
            break;
        case 2:
            cmp_value = is1.corner_y(3) - is2.corner_y(3);
            break;
        case 3:
            cmp_value = is1.corner_y(4) - is2.corner_y(4);
            break;
        case 4:
            cmp_value = is2.corner_x(5) - is1.corner_x(5);
            break;
        case 5:
            cmp_value = is2.corner_x(6) - is1.corner_x(6);
            break;
        case 6:
            cmp_value = is2.corner_y(7) - is1.corner_y(7);
            break;
        case 7:
            cmp_value = is2.corner_y(0) - is1.corner_y(0);
            break;
        default:
            break;
        }
    }
    if (cmp_value == 0)
    {
        // Deterministic tie-breaker for identical geometry
        int this_id = search_tree_object != nullptr ? search_tree_object->get_id() : 0;
        int other_id = p_other.search_tree_object != nullptr ? p_other.search_tree_object->get_id() : 0;
        cmp_value = static_cast<int64_t>(this_id) - static_cast<int64_t>(other_id);
    }
    return static_cast<int>(cmp_value);
}

// ============================== Sorted45DegreeRoomNeighbours ==============================

Sorted45DegreeRoomNeighbours::Sorted45DegreeRoomNeighbours(ExpansionRoom* p_from_room,
    CompleteExpansionRoom* p_completed_room)
    : from_room_(p_from_room)
    , completed_room_(p_completed_room)
    , room_shape_(p_completed_room->get_shape_tile().bounding_octagon())
{
    edge_interior_touches_obstacle_.fill(false);
}

CompleteExpansionRoom* Sorted45DegreeRoomNeighbours::calculate(ExpansionRoom* p_room,
    AutorouteEngine* p_autoroute_engine)
{
    // Port of Java Sorted45DegreeRoomNeighbours.calculate().
    if (p_room == nullptr || p_autoroute_engine == nullptr)
    {
        return nullptr;
    }
    const int net_no = p_autoroute_engine->get_net_no();
    board::ShapeSearchTree* autoroute_search_tree = p_autoroute_engine->get_autoroute_search_tree();
    if (autoroute_search_tree == nullptr)
    {
        return nullptr;
    }

    std::unique_ptr<Sorted45DegreeRoomNeighbours> room_neighbours =
        calculate_neighbours(p_room, net_no, autoroute_search_tree, p_autoroute_engine->generate_room_id_no());
    if (room_neighbours == nullptr)
    {
        // Unexpected expansion room type (already reported).
        return nullptr;
    }

    // Check, that each side of the room shape has at least one touching
    // neighbour. Otherwise, improve the room shape by enlarging.
    const bool edge_removed = room_neighbours->try_remove_edge_line(net_no, autoroute_search_tree);
    CompleteExpansionRoom* result = room_neighbours->get_completed_room();
    if (edge_removed)
    {
        p_autoroute_engine->remove_all_doors(result);
        return calculate(p_room, p_autoroute_engine);
    }

    // Now calculate the new incomplete rooms together with the doors between
    // this room and the sorted neighbours.
    if (room_neighbours->sorted_neighbours_.empty())
    {
        if (dynamic_cast<ObstacleExpansionRoom*>(result) != nullptr)
        {
            room_neighbours->calculate_edge_incomplete_rooms_of_obstacle_expansion_room(
                0, 7, p_autoroute_engine);
        }
    }
    else
    {
        room_neighbours->calculate_new_incomplete_rooms(p_autoroute_engine);
    }

    // A newly created complete free space room is owned by this instance, so its
    // ownership is transferred to the caller. If the from room was an obstacle
    // room, the completed room is the from room itself.
    CompleteFreeSpaceExpansionRoom* created_room = room_neighbours->release_completed_room();
    return created_room != nullptr ? static_cast<CompleteExpansionRoom*>(created_room) : result;
}

std::unique_ptr<Sorted45DegreeRoomNeighbours> Sorted45DegreeRoomNeighbours::calculate_neighbours(ExpansionRoom* p_room,
    int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no)
{
    const fp::Simplex room_shape_simplex = p_room->get_shape_tile();
    const fp::IntOctagon room_oct = room_shape_simplex.bounding_octagon();

    CompleteExpansionRoom* completed_room = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room;
    if (dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(p_room) != nullptr)
    {
        owned_completed_room = std::make_unique<CompleteFreeSpaceExpansionRoom>(
            p_room->get_layer(), p_room_id_no, room_shape_simplex);
        completed_room = owned_completed_room.get();
    }
    else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(p_room))
    {
        completed_room = obstacle_room;
    }
    else
    {
        logger::FRLogger::instance().warn("Sorted45DegreeRoomNeighbours::calculate_neighbours: "
                                          "unexpected expansion room type");
        return nullptr;
    }

    std::unique_ptr<Sorted45DegreeRoomNeighbours> result(
        new Sorted45DegreeRoomNeighbours(p_room, completed_room));
    result->owned_completed_room_ = std::move(owned_completed_room);
    std::vector<datastructures::ShapeTree::TreeEntry> overlapping_objects;
    p_autoroute_search_tree->overlapping_tree_entries(room_shape_simplex, p_room->get_layer(), overlapping_objects);

    // Sort the overlapping objects deterministically to ensure parity with v1.9.
    std::sort(overlapping_objects.begin(), overlapping_objects.end(),
        [](const datastructures::ShapeTree::TreeEntry& p_e1, const datastructures::ShapeTree::TreeEntry& p_e2)
        {
            auto* object1 = dynamic_cast<board::SearchTreeObject*>(p_e1.object);
            auto* object2 = dynamic_cast<board::SearchTreeObject*>(p_e2.object);
            int id1 = object1 != nullptr ? object1->get_id() : 0;
            int id2 = object2 != nullptr ? object2->get_id() : 0;
            int id_diff = id1 - id2;
            if (id_diff != 0)
            {
                return id_diff < 0;
            }
            return p_e1.shape_index_in_object < p_e2.shape_index_in_object;
        });

    // Calculate the touching neighbour objects and sort them in counterclock
    // sense around the border of the room shape.
    for (const datastructures::ShapeTree::TreeEntry& current_entry : overlapping_objects)
    {
        auto* current_object = dynamic_cast<board::SearchTreeObject*>(current_entry.object);
        if (current_object == nullptr || static_cast<const void*>(current_object) == static_cast<const void*>(p_room))
        {
            continue;
        }
        if (auto* free_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(completed_room))
        {
            if (!current_object->is_trace_obstacle(p_net_no))
            {
                // the target doors of a free space room are calculated
                // immediately, because the shape of the room does not change
                // anymore
                free_room->calculate_target_doors(current_entry, p_net_no, p_autoroute_search_tree);
                continue;
            }
        }
        const fp::TileShape* current_shape_ptr =
            current_object->get_tree_shape(*p_autoroute_search_tree, current_entry.shape_index_in_object);
        if (current_shape_ptr == nullptr)
        {
            continue;
        }
        const fp::IntOctagon current_oct = current_shape_ptr->to_Simplex().bounding_octagon();
        const fp::IntOctagon intersection = room_oct.intersection(current_oct);
        int dimension = java_dimension(intersection);
        auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(completed_room);
        if (dimension > 1 && obstacle_room != nullptr)
        {
            // only an obstacle expansion room may have a 2-dimensional overlap
            if (auto* current_item = dynamic_cast<board::Item*>(current_object))
            {
                if (current_item->is_routable())
                {
                    auto item_info = current_item->get_autoroute_info();
                    ObstacleExpansionRoom* current_overlap_room =
                        item_info->get_expansion_room(current_entry.shape_index_in_object, p_autoroute_search_tree);
                    obstacle_room->create_overlap_door(current_overlap_room);
                }
            }
            continue;
        }
        if (dimension < 0)
        {
            // may happen at a corner from 2 diagonal lines with non integer
            // coordinates (--.5, ---.5).
            continue;
        }
        result->add_sorted_neighbour(current_object, current_oct, intersection);
        if (dimension > 0)
        {
            // make sure, that there is a door to the neighbour room.
            ExpansionRoom* neighbour_room = nullptr;
            if (auto* neighbour_expansion_room = dynamic_cast<ExpansionRoom*>(current_object))
            {
                neighbour_room = neighbour_expansion_room;
            }
            else if (auto* current_item = dynamic_cast<board::Item*>(current_object))
            {
                if (current_item->is_routable())
                {
                    // expand the item for ripup and pushing purposes
                    auto item_info = current_item->get_autoroute_info();
                    neighbour_room =
                        item_info->get_expansion_room(current_entry.shape_index_in_object, p_autoroute_search_tree);
                }
            }
            if (neighbour_room != nullptr)
            {
                if (SortedRoomNeighbours::insert_door_ok(completed_room, neighbour_room, intersection))
                {
                    auto* new_door = new ExpansionDoor(completed_room, neighbour_room, 1);
                    neighbour_room->add_door(new_door);
                    completed_room->add_door(new_door);
                }
            }
        }
    }
    return result;
}

void Sorted45DegreeRoomNeighbours::add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
    const fp::IntOctagon& p_neighbour_shape, const fp::IntOctagon& p_intersection)
{
    Sorted45DegreeRoomNeighbour new_neighbour(
        p_search_tree_object, p_neighbour_shape, p_intersection, room_shape_, &edge_interior_touches_obstacle_);
    if (new_neighbour.last_touching_side < 0)
    {
        // The sorting of the neighbour failed, because the room shape is
        // contained in the neighbour shape.
        return;
    }
    // Java uses a TreeSet, so a neighbour comparing equal to an existing one is
    // not inserted twice.
    auto position = std::lower_bound(sorted_neighbours_.begin(), sorted_neighbours_.end(), new_neighbour,
        [](const Sorted45DegreeRoomNeighbour& p_a, const Sorted45DegreeRoomNeighbour& p_b)
        { return p_a.compare_to(p_b) < 0; });
    if (position != sorted_neighbours_.end() && position->compare_to(new_neighbour) == 0)
    {
        return;
    }
    sorted_neighbours_.insert(position, std::move(new_neighbour));
}

CompleteFreeSpaceExpansionRoom* Sorted45DegreeRoomNeighbours::release_completed_room()
{
    if (!owned_completed_room_)
    {
        return nullptr;
    }
    CompleteFreeSpaceExpansionRoom* result = owned_completed_room_.get();
    owned_completed_room_.release();
    completed_room_ = nullptr;
    return result;
}

fp::IntOctagon Sorted45DegreeRoomNeighbours::remove_not_touching_border_lines(const fp::IntOctagon& p_room_oct,
    const std::array<bool, 8>& p_edge_interior_touches_obstacle)
{
    int64_t left_x = p_edge_interior_touches_obstacle[6] ? p_room_oct.left : -fp::Limits::CRIT_INT;
    int64_t bottom_y = p_edge_interior_touches_obstacle[0] ? p_room_oct.bottom : -fp::Limits::CRIT_INT;
    int64_t right_x = p_edge_interior_touches_obstacle[2] ? p_room_oct.right : fp::Limits::CRIT_INT;
    int64_t top_y = p_edge_interior_touches_obstacle[4] ? p_room_oct.top : fp::Limits::CRIT_INT;
    int64_t upper_left_diagonal_x =
        p_edge_interior_touches_obstacle[5] ? p_room_oct.left_top : -fp::Limits::CRIT_INT;
    int64_t lower_right_diagonal_x =
        p_edge_interior_touches_obstacle[1] ? p_room_oct.right_bottom : fp::Limits::CRIT_INT;
    int64_t lower_left_diagonal_x =
        p_edge_interior_touches_obstacle[7] ? p_room_oct.left_bottom : -fp::Limits::CRIT_INT;
    int64_t upper_right_diagonal_x =
        p_edge_interior_touches_obstacle[3] ? p_room_oct.right_top : fp::Limits::CRIT_INT;

    return fp::IntOctagon(left_x, bottom_y, right_x, top_y, upper_left_diagonal_x, lower_right_diagonal_x,
        lower_left_diagonal_x, upper_right_diagonal_x)
        .normalize();
}

void Sorted45DegreeRoomNeighbours::calculate_edge_incomplete_rooms_of_obstacle_expansion_room(int p_from_side_index,
    int p_to_side_index, AutorouteEngine* p_autoroute_engine)
{
    if (dynamic_cast<ObstacleExpansionRoom*>(from_room_) == nullptr)
    {
        logger::FRLogger::instance().warn("Sorted45DegreeRoomNeighbours"
                                           "::calculate_edge_incomplete_rooms_of_obstacle_expansion_room: "
                                           "ObstacleExpansionRoom expected for from_room");
        return;
    }
    const fp::IntOctagon board_bounding_oct = p_autoroute_engine->board()->get_bounding_box().bounding_octagon();

    fp::IntPoint current_corner = room_shape_.corner(p_from_side_index);
    int current_side_index = p_from_side_index;
    for (;;)
    {
        int next_side_no = (current_side_index + 1) % 8;
        fp::IntPoint next_corner = room_shape_.corner(next_side_no);
        if (!(current_corner == next_corner))
        {
            int64_t left_x = board_bounding_oct.left;
            int64_t bottom_y = board_bounding_oct.bottom;
            int64_t right_x = board_bounding_oct.right;
            int64_t top_y = board_bounding_oct.top;
            int64_t upper_left_diagonal_x = board_bounding_oct.left_top;
            int64_t lower_right_diagonal_x = board_bounding_oct.right_bottom;
            int64_t lower_left_diagonal_x = board_bounding_oct.left_bottom;
            int64_t upper_right_diagonal_x = board_bounding_oct.right_top;
            switch (current_side_index)
            {
            case 0:
                top_y = room_shape_.bottom;
                break;
            case 1:
                upper_left_diagonal_x = room_shape_.right_bottom;
                break;
            case 2:
                left_x = room_shape_.right;
                break;
            case 3:
                lower_left_diagonal_x = room_shape_.right_top;
                break;
            case 4:
                bottom_y = room_shape_.top;
                break;
            case 5:
                lower_right_diagonal_x = room_shape_.left_top;
                break;
            case 6:
                right_x = room_shape_.left;
                break;
            case 7:
                upper_right_diagonal_x = room_shape_.left_bottom;
                break;
            default:
                logger::FRLogger::instance().warn("Sorted45DegreeRoomNeighbours"
                                                   "::calculate_edge_incomplete_rooms_of_obstacle_expansion_room: "
                                                   "current_side_index illegal");
                return;
            }
            insert_incomplete_room(p_autoroute_engine, left_x, bottom_y, right_x, top_y, upper_left_diagonal_x,
                lower_right_diagonal_x, lower_left_diagonal_x, upper_right_diagonal_x);
        }
        if (current_side_index == p_to_side_index)
        {
            break;
        }
        current_side_index = next_side_no;
    }
}

void Sorted45DegreeRoomNeighbours::insert_incomplete_room(AutorouteEngine* p_autoroute_engine, int64_t p_left_x,
    int64_t p_bottom_y, int64_t p_right_x, int64_t p_top_y, int64_t p_upper_left_diagonal_x,
    int64_t p_lower_right_diagonal_x, int64_t p_lower_left_diagonal_x, int64_t p_upper_right_diagonal_x)
{
    fp::IntOctagon new_incomplete_room_shape = fp::IntOctagon(p_left_x, p_bottom_y, p_right_x, p_top_y,
        p_upper_left_diagonal_x, p_lower_right_diagonal_x, p_lower_left_diagonal_x, p_upper_right_diagonal_x)
                                                  .normalize();
    if (java_dimension(new_incomplete_room_shape) != 2)
    {
        return;
    }
    const fp::IntOctagon new_contained_shape = room_shape_.intersection(new_incomplete_room_shape);
    if (java_is_empty(new_contained_shape))
    {
        return;
    }
    int door_dimension = java_dimension(new_contained_shape);
    if (door_dimension <= 0)
    {
        return;
    }
    IncompleteFreeSpaceExpansionRoom* new_room = p_autoroute_engine->add_incomplete_expansion_room(
        new_incomplete_room_shape.to_Simplex(), from_room_->get_layer(), new_contained_shape.to_Simplex());
    if (new_room == nullptr)
    {
        return;
    }
    auto* new_door = new ExpansionDoor(completed_room_, new_room, door_dimension);
    completed_room_->add_door(new_door);
    new_room->add_door(new_door);
}

bool Sorted45DegreeRoomNeighbours::try_remove_edge_line(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree)
{
    auto* current_incomplete_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(from_room_);
    if (current_incomplete_room == nullptr)
    {
        // Only the shape of an incomplete free space room may be enlarged.
        return false;
    }
    const fp::Simplex room_shape_simplex = current_incomplete_room->get_shape_tile();
    if (!room_shape_simplex.is_IntOctagon())
    {
        logger::FRLogger::instance().warn(
            "Sorted45DegreeRoomNeighbours::try_remove_edge_line: IntOctagon expected for room_shape type");
        return false;
    }
    const fp::IntOctagon room_oct = room_shape_simplex.to_IntOctagon();
    const double room_area = room_oct.area();

    bool try_remove_edge_lines = false;
    for (int i = 0; i < 8; i++)
    {
        if (!edge_interior_touches_obstacle_[i])
        {
            fp::FloatPoint prev_corner = room_shape_.corner_approx(i);
            fp::FloatPoint next_corner = room_shape_.corner_approx((i + 1) % 8);
            if (prev_corner.distance_square(next_corner) > 1)
            {
                try_remove_edge_lines = true;
                break;
            }
        }
    }

    if (!try_remove_edge_lines)
    {
        return false;
    }

    // Touching neighbour missing at the border line without obstacle contact.
    // Remove the border line and restart the algorithm.
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE start, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", room_bounds=" + describe_bounds(room_oct.bounding_box()));

    const fp::IntOctagon enlarged_oct = remove_not_touching_border_lines(room_oct, edge_interior_touches_obstacle_);
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE enlarged, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", enlarged_bounds=" + describe_bounds(enlarged_oct.bounding_box()));
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE contained, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", type=" + describe_shape_type(current_incomplete_room->get_contained_shape_tile())
        + ", bounds=" + describe_bounds(current_incomplete_room->get_contained_shape_tile().bounding_box()));

    // Insert the overlapping doors with complete free space expansion rooms into
    // the complete_shape call, to get the information about the objects to
    // ignore. The C++ ExpansionDoor has no door shape of its own, so the
    // intersection box of the door rooms is used, which is the door shape for a
    // 2-dimensional door.
    fp::IntBox ignore_shape_storage;
    const fp::TileShape* ignore_shape = nullptr;
    board::SearchTreeObject* ignore_object = nullptr;
    double max_door_area = 0;
    for (ExpansionDoor* current_door : completed_room_->get_doors())
    {
        if (current_door == nullptr || current_door->get_dimension() != 2)
        {
            continue;
        }
        auto* other_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(current_door->other_room(completed_room_));
        if (other_room == nullptr)
        {
            continue;
        }
        const fp::IntBox current_door_shape = current_door->get_shape();
        double current_door_area = current_door_shape.area();
        if (current_door_area > max_door_area)
        {
            max_door_area = current_door_area;
            ignore_shape_storage = current_door_shape;
            ignore_shape = &ignore_shape_storage;
            ignore_object = other_room;
        }
    }

    // Java ShapeSearchTree.completeShape() returns an empty result for a tree
    // without a root (an empty tree). The C++ complete_shape() uses the whole
    // board instead, so the empty tree is handled here to keep the behaviour of
    // the Java room enlargement.
    if (p_autoroute_search_tree == nullptr || p_autoroute_search_tree->size() == 0)
    {
        logger::FRLogger::instance().trace("AutorouteEngine.calculate_doors: 1 completed shape expected");
        return false;
    }

    const fp::Simplex contained_shape = current_incomplete_room->get_contained_shape_tile();
    board::ShapeSearchTree::CompleteShapeInput input;
    input.shape = &enlarged_oct;
    input.contained_shape = &contained_shape;
    input.layer = current_incomplete_room->get_layer();
    input.net_no = p_net_no;
    input.ignore_object = ignore_object;
    input.ignore_shape = ignore_shape;
    std::vector<board::ShapeSearchTree::CompleteShapePiece> new_rooms = p_autoroute_search_tree->complete_shape(input);
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE complete_shape, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", candidate_count=" + std::to_string(new_rooms.size()));
    if (new_rooms.size() != 1)
    {
        logger::FRLogger::instance().trace("AutorouteEngine.calculate_doors: 1 completed shape expected");
        return false;
    }
    const board::ShapeSearchTree::CompleteShapePiece& new_room = new_rooms.front();
    if (new_room.shape == nullptr || new_room.contained_shape == nullptr)
    {
        logger::FRLogger::instance().trace("AutorouteEngine.calculate_doors: 1 completed shape expected");
        return false;
    }
    // Check, that the area increases to prevent endless loop.
    if (new_room.shape->area() <= room_area)
    {
        return false;
    }
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE applied, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", old_bounds=" + describe_bounds(room_oct.bounding_box())
        + ", newBounds=" + describe_bounds(new_room.shape->bounding_box()));
    current_incomplete_room->set_shape_tile(new_room.shape->to_Simplex());
    current_incomplete_room->set_contained_shape_tile(new_room.contained_shape->to_Simplex());
    return true;
}

void Sorted45DegreeRoomNeighbours::calculate_new_incomplete_rooms_for_obstacle_expansion_room(
    const Sorted45DegreeRoomNeighbour& p_prev_neighbour, const Sorted45DegreeRoomNeighbour& p_next_neighbour,
    AutorouteEngine* p_autoroute_engine)
{
    int from_side_index = p_prev_neighbour.last_touching_side;
    int to_side_index = p_next_neighbour.first_touching_side;
    if (from_side_index == to_side_index && &p_prev_neighbour != &p_next_neighbour)
    {
        // no return in case of only 1 neighbour.
        return;
    }
    const fp::IntOctagon board_bounding_oct = p_autoroute_engine->board()->get_bounding_box().bounding_octagon();

    // insert the new incomplete room from p_prev_neighbour to the next corner of
    // the room shape.
    {
        int64_t left_x = board_bounding_oct.left;
        int64_t bottom_y = board_bounding_oct.bottom;
        int64_t right_x = board_bounding_oct.right;
        int64_t top_y = board_bounding_oct.top;
        int64_t upper_left_diagonal_x = board_bounding_oct.left_top;
        int64_t lower_right_diagonal_x = board_bounding_oct.right_bottom;
        int64_t lower_left_diagonal_x = board_bounding_oct.left_bottom;
        int64_t upper_right_diagonal_x = board_bounding_oct.right_top;
        switch (from_side_index)
        {
        case 0:
            top_y = room_shape_.bottom;
            upper_left_diagonal_x = p_prev_neighbour.intersection.right_bottom;
            break;
        case 1:
            upper_left_diagonal_x = room_shape_.right_bottom;
            left_x = p_prev_neighbour.intersection.right;
            break;
        case 2:
            left_x = room_shape_.right;
            lower_left_diagonal_x = p_prev_neighbour.intersection.right_top;
            break;
        case 3:
            lower_left_diagonal_x = room_shape_.right_top;
            bottom_y = p_prev_neighbour.intersection.top;
            break;
        case 4:
            bottom_y = room_shape_.top;
            lower_right_diagonal_x = p_prev_neighbour.intersection.left_top;
            break;
        case 5:
            lower_right_diagonal_x = room_shape_.left_top;
            right_x = p_prev_neighbour.intersection.left;
            break;
        case 6:
            right_x = room_shape_.left;
            upper_right_diagonal_x = p_prev_neighbour.intersection.left_bottom;
            break;
        case 7:
            upper_right_diagonal_x = room_shape_.left_bottom;
            top_y = p_prev_neighbour.intersection.bottom;
            break;
        default:
            break;
        }
        insert_incomplete_room(p_autoroute_engine, left_x, bottom_y, right_x, top_y, upper_left_diagonal_x,
            lower_right_diagonal_x, lower_left_diagonal_x, upper_right_diagonal_x);
    }

    // insert the new incomplete room from p_next_neighbour to the next corner of
    // the room shape.
    {
        int64_t left_x = board_bounding_oct.left;
        int64_t bottom_y = board_bounding_oct.bottom;
        int64_t right_x = board_bounding_oct.right;
        int64_t top_y = board_bounding_oct.top;
        int64_t upper_left_diagonal_x = board_bounding_oct.left_top;
        int64_t lower_right_diagonal_x = board_bounding_oct.right_bottom;
        int64_t lower_left_diagonal_x = board_bounding_oct.left_bottom;
        int64_t upper_right_diagonal_x = board_bounding_oct.right_top;
        switch (to_side_index)
        {
        case 0:
            top_y = room_shape_.bottom;
            upper_right_diagonal_x = p_next_neighbour.intersection.left_bottom;
            break;
        case 1:
            upper_left_diagonal_x = room_shape_.right_bottom;
            top_y = p_next_neighbour.intersection.bottom;
            break;
        case 2:
            left_x = room_shape_.right;
            upper_left_diagonal_x = p_next_neighbour.intersection.right_bottom;
            break;
        case 3:
            lower_left_diagonal_x = room_shape_.right_top;
            left_x = p_next_neighbour.intersection.right;
            break;
        case 4:
            bottom_y = room_shape_.top;
            lower_left_diagonal_x = p_next_neighbour.intersection.right_top;
            break;
        case 5:
            lower_right_diagonal_x = room_shape_.left_top;
            bottom_y = p_next_neighbour.intersection.top;
            break;
        case 6:
            right_x = room_shape_.left;
            lower_right_diagonal_x = p_next_neighbour.intersection.left_top;
            break;
        case 7:
            upper_right_diagonal_x = room_shape_.left_bottom;
            right_x = p_next_neighbour.intersection.left;
            break;
        default:
            break;
        }
        insert_incomplete_room(p_autoroute_engine, left_x, bottom_y, right_x, top_y, upper_left_diagonal_x,
            lower_right_diagonal_x, lower_left_diagonal_x, upper_right_diagonal_x);
    }

    // Insert the new incomplete rooms on the intermediate free sides of the
    // obstacle expansion room.
    int current_from_side_no = (from_side_index + 1) % 8;
    if (current_from_side_no == to_side_index)
    {
        return;
    }
    int current_to_side_no = (to_side_index + 7) % 8;
    calculate_edge_incomplete_rooms_of_obstacle_expansion_room(current_from_side_no, current_to_side_no,
        p_autoroute_engine);
}

void Sorted45DegreeRoomNeighbours::calculate_new_incomplete_rooms(AutorouteEngine* p_autoroute_engine)
{
    if (p_autoroute_engine == nullptr || completed_room_ == nullptr || sorted_neighbours_.empty())
    {
        return;
    }
    const fp::IntOctagon board_bounding_oct = p_autoroute_engine->board()->get_bounding_box().bounding_octagon();
    const bool from_room_is_obstacle = dynamic_cast<ObstacleExpansionRoom*>(from_room_) != nullptr;
    size_t prev_neighbour_index = sorted_neighbours_.size() - 1;
    if (from_room_is_obstacle && sorted_neighbours_.size() == 1)
    {
        // ObstacleExpansionRoom has only 1 neighbour
        calculate_new_incomplete_rooms_for_obstacle_expansion_room(
            sorted_neighbours_[prev_neighbour_index], sorted_neighbours_[prev_neighbour_index], p_autoroute_engine);
        return;
    }

    for (size_t next_neighbour_index = 0; next_neighbour_index < sorted_neighbours_.size(); next_neighbour_index++)
    {
        const Sorted45DegreeRoomNeighbour& prev_neighbour = sorted_neighbours_[prev_neighbour_index];
        const Sorted45DegreeRoomNeighbour& next_neighbour = sorted_neighbours_[next_neighbour_index];
        bool insert_incomplete_room_needed;

        if (dynamic_cast<ObstacleExpansionRoom*>(completed_room_) != nullptr && sorted_neighbours_.size() == 2)
        {
            // check, if this site is touching or open.
            const fp::IntOctagon intersection =
                next_neighbour.intersection.intersection(prev_neighbour.intersection);
            if (java_is_empty(intersection))
            {
                insert_incomplete_room_needed = true;
            }
            else if (java_dimension(intersection) >= 1)
            {
                insert_incomplete_room_needed = false;
            }
            else
            {
                // dimension = 0
                // touch at a corner of the room shape
                if (prev_neighbour.last_touching_side == next_neighbour.first_touching_side)
                {
                    // touch along the side of the room shape
                    insert_incomplete_room_needed = false;
                }
                else
                {
                    insert_incomplete_room_needed =
                        prev_neighbour.last_touching_side != (next_neighbour.first_touching_side + 1) % 8;
                }
            }
        }
        else
        {
            // the 2 neighbours do not touch
            insert_incomplete_room_needed = !next_neighbour.intersection.intersects(prev_neighbour.intersection);
        }

        if (insert_incomplete_room_needed)
        {
            // create a door to a new incomplete expansion room between the last
            // corner of the previous neighbour and the first corner of the
            // current neighbour
            if (from_room_is_obstacle && next_neighbour.first_touching_side != prev_neighbour.last_touching_side)
            {
                calculate_new_incomplete_rooms_for_obstacle_expansion_room(
                    prev_neighbour, next_neighbour, p_autoroute_engine);
            }
            else
            {
                int64_t lx = board_bounding_oct.left;
                int64_t ly = board_bounding_oct.bottom;
                int64_t rx = board_bounding_oct.right;
                int64_t uy = board_bounding_oct.top;
                int64_t ulx = board_bounding_oct.left_top;
                int64_t lrx = board_bounding_oct.right_bottom;
                int64_t llx = board_bounding_oct.left_bottom;
                int64_t urx = board_bounding_oct.right_top;

                const fp::IntOctagon& prev_intersection = prev_neighbour.intersection;
                const fp::IntOctagon& next_intersection = next_neighbour.intersection;

                switch (next_neighbour.first_touching_side)
                {
                case 0:
                    if (prev_intersection.left_bottom < next_intersection.left_bottom)
                    {
                        urx = next_intersection.left_bottom;
                        uy = prev_intersection.bottom;
                        if (prev_neighbour.last_touching_side == 0)
                        {
                            ulx = prev_intersection.right_bottom;
                        }
                    }
                    else if (prev_intersection.left_bottom > next_intersection.left_bottom)
                    {
                        rx = next_intersection.left;
                        urx = prev_intersection.left_bottom;
                    }
                    else
                    {
                        // prev_intersection.left_bottom == next_intersection.left_bottom
                        urx = next_intersection.left_bottom;
                    }
                    break;
                case 1:
                    if (prev_intersection.bottom < next_intersection.bottom)
                    {
                        uy = next_intersection.bottom;
                        ulx = prev_intersection.right_bottom;
                        if (prev_neighbour.last_touching_side == 1)
                        {
                            lx = prev_intersection.right;
                        }
                    }
                    else if (prev_intersection.bottom > next_intersection.bottom)
                    {
                        uy = prev_intersection.bottom;
                        urx = prev_intersection.left_bottom;
                    }
                    else
                    {
                        // prev_intersection.bottom == next_intersection.bottom
                        uy = next_intersection.bottom;
                    }
                    break;
                case 2:
                    if (prev_intersection.right_bottom > next_intersection.right_bottom)
                    {
                        ulx = next_intersection.right_bottom;
                        lx = prev_intersection.right;
                        if (prev_neighbour.last_touching_side == 2)
                        {
                            llx = prev_intersection.right_top;
                        }
                    }
                    else if (prev_intersection.right_bottom < next_intersection.right_bottom)
                    {
                        uy = prev_intersection.bottom;
                        ulx = prev_intersection.right_bottom;
                    }
                    else
                    {
                        // prev_intersection.right_bottom == next_intersection.right_bottom
                        ulx = next_intersection.right_bottom;
                    }
                    break;
                case 3:
                    if (prev_intersection.right > next_intersection.right)
                    {
                        lx = next_intersection.right;
                        llx = next_intersection.right_top;
                        if (prev_neighbour.last_touching_side == 3)
                        {
                            ly = prev_intersection.top;
                        }
                    }
                    else if (prev_intersection.right < next_intersection.right)
                    {
                        lx = prev_intersection.right;
                        ulx = prev_intersection.right_bottom;
                    }
                    else
                    {
                        // prev_intersection.right == next_intersection.right
                        lx = next_intersection.right;
                    }
                    break;
                case 4:
                    if (prev_intersection.right_top > next_intersection.right_top)
                    {
                        llx = next_intersection.right_top;
                        ly = prev_intersection.top;
                        if (prev_neighbour.last_touching_side == 4)
                        {
                            lrx = prev_intersection.left_top;
                        }
                    }
                    else if (prev_intersection.right_top < next_intersection.right_top)
                    {
                        lx = prev_intersection.right;
                        llx = prev_intersection.right_top;
                    }
                    else
                    {
                        // prev_intersection.right_top == next_intersection.right_top
                        llx = next_intersection.right_top;
                    }
                    break;
                case 5:
                    if (prev_intersection.top > next_intersection.top)
                    {
                        ly = next_intersection.top;
                        lrx = prev_intersection.left_top;
                        if (prev_neighbour.last_touching_side == 5)
                        {
                            rx = prev_intersection.left;
                        }
                    }
                    else if (prev_intersection.top < next_intersection.top)
                    {
                        ly = prev_intersection.top;
                        llx = prev_intersection.right_top;
                    }
                    else
                    {
                        // prev_intersection.top == next_intersection.top
                        ly = next_intersection.top;
                    }
                    break;
                case 6:
                    if (prev_intersection.left_top < next_intersection.left_top)
                    {
                        lrx = next_intersection.left_top;
                        rx = prev_intersection.left;
                        if (prev_neighbour.last_touching_side == 6)
                        {
                            urx = prev_intersection.left_bottom;
                        }
                    }
                    else if (prev_intersection.left_top > next_intersection.left_top)
                    {
                        ly = prev_intersection.top;
                        lrx = prev_intersection.left_top;
                    }
                    else
                    {
                        // prev_intersection.left_top == next_intersection.left_top
                        lrx = next_intersection.left_top;
                    }
                    break;
                case 7:
                    if (prev_intersection.left < next_intersection.left)
                    {
                        rx = next_intersection.left;
                        urx = next_intersection.left_bottom;
                        if (prev_neighbour.last_touching_side == 7)
                        {
                            uy = prev_intersection.bottom;
                        }
                    }
                    else if (prev_intersection.left > next_intersection.left)
                    {
                        rx = prev_intersection.left;
                        lrx = next_intersection.left_top;
                    }
                    else
                    {
                        // prev_intersection.left == next_intersection.left
                        rx = next_intersection.left;
                    }
                    break;
                default:
                    logger::FRLogger::instance().warn(
                        "Sorted45DegreeRoomNeighbours::calculate_new_incomplete_rooms: illegal touching side");
                    break;
                }
                insert_incomplete_room(p_autoroute_engine, lx, ly, rx, uy, ulx, lrx, llx, urx);
            }
        }
        prev_neighbour_index = next_neighbour_index;
    }
}

} // namespace freerouting::autoroute
