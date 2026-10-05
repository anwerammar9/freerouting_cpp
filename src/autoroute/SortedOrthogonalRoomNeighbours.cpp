#include "autoroute/SortedOrthogonalRoomNeighbours.h"

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

/** Human readable description of a bounding box, used in the trace messages. */
std::string describe_bounds(const fp::IntBox& p_bounds)
{
    return "IntBox" + p_bounds.to_string();
}

} // namespace

// ============================== SortedOrthogonalRoomNeighbour ==============================

SortedOrthogonalRoomNeighbour::SortedOrthogonalRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
    const fp::IntBox& p_shape, const fp::IntBox& p_intersection, const fp::IntBox& p_room_shape,
    std::array<bool, 4>* p_edge_interior_touches_obstacle)
    : search_tree_object(p_search_tree_object)
    , shape(p_shape)
    , intersection(p_intersection)
    , room_shape_(p_room_shape)
    , edge_interior_touches_obstacle_(p_edge_interior_touches_obstacle)
{
    // The first touching side is the first side of the room shape, where the
    // neighbour shape touches. If the room shape is contained in the neighbour
    // shape, no side is touched at all.
    if (intersection.ll.y == room_shape_.ll.y && intersection.ll.x > room_shape_.ll.x)
    {
        first_touching_side = 0;
    }
    else if (intersection.ur.x == room_shape_.ur.x && intersection.ll.y > room_shape_.ll.y)
    {
        first_touching_side = 1;
    }
    else if (intersection.ur.y == room_shape_.ur.y)
    {
        first_touching_side = 2;
    }
    else if (intersection.ll.x == room_shape_.ll.x)
    {
        first_touching_side = 3;
    }
    else
    {
        logger::FRLogger::instance().warn("SortedOrthogonalRoomNeighbour: case not expected");
        first_touching_side = -1;
    }

    if (intersection.ll.x == room_shape_.ll.x && intersection.ll.y > room_shape_.ll.y)
    {
        last_touching_side = 3;
    }
    else if (intersection.ur.y == room_shape_.ur.y && intersection.ll.x > room_shape_.ll.x)
    {
        last_touching_side = 2;
    }
    else if (intersection.ur.x == room_shape_.ur.x)
    {
        last_touching_side = 1;
    }
    else if (intersection.ll.y == room_shape_.ll.y)
    {
        last_touching_side = 0;
    }
    else
    {
        logger::FRLogger::instance().warn("SortedOrthogonalRoomNeighbour: case not expected");
        last_touching_side = -1;
    }

    mark_touching_sides();
}

void SortedOrthogonalRoomNeighbour::mark_touching_sides()
{
    if (edge_interior_touches_obstacle_ == nullptr)
    {
        return;
    }
    if (intersection.ll.y == room_shape_.ll.y && intersection.ur.x > room_shape_.ll.x
        && intersection.ll.x < room_shape_.ur.x)
    {
        (*edge_interior_touches_obstacle_)[0] = true;
    }
    if (intersection.ur.x == room_shape_.ur.x && intersection.ur.y > room_shape_.ll.y
        && intersection.ll.y < room_shape_.ur.y)
    {
        (*edge_interior_touches_obstacle_)[1] = true;
    }
    if (intersection.ur.y == room_shape_.ur.y && intersection.ur.x > room_shape_.ll.x
        && intersection.ll.x < room_shape_.ur.x)
    {
        (*edge_interior_touches_obstacle_)[2] = true;
    }
    if (intersection.ll.x == room_shape_.ll.x && intersection.ur.y > room_shape_.ll.y
        && intersection.ll.y < room_shape_.ur.y)
    {
        (*edge_interior_touches_obstacle_)[3] = true;
    }
}

int SortedOrthogonalRoomNeighbour::compare_to(const SortedOrthogonalRoomNeighbour& p_other) const
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
    const fp::IntBox& is1 = intersection;
    const fp::IntBox& is2 = p_other.intersection;
    int64_t cmp_value = 0;

    switch (first_touching_side)
    {
    case 0:
        cmp_value = is1.ll.x - is2.ll.x;
        break;
    case 1:
        cmp_value = is1.ll.y - is2.ll.y;
        break;
    case 2:
        cmp_value = is2.ur.x - is1.ur.x;
        break;
    case 3:
        cmp_value = is2.ur.y - is1.ur.y;
        break;
    default:
        logger::FRLogger::instance().warn(
            "SortedOrthogonalRoomNeighbour::compare_to: first_touching_side out of range ");
        return 0;
    }
    if (cmp_value == 0)
    {
        // The first touching points of this neighbour and other with the room
        // shape are equal. Compare the last touching points.
        int this_touching_side_diff = (last_touching_side - first_touching_side + 4) % 4;
        int other_touching_side_diff = (p_other.last_touching_side - p_other.first_touching_side + 4) % 4;
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
            cmp_value = is1.ur.x - is2.ur.x;
            break;
        case 1:
            cmp_value = is1.ur.y - is2.ur.y;
            break;
        case 2:
            cmp_value = is2.ll.x - is1.ll.x;
            break;
        case 3:
            cmp_value = is2.ll.y - is1.ll.y;
            break;
        default:
            logger::FRLogger::instance().warn(
                "SortedOrthogonalRoomNeighbour::compare_to: first_touching_side out of range ");
            return 0;
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

// ============================== SortedOrthogonalRoomNeighbours ==============================

SortedOrthogonalRoomNeighbours::SortedOrthogonalRoomNeighbours(ExpansionRoom* p_from_room,
    CompleteExpansionRoom* p_completed_room, const fp::IntBox& p_room_shape)
    : from_room_(p_from_room)
    , completed_room_(p_completed_room)
    , room_shape_(p_room_shape)
{
    edge_interior_touches_obstacle_.fill(false);
    is_obstacle_expansion_room_ = dynamic_cast<ObstacleExpansionRoom*>(from_room_) != nullptr;
}

CompleteExpansionRoom* SortedOrthogonalRoomNeighbours::calculate(ExpansionRoom* p_room,
    AutorouteEngine* p_autoroute_engine)
{
    // Port of Java SortedOrthogonalRoomNeighbours.calculate().
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

    std::unique_ptr<SortedOrthogonalRoomNeighbours> room_neighbours =
        calculate_neighbours(p_room, net_no, autoroute_search_tree, p_autoroute_engine->generate_room_id_no());
    if (room_neighbours == nullptr)
    {
        // Unexpected expansion room type or room shape (already reported).
        return nullptr;
    }

    // Check, that each side of the room shape has at least one touching
    // neighbour. Otherwise, improve the room shape by enlarging.
    const bool edge_removed = room_neighbours->try_remove_edge(net_no, autoroute_search_tree);
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
        if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(result))
        {
            calculate_incomplete_rooms_with_empty_neighbours(obstacle_room, p_autoroute_engine);
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

std::unique_ptr<SortedOrthogonalRoomNeighbours> SortedOrthogonalRoomNeighbours::calculate_neighbours(
    ExpansionRoom* p_room, int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no)
{
    if (p_room == nullptr || p_autoroute_search_tree == nullptr)
    {
        return nullptr;
    }
    const fp::Simplex room_shape_simplex = p_room->get_shape_tile();
    if (!room_shape_simplex.is_IntBox())
    {
        logger::FRLogger::instance().warn(
            "SortedOrthogonalRoomNeighbours::calculate_neighbours: IntBox expected for roomShape");
        return nullptr;
    }
    const fp::IntBox room_box = room_shape_simplex.bounding_box();

    CompleteExpansionRoom* completed_room = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room;
    if (dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(p_room) != nullptr)
    {
        owned_completed_room =
            std::make_unique<CompleteFreeSpaceExpansionRoom>(p_room->get_layer(), p_room_id_no, room_shape_simplex);
        completed_room = owned_completed_room.get();
    }
    else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(p_room))
    {
        completed_room = obstacle_room;
    }
    else
    {
        logger::FRLogger::instance().warn("SortedOrthogonalRoomNeighbours::calculate_neighbours: "
                                          "unexpected expansion room type");
        return nullptr;
    }

    std::unique_ptr<SortedOrthogonalRoomNeighbours> result(
        new SortedOrthogonalRoomNeighbours(p_room, completed_room, room_box));
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
        if (!current_shape_ptr->is_IntBox())
        {
            logger::FRLogger::instance().warn("SortedOrthogonalRoomNeighbours::calculate_neighbours: "
                                              "IntBox expected for currentShape");
            return nullptr;
        }
        const fp::IntBox current_box = current_shape_ptr->bounding_box();
        const fp::IntBox intersection = room_box.intersection(current_box);
        int dimension = intersection.dimension();
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
            logger::FRLogger::instance().warn("AutorouteEngine.calculate_doors: dimension >= 0 expected");
            continue;
        }
        result->add_sorted_neighbour(current_object, current_box, intersection);
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
                    auto* new_door = new ExpansionDoor(completed_room, neighbour_room);
                    neighbour_room->add_door(new_door);
                    completed_room->add_door(new_door);
                }
            }
        }
    }
    return result;
}

void SortedOrthogonalRoomNeighbours::add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
    const fp::IntBox& p_neighbour_shape, const fp::IntBox& p_intersection)
{
    SortedOrthogonalRoomNeighbour new_neighbour(
        p_search_tree_object, p_neighbour_shape, p_intersection, room_shape_, &edge_interior_touches_obstacle_);
    // Java uses a TreeSet, so a neighbour comparing equal to an existing one is
    // not inserted twice. In contrast to the 45 degree implementation the Java
    // orthogonal neighbour is not dropped when the touching sides could not be
    // determined.
    auto position = std::lower_bound(sorted_neighbours_.begin(), sorted_neighbours_.end(), new_neighbour,
        [](const SortedOrthogonalRoomNeighbour& p_a, const SortedOrthogonalRoomNeighbour& p_b)
        { return p_a.compare_to(p_b) < 0; });
    if (position != sorted_neighbours_.end() && position->compare_to(new_neighbour) == 0)
    {
        return;
    }
    sorted_neighbours_.insert(position, std::move(new_neighbour));
}

CompleteFreeSpaceExpansionRoom* SortedOrthogonalRoomNeighbours::release_completed_room()
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

void SortedOrthogonalRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom* p_room,
    AutorouteEngine* p_autoroute_engine)
{
    // Port of Java
    // SortedOrthogonalRoomNeighbours.calculateIncompleteRoomsWithEmptyNeighbours().
    if (p_room == nullptr || p_autoroute_engine == nullptr)
    {
        return;
    }
    const fp::Simplex room_shape_simplex = p_room->get_shape_tile();
    if (!room_shape_simplex.is_IntBox())
    {
        logger::FRLogger::instance().warn("SortedOrthogonalRoomNeighbours"
                                           "::calculate_incomplete_rooms_with_empty_neighbours: "
                                           "IntBox expected for roomShape");
        return;
    }
    const fp::IntBox room_box = room_shape_simplex.bounding_box();
    const fp::IntBox bounding_box = p_autoroute_engine->board()->get_bounding_box();
    for (int i = 0; i < 4; i++)
    {
        fp::IntBox new_room_box;
        switch (i)
        {
        case 0:
            new_room_box = fp::IntBox(bounding_box.ll.x, bounding_box.ll.y, bounding_box.ur.x, room_box.ll.y);
            break;
        case 1:
            new_room_box = fp::IntBox(room_box.ur.x, bounding_box.ll.y, bounding_box.ur.x, bounding_box.ur.y);
            break;
        case 2:
            new_room_box = fp::IntBox(bounding_box.ll.x, room_box.ur.y, bounding_box.ur.x, bounding_box.ur.y);
            break;
        default:
            // i == 3
            new_room_box = fp::IntBox(bounding_box.ll.x, bounding_box.ll.y, room_box.ll.x, bounding_box.ur.y);
            break;
        }
        const fp::IntBox new_contained_box = room_box.intersection(new_room_box);
        IncompleteFreeSpaceExpansionRoom* new_room = p_autoroute_engine->add_incomplete_expansion_room(
            new_room_box.to_Simplex(), p_room->get_layer(), new_contained_box.to_Simplex());
        if (new_room == nullptr)
        {
            continue;
        }
        auto* new_door = new ExpansionDoor(p_room, new_room, 1);
        p_room->add_door(new_door);
        new_room->add_door(new_door);
    }
}

fp::IntBox SortedOrthogonalRoomNeighbours::remove_border_line(const fp::IntBox& p_room_box, int p_remove_edge_no)
{
    // Port of Java SortedOrthogonalRoomNeighbours.removeBorderLine(). The side
    // numbers are the same as for the edge interior obstacle flags:
    // 0 = bottom, 1 = right, 2 = top, 3 = left.
    switch (p_remove_edge_no)
    {
    case 0:
        return fp::IntBox(p_room_box.ll.x, -fp::Limits::CRIT_INT, p_room_box.ur.x, p_room_box.ur.y);
    case 1:
        return fp::IntBox(p_room_box.ll.x, p_room_box.ll.y, fp::Limits::CRIT_INT, p_room_box.ur.y);
    case 2:
        return fp::IntBox(p_room_box.ll.x, p_room_box.ll.y, p_room_box.ur.x, fp::Limits::CRIT_INT);
    case 3:
        return fp::IntBox(-fp::Limits::CRIT_INT, p_room_box.ll.y, p_room_box.ur.x, p_room_box.ur.y);
    default:
        logger::FRLogger::instance().warn(
            "SortedOrthogonalRoomNeighbours::remove_border_line: illegal removeEdgeNo");
        return fp::IntBox::EMPTY;
    }
}

void SortedOrthogonalRoomNeighbours::insert_incomplete_room(
    AutorouteEngine* p_autoroute_engine, int64_t p_ll_x, int64_t p_ll_y, int64_t p_ur_x, int64_t p_ur_y)
{
    // Port of Java SortedOrthogonalRoomNeighbours.insertIncompleteRoom().
    const fp::IntBox new_incomplete_room_shape(p_ll_x, p_ll_y, p_ur_x, p_ur_y);
    if (new_incomplete_room_shape.dimension() != 2)
    {
        return;
    }
    const fp::IntBox new_contained_shape = room_shape_.intersection(new_incomplete_room_shape);
    if (new_contained_shape.is_empty())
    {
        return;
    }
    const int door_dimension = new_incomplete_room_shape.intersection(room_shape_).dimension();
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

bool SortedOrthogonalRoomNeighbours::try_remove_edge(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree)
{
    // Port of Java SortedOrthogonalRoomNeighbours.tryRemoveEdge().
    auto* current_incomplete_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(from_room_);
    if (current_incomplete_room == nullptr)
    {
        // Only the shape of an incomplete free space room may be enlarged.
        return false;
    }
    const fp::Simplex room_shape_simplex = current_incomplete_room->get_shape_tile();
    if (!room_shape_simplex.is_IntBox())
    {
        logger::FRLogger::instance().warn(
            "SortedOrthogonalRoomNeighbours::try_remove_edge: IntBox expected for roomShape type");
        return false;
    }
    const fp::IntBox room_box = room_shape_simplex.bounding_box();
    const double room_area = room_box.area();

    int remove_edge_no = -1;
    for (int i = 0; i < 4; i++)
    {
        if (!edge_interior_touches_obstacle_[i])
        {
            remove_edge_no = i;
            break;
        }
    }
    if (remove_edge_no < 0)
    {
        return false;
    }

    // Touching neighbour missing at the edge side with index remove_edge_no.
    // Remove the edge line and restart the algorithm.
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE start, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", removeEdge=" + std::to_string(remove_edge_no)
        + ", room_bounds=" + describe_bounds(room_box));
    const fp::IntBox enlarged_box = remove_border_line(room_box, remove_edge_no);

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
        const double current_door_area = current_door_shape.area();
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
    input.shape = &enlarged_box;
    input.contained_shape = &contained_shape;
    input.layer = current_incomplete_room->get_layer();
    input.net_no = p_net_no;
    input.ignore_object = ignore_object;
    input.ignore_shape = ignore_shape;
    std::vector<board::ShapeSearchTree::CompleteShapePiece> new_rooms = p_autoroute_search_tree->complete_shape(input);
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE complete_shape, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", removeEdge=" + std::to_string(remove_edge_no)
        + ", enlarged_bounds=" + describe_bounds(enlarged_box)
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
        + ", removeEdge=" + std::to_string(remove_edge_no)
        + ", old_bounds=" + describe_bounds(room_box)
        + ", newBounds=" + describe_bounds(new_room.shape->bounding_box()));
    current_incomplete_room->set_shape_tile(new_room.shape->to_Simplex());
    current_incomplete_room->set_contained_shape_tile(new_room.contained_shape->to_Simplex());
    return true;
}

void SortedOrthogonalRoomNeighbours::calculate_new_incomplete_rooms(AutorouteEngine* p_autoroute_engine)
{
    // Port of Java SortedOrthogonalRoomNeighbours.calculateNewIncompleteRooms().
    if (p_autoroute_engine == nullptr || completed_room_ == nullptr || sorted_neighbours_.empty())
    {
        return;
    }
    const fp::IntBox board_bounds = p_autoroute_engine->board()->get_bounding_box();
    size_t prev_neighbour_index = sorted_neighbours_.size() - 1;

    for (size_t next_neighbour_index = 0; next_neighbour_index < sorted_neighbours_.size(); next_neighbour_index++)
    {
        const SortedOrthogonalRoomNeighbour& prev_neighbour = sorted_neighbours_[prev_neighbour_index];
        const SortedOrthogonalRoomNeighbour& next_neighbour = sorted_neighbours_[next_neighbour_index];
        if (!next_neighbour.intersection.intersects(prev_neighbour.intersection))
        {
            // create a door to a new incomplete expansion room between
            // the last corner of the previous neighbour and the first corner of the
            // current neighbour.
            switch (next_neighbour.first_touching_side)
            {
            case 0:
                if (prev_neighbour.last_touching_side == 0)
                {
                    if (prev_neighbour.intersection.ur.x < next_neighbour.intersection.ll.x)
                    {
                        insert_incomplete_room(p_autoroute_engine, prev_neighbour.intersection.ur.x, board_bounds.ll.y,
                            next_neighbour.intersection.ll.x, room_shape_.ll.y);
                    }
                }
                else
                {
                    if (prev_neighbour.intersection.ll.y > room_shape_.ll.y
                        || next_neighbour.intersection.ll.x > room_shape_.ll.x)
                    {
                        if (is_obstacle_expansion_room_)
                        {
                            // no 2-dim doors between obstacle_expansion_rooms and free space rooms
                            // allowed.
                            if (prev_neighbour.last_touching_side == 3)
                            {
                                insert_incomplete_room(p_autoroute_engine, board_bounds.ll.x, room_shape_.ll.y,
                                    room_shape_.ll.x, prev_neighbour.intersection.ll.y);
                            }
                            insert_incomplete_room(p_autoroute_engine, room_shape_.ll.x, board_bounds.ll.y,
                                next_neighbour.intersection.ll.x, room_shape_.ll.y);
                        }
                        else
                        {
                            insert_incomplete_room(p_autoroute_engine, board_bounds.ll.x, board_bounds.ll.y,
                                next_neighbour.intersection.ll.x, prev_neighbour.intersection.ll.y);
                        }
                    }
                }
                break;
            case 1:
                if (prev_neighbour.last_touching_side == 1)
                {
                    if (prev_neighbour.intersection.ur.y < next_neighbour.intersection.ll.y)
                    {
                        insert_incomplete_room(p_autoroute_engine, room_shape_.ur.x, prev_neighbour.intersection.ur.y,
                            board_bounds.ur.x, next_neighbour.intersection.ll.y);
                    }
                }
                else
                {
                    if (prev_neighbour.intersection.ur.x < room_shape_.ur.x
                        || next_neighbour.intersection.ll.y > room_shape_.ll.y)
                    {
                        if (is_obstacle_expansion_room_)
                        {
                            // no 2-dim doors between obstacle_expansion_rooms and free space rooms
                            // allowed.
                            if (prev_neighbour.last_touching_side == 0)
                            {
                                insert_incomplete_room(p_autoroute_engine, prev_neighbour.intersection.ur.x,
                                    board_bounds.ll.y, room_shape_.ur.x, room_shape_.ll.y);
                            }
                            insert_incomplete_room(p_autoroute_engine, room_shape_.ur.x, room_shape_.ll.y,
                                room_shape_.ur.x, next_neighbour.intersection.ll.y);
                        }
                        else
                        {
                            insert_incomplete_room(p_autoroute_engine, prev_neighbour.intersection.ur.x,
                                board_bounds.ll.y, board_bounds.ur.x, next_neighbour.intersection.ll.y);
                        }
                    }
                }
                break;
            case 2:
                if (prev_neighbour.last_touching_side == 2)
                {
                    if (prev_neighbour.intersection.ll.x > next_neighbour.intersection.ur.x)
                    {
                        insert_incomplete_room(p_autoroute_engine, next_neighbour.intersection.ur.x, room_shape_.ur.y,
                            prev_neighbour.intersection.ll.x, board_bounds.ur.y);
                    }
                }
                else
                {
                    if (prev_neighbour.intersection.ur.y < room_shape_.ur.y
                        || next_neighbour.intersection.ur.x < room_shape_.ur.x)
                    {
                        if (is_obstacle_expansion_room_)
                        {
                            // no 2-dim doors between obstacle_expansion_rooms and free space rooms
                            // allowed.
                            if (prev_neighbour.last_touching_side == 1)
                            {
                                insert_incomplete_room(p_autoroute_engine, room_shape_.ur.x,
                                    prev_neighbour.intersection.ur.y, board_bounds.ur.x, room_shape_.ur.y);
                            }
                            insert_incomplete_room(p_autoroute_engine, next_neighbour.intersection.ur.x, room_shape_.ur.y,
                                room_shape_.ur.x, board_bounds.ur.y);
                        }
                        else
                        {
                            insert_incomplete_room(p_autoroute_engine, next_neighbour.intersection.ur.x,
                                prev_neighbour.intersection.ur.y, board_bounds.ur.x, board_bounds.ur.y);
                        }
                    }
                }
                break;
            case 3:
                if (prev_neighbour.last_touching_side == 3)
                {
                    if (prev_neighbour.intersection.ll.y > next_neighbour.intersection.ur.y)
                    {
                        insert_incomplete_room(p_autoroute_engine, board_bounds.ll.x, next_neighbour.intersection.ur.y,
                            room_shape_.ll.x, prev_neighbour.intersection.ll.y);
                    }
                }
                else
                {
                    if (next_neighbour.intersection.ur.y < room_shape_.ur.y
                        || prev_neighbour.intersection.ll.x > room_shape_.ll.x)
                    {
                        if (is_obstacle_expansion_room_)
                        {
                            // no 2-dim doors between obstacle_expansion_rooms and free space rooms
                            // allowed.
                            if (prev_neighbour.last_touching_side == 2)
                            {
                                insert_incomplete_room(p_autoroute_engine, room_shape_.ll.x, room_shape_.ur.y,
                                    prev_neighbour.intersection.ll.x, board_bounds.ur.y);
                            }
                            insert_incomplete_room(p_autoroute_engine, board_bounds.ll.x, next_neighbour.intersection.ur.y,
                                room_shape_.ll.x, room_shape_.ur.y);
                        }
                        else
                        {
                            insert_incomplete_room(p_autoroute_engine, board_bounds.ll.x,
                                next_neighbour.intersection.ur.y, prev_neighbour.intersection.ll.x, board_bounds.ur.y);
                        }
                    }
                }
                break;
            default:
                logger::FRLogger::instance().warn(
                    "SortedOrthogonalRoomNeighbour::calculate_new_incomplete: illegal touching side");
                break;
            }
        }
        prev_neighbour_index = next_neighbour_index;
    }
}

} // namespace freerouting::autoroute
