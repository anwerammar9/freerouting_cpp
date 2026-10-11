#include "autoroute/SortedRoomNeighbours.h"

#include <algorithm>
#include <cmath>

#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "autoroute/Sorted45DegreeRoomNeighbours.h"
#include "autoroute/SortedOrthogonalRoomNeighbours.h"
#include "board/ConductionArea.h"
#include "board/Item.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"
#include "datastructures/Signum.h"
#include "geometry/planar/Direction.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/ShapeBoundingDirections.h"
#include "geometry/planar/Simplex.h"
#include "geometry/planar/TileShape.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

namespace fp = geometry::planar;
using datastructures::Signum;

namespace {

/**
 * Returns the direction of p_line as a Java Direction equivalent. The C++
 * Line::direction() returns the raw IntVector, so the angle comparison is done
 * on a temporary IntDirection.
 */
std::unique_ptr<fp::Direction> line_direction_object(const fp::Line& p_line)
{
    fp::IntVector vector = p_line.direction();
    return std::make_unique<fp::IntDirection>(vector.x, vector.y);
}

/** Takes over the ownership of a cloned tile shape into a shared handle. */
std::shared_ptr<const fp::TileShape> share_shape(std::unique_ptr<fp::TileShape> p_shape)
{
    return std::shared_ptr<const fp::TileShape>(p_shape.release());
}

} // namespace

// ============================== SortedRoomNeighbour ==============================

SortedRoomNeighbour::SortedRoomNeighbour(board::SearchTreeObject* p_search_tree_object,
    std::shared_ptr<const fp::TileShape> p_neighbour_shape,
    std::shared_ptr<const fp::TileShape> p_intersection, int p_touching_side_no_of_room,
    int p_touching_side_no_of_neighbour_room, bool p_room_touch_is_corner,
    bool p_neighbour_room_touch_is_corner,
    std::shared_ptr<const fp::TileShape> p_room_shape)
    : search_tree_object(p_search_tree_object)
    , neighbour_shape(std::move(p_neighbour_shape))
    , intersection(std::move(p_intersection))
    , touching_side_no_of_room(p_touching_side_no_of_room)
    , touching_side_no_of_neighbour_room(p_touching_side_no_of_neighbour_room)
    , room_touch_is_corner(p_room_touch_is_corner)
    , neighbour_room_touch_is_corner(p_neighbour_room_touch_is_corner)
    , room_shape_(std::move(p_room_shape))
{
}

SortedRoomNeighbour::SortedRoomNeighbour(const SortedRoomNeighbour& p_other) = default;
SortedRoomNeighbour& SortedRoomNeighbour::operator=(const SortedRoomNeighbour& p_other) = default;

int SortedRoomNeighbour::compare_to(const SortedRoomNeighbour& p_other) const
{
    int compare_value = touching_side_no_of_room - p_other.touching_side_no_of_room;
    if (compare_value != 0)
    {
        return compare_value;
    }

    const fp::FloatPoint compare_corner = room_shape_->corner_approx(touching_side_no_of_room);
    double this_distance = first_corner().to_float().distance(compare_corner);
    double other_distance = p_other.first_corner().to_float().distance(compare_corner);
    double delta_distance = this_distance - other_distance;

    if (std::abs(delta_distance) <= C_DIST_TOLERANCE)
    {
        // check corners for equality
        if (first_corner() == p_other.first_corner())
        {
            // in this case compare the last corners
            double this_distance2 = last_corner().to_float().distance(compare_corner);
            double other_distance2 = p_other.last_corner().to_float().distance(compare_corner);
            delta_distance = this_distance2 - other_distance2;
            if (std::abs(delta_distance) <= C_DIST_TOLERANCE)
            {
                if (neighbour_room_touch_is_corner && p_other.neighbour_room_touch_is_corner)
                {
                    // Otherwise there may be a short 1 dim. touch at a link
                    // between 2 trace lines. In this case equality is ok,
                    // because the 2 intersection pieces with the expansion room
                    // are identical, so that only 1 obstacle is needed.
                    int compare_line_no = touching_side_no_of_room;
                    if (room_touch_is_corner)
                    {
                        compare_line_no = room_shape_->prev_no(compare_line_no);
                    }
                    std::unique_ptr<fp::Direction> compare_dir =
                        line_direction_object(room_shape_->border_line(compare_line_no))->opposite();
                    std::unique_ptr<fp::Direction> this_compare_direction =
                        line_direction_object(neighbour_shape->border_line(touching_side_no_of_neighbour_room));
                    std::unique_ptr<fp::Direction> other_compare_direction =
                        line_direction_object(
                            p_other.neighbour_shape->border_line(p_other.touching_side_no_of_neighbour_room));
                    delta_distance = compare_dir->compare_from(*this_compare_direction, *other_compare_direction);
                }
            }
        }
    }
    int res = Signum::as_int(delta_distance);
    if (res == 0)
    {
        // Deterministic tie-breaker for identical geometry
        int this_id = search_tree_object != nullptr ? search_tree_object->get_id() : 0;
        int other_id = p_other.search_tree_object != nullptr ? p_other.search_tree_object->get_id() : 0;
        res = this_id - other_id;
    }
    return res;
}

const fp::IntPoint& SortedRoomNeighbour::first_corner() const
{
    if (!precalculated_first_corner_)
    {
        if (room_touch_is_corner)
        {
            precalculated_first_corner_ = room_shape_->corner(touching_side_no_of_room);
        }
        else if (neighbour_room_touch_is_corner)
        {
            precalculated_first_corner_ = neighbour_shape->corner(touching_side_no_of_neighbour_room);
        }
        else
        {
            fp::IntPoint current_first_corner =
                neighbour_shape->corner(neighbour_shape->next_no(touching_side_no_of_neighbour_room));
            fp::Line prev_line = room_shape_->border_line(room_shape_->prev_no(touching_side_no_of_room));
            if (prev_line.side_of(current_first_corner) == fp::Side::ON_THE_RIGHT)
            {
                precalculated_first_corner_ = current_first_corner;
            }
            else
            {
                // current_first_corner is outside the door shape
                precalculated_first_corner_ = room_shape_->corner(touching_side_no_of_room);
            }
        }
    }
    return *precalculated_first_corner_;
}

const fp::IntPoint& SortedRoomNeighbour::last_corner() const
{
    if (!precalculated_last_corner_)
    {
        if (room_touch_is_corner)
        {
            precalculated_last_corner_ = room_shape_->corner(touching_side_no_of_room);
        }
        else if (neighbour_room_touch_is_corner)
        {
            precalculated_last_corner_ = neighbour_shape->corner(touching_side_no_of_neighbour_room);
        }
        else
        {
            fp::IntPoint current_last_corner = neighbour_shape->corner(touching_side_no_of_neighbour_room);
            fp::Line next_line = room_shape_->border_line(room_shape_->next_no(touching_side_no_of_room));
            if (next_line.side_of(current_last_corner) == fp::Side::ON_THE_RIGHT)
            {
                precalculated_last_corner_ = current_last_corner;
            }
            else
            {
                // current_last_corner is outside the door shape
                precalculated_last_corner_ = room_shape_->corner(room_shape_->next_no(touching_side_no_of_room));
            }
        }
    }
    return *precalculated_last_corner_;
}

// ============================== SortedRoomNeighbours ==============================

SortedRoomNeighbours::SortedRoomNeighbours(ExpansionRoom* p_from_room,
    CompleteExpansionRoom* p_completed_room)
    : from_room_(p_from_room)
    , completed_room_(p_completed_room)
    , room_shape_(std::make_shared<fp::Simplex>(p_completed_room->get_shape_tile()))
{
}

void SortedRoomNeighbours::add_sorted_neighbour(board::SearchTreeObject* p_search_tree_object,
    const fp::TileShape& p_neighbour_shape, const fp::TileShape& p_intersection,
    int p_touching_side_no_of_room, int p_touching_side_no_of_neighbour_room, bool p_room_touch_is_corner,
    bool p_neighbour_room_touch_is_corner)
{
    SortedRoomNeighbour new_neighbour(p_search_tree_object,
        share_shape(p_neighbour_shape.clone()), share_shape(p_intersection.clone()),
        p_touching_side_no_of_room, p_touching_side_no_of_neighbour_room, p_room_touch_is_corner,
        p_neighbour_room_touch_is_corner, room_shape_);

    // Java uses a TreeSet, so a neighbour comparing equal to an existing one is
    // not inserted twice.
    auto position = std::lower_bound(sorted_neighbours_.begin(), sorted_neighbours_.end(), new_neighbour,
        [](const SortedRoomNeighbour& p_a, const SortedRoomNeighbour& p_b)
        { return p_a.compare_to(p_b) < 0; });
    if (position != sorted_neighbours_.end() && position->compare_to(new_neighbour) == 0)
    {
        return;
    }
    sorted_neighbours_.insert(position, std::move(new_neighbour));
}

bool SortedRoomNeighbours::insert_door_ok_impl(ObstacleExpansionRoom* p_room,
    const std::optional<fp::Line>& p_door_line)
{
    if (!p_door_line.has_value())
    {
        logger::FRLogger::instance().warn("SortedRoomNeighbours::insert_door_ok: doorLine is null");
        return false;
    }
    board::Item* current_item = p_room->get_item();
    if (auto* current_trace = dynamic_cast<board::PolylineTrace*>(current_item))
    {
        int room_index = p_room->get_index_in_item();
        int line_count = current_trace->tile_shape_count();
        if (room_index == 0 || room_index == line_count - 1)
        {
            // Polyline::arr holds the closing start line at index 0, the middle
            // line i at index i + 1 and the closing end line at the last index,
            // so arr[room_index + 1] is the trace line following the room.
            const std::vector<fp::Line>& lines = current_trace->polyline().arr;
            int following_line_no = room_index + 1;
            if (following_line_no >= static_cast<int>(lines.size()))
            {
                return true;
            }
            return lines[following_line_no].is_parallel(*p_door_line);
        }
    }
    return true;
}

bool SortedRoomNeighbours::insert_door_ok(ObstacleExpansionRoom* p_room, const fp::Line& p_door_line)
{
    return insert_door_ok_impl(p_room, std::optional<fp::Line>(p_door_line));
}

CompleteFreeSpaceExpansionRoom* SortedRoomNeighbours::release_completed_room()
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

bool SortedRoomNeighbours::insert_door_ok(ExpansionRoom* p_room1, ExpansionRoom* p_room2,
    const fp::TileShape& p_door_shape)
{
    if (p_room1->door_exists(p_room2))
    {
        return false;
    }
    auto* obs_room1 = dynamic_cast<ObstacleExpansionRoom*>(p_room1);
    auto* obs_room2 = dynamic_cast<ObstacleExpansionRoom*>(p_room2);
    if (obs_room1 != nullptr && obs_room2 != nullptr)
    {
        // insert only overlap_doors between items of the same net for
        // performance reasons.
        return obs_room1->get_item()->shares_net(obs_room2->get_item());
    }
    if (obs_room1 == nullptr && obs_room2 == nullptr)
    {
        return true;
    }

    // Insert 1 dimensional doors of trace rooms only, if they are parallel to
    // the trace line. Otherwise, there may be check ripup problems with
    // entering at the wrong side at a fork.
    std::optional<fp::Line> door_line;
    fp::IntPoint prev_corner = p_door_shape.corner(0);
    int corner_count = p_door_shape.border_line_count();
    for (int i = 1; i < corner_count; i++)
    {
        fp::IntPoint current_corner = p_door_shape.corner(i);
        if (!(current_corner == prev_corner))
        {
            door_line = p_door_shape.border_line(i - 1);
            break;
        }
        prev_corner = current_corner;
    }

    if (obs_room1 != nullptr && !insert_door_ok_impl(obs_room1, door_line))
    {
        return false;
    }
    if (obs_room2 != nullptr)
    {
        return insert_door_ok_impl(obs_room2, door_line);
    }
    return true;
}

std::unique_ptr<SortedRoomNeighbours> SortedRoomNeighbours::calculate_neighbours(ExpansionRoom* p_room,
    int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree, int p_room_id_no)
{
    const fp::Simplex room_simplex = p_room->get_shape_tile();
    CompleteExpansionRoom* completed_room = nullptr;
    std::unique_ptr<CompleteFreeSpaceExpansionRoom> owned_completed_room;
    if (dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(p_room) != nullptr)
    {
        owned_completed_room = std::make_unique<CompleteFreeSpaceExpansionRoom>(
            p_room->get_layer(), p_room_id_no, room_simplex);
        completed_room = owned_completed_room.get();
    }
    else if (auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(p_room))
    {
        completed_room = obstacle_room;
    }
    else
    {
        logger::FRLogger::instance().warn("SortedRoomNeighbours::calculate_neighbours: "
                                          "unexpected expansion room type");
        return nullptr;
    }

    std::unique_ptr<SortedRoomNeighbours> result(new SortedRoomNeighbours(p_room, completed_room));
    result->owned_completed_room_ = std::move(owned_completed_room);
    std::vector<datastructures::ShapeTree::TreeEntry> overlapping_objects;
    p_autoroute_search_tree->overlapping_tree_entries(
        *result->room_shape_, p_room->get_layer(), overlapping_objects);

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
    const bool room_is_incomplete_free_space =
        dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(p_room) != nullptr;
    for (const datastructures::ShapeTree::TreeEntry& current_entry : overlapping_objects)
    {
        auto* current_object = dynamic_cast<board::SearchTreeObject*>(current_entry.object);
        if (current_object == nullptr
            || static_cast<const void*>(current_object) == static_cast<const void*>(p_room))
        {
            continue;
        }
        if (room_is_incomplete_free_space && !current_object->is_trace_obstacle(p_net_no))
        {
            // delay processing the target doors until the room shape will not
            // change anymore
            result->own_net_objects_.push_back(current_entry);
            continue;
        }
        const fp::TileShape* current_shape_ptr =
            current_object->get_tree_shape(*p_autoroute_search_tree, current_entry.shape_index_in_object);
        if (current_shape_ptr == nullptr)
        {
            continue;
        }
        std::unique_ptr<fp::TileShape> current_shape_owned = current_shape_ptr->clone();
        const fp::TileShape& current_shape = *current_shape_owned;
        std::unique_ptr<fp::TileShape> intersection =
            fp::TileShape::intersection_of(*result->room_shape_, current_shape);
        int dimension = intersection->dimension();

        if (dimension > 1)
        {
            auto* obstacle_room = dynamic_cast<ObstacleExpansionRoom*>(completed_room);
            auto* current_item = dynamic_cast<board::Item*>(current_object);
            if (obstacle_room != nullptr && current_item != nullptr)
            {
                // only Obstacle expansion room may have a 2-dim overlap
                if (current_item->is_routable())
                {
                    auto item_info = current_item->get_autoroute_info();
                    ObstacleExpansionRoom* current_overlap_room =
                        item_info->get_expansion_room(current_entry.shape_index_in_object,
                            p_autoroute_search_tree);
                    obstacle_room->create_overlap_door(current_overlap_room);
                }
            }
            else
            {
                logger::FRLogger::instance().trace("SortedRoomNeighbours::calculate_neighbours: "
                                                   "unexpected area overlap of free space expansion room");
            }
            continue;
        }
        if (dimension < 0)
        {
            logger::FRLogger::instance().debug("SortedRoomNeighbours::calculate_neighbours: "
                                               "dimension >= 0 expected");
            continue;
        }
        if (dimension == 1)
        {
            std::vector<int> touching_sides = result->room_shape_->touching_sides(current_shape);
            if (touching_sides.size() != 2)
            {
                logger::FRLogger::instance().debug("SortedRoomNeighbours::calculate_neighbours: "
                                                   "touchingSides length 2 expected");
                continue;
            }
            result->add_sorted_neighbour(current_object, current_shape, *intersection, touching_sides[0],
                touching_sides[1], false, false);
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
                    neighbour_room = item_info->get_expansion_room(current_entry.shape_index_in_object,
                        p_autoroute_search_tree);
                }
            }
            if (neighbour_room != nullptr)
            {
                if (insert_door_ok(completed_room, neighbour_room, *intersection))
                {
                    auto* new_door = new ExpansionDoor(completed_room, neighbour_room, 1);
                    neighbour_room->add_door(new_door);
                    completed_room->add_door(new_door);
                }
            }
        }
        else
        {
            // dimension = 0
            fp::IntPoint touching_point = intersection->corner(0);
            int room_corner_no = result->room_shape_->equals_corner(touching_point);
            bool room_touch_is_corner;
            int touching_side_no_of_room;
            if (room_corner_no >= 0)
            {
                room_touch_is_corner = true;
                touching_side_no_of_room = room_corner_no;
            }
            else
            {
                room_touch_is_corner = false;
                touching_side_no_of_room = result->room_shape_->contains_on_border_line_no(touching_point);
                if (touching_side_no_of_room < 0)
                {
                    logger::FRLogger::instance().debug("SortedRoomNeighbours::calculate_neighbours: "
                                                       "touchingSideNoOfRoom >= 0 expected");
                }
            }
            int neighbour_room_corner_no = current_shape.equals_corner(touching_point);
            bool neighbour_room_touch_is_corner;
            int touching_side_no_of_neighbour_room;
            if (neighbour_room_corner_no >= 0)
            {
                neighbour_room_touch_is_corner = true;
                // The previous border line is preferred to make the shape of the
                // incomplete room as big as possible
                touching_side_no_of_neighbour_room = current_shape.prev_no(neighbour_room_corner_no);
            }
            else
            {
                neighbour_room_touch_is_corner = false;
                touching_side_no_of_neighbour_room = current_shape.contains_on_border_line_no(touching_point);
                if (touching_side_no_of_neighbour_room < 0)
                {
                    logger::FRLogger::instance().debug("SortedRoomNeighbours::calculate_neighbours: "
                                                       "touchingSideNoOfNeighbourRoom >= 0 expected");
                }
            }
            result->add_sorted_neighbour(current_object, current_shape, *intersection,
                touching_side_no_of_room, touching_side_no_of_neighbour_room, room_touch_is_corner,
                neighbour_room_touch_is_corner);
        }
    }
    return result;
}

void SortedRoomNeighbours::calculate_incomplete_rooms_with_empty_neighbours(ObstacleExpansionRoom* p_room,
    AutorouteEngine* p_autoroute_engine)
{
    if (p_room == nullptr || p_autoroute_engine == nullptr)
    {
        return;
    }
    const fp::Simplex room_simplex = p_room->get_shape_tile();
    const int line_count = room_simplex.border_line_count();
    for (int i = 0; i < line_count; i++)
    {
        fp::Line current_line = room_simplex.border_line(i);
        if (!insert_door_ok(p_room, current_line))
        {
            continue;
        }
        std::vector<fp::Line> shape_line;
        shape_line.push_back(current_line.opposite());
        std::unique_ptr<fp::TileShape> new_room_shape = fp::TileShape::get_instance(shape_line);
        if (new_room_shape == nullptr || new_room_shape->is_empty())
        {
            continue;
        }
        std::unique_ptr<fp::TileShape> new_contained_shape =
            fp::TileShape::intersection_of(room_simplex, *new_room_shape);
        if (new_contained_shape == nullptr || new_contained_shape->is_empty())
        {
            continue;
        }
        IncompleteFreeSpaceExpansionRoom* new_room =
            p_autoroute_engine->add_incomplete_expansion_room(new_room_shape->to_Simplex(), p_room->get_layer(),
                new_contained_shape->to_Simplex());
        if (new_room == nullptr)
        {
            continue;
        }
        auto* new_door = new ExpansionDoor(p_room, new_room, 1);
        p_room->add_door(new_door);
        new_room->add_door(new_door);
    }
}

CalculationMode select_calculation_mode(const board::ShapeSearchTree* p_search_tree)
{
    // Port of Java SortedRoomNeighbours.selectCalculationMode(): Java
    // instanceof checks ShapeSearchTree45Degree / ShapeSearchTree90Degree. The
    // C++ ShapeSearchTree is configured with the bounding directions instead of
    // a subclass, so the number of bounding directions selects the mode.
    if (p_search_tree == nullptr)
    {
        return CalculationMode::ANY_ANGLE;
    }
    switch (p_search_tree->get_bounding_directions().count())
    {
    case 4:
        return CalculationMode::ORTHOGONAL;
    case 8:
        return CalculationMode::DEGREE_45;
    default:
        return CalculationMode::ANY_ANGLE;
    }
}

CompleteExpansionRoom* SortedRoomNeighbours::calculate(ExpansionRoom* p_room, AutorouteEngine* p_autoroute_engine)
{
    // Port of Java SortedRoomNeighbours.complete() together with the dispatch
    // of Java selectCalculationMode(): the search tree specific implementation
    // is used, if one exists for the bounding directions of the tree.
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

    // Port of the Java switch on selectCalculationMode(autorouteEngine.autorouteSearchTree).
    switch (select_calculation_mode(autoroute_search_tree))
    {
    case CalculationMode::ORTHOGONAL:
        return SortedOrthogonalRoomNeighbours::calculate(p_room, p_autoroute_engine);
    case CalculationMode::DEGREE_45:
        return Sorted45DegreeRoomNeighbours::calculate(p_room, p_autoroute_engine);
    case CalculationMode::ANY_ANGLE:
        break;
    }

    std::unique_ptr<SortedRoomNeighbours> room_neighbours = calculate_neighbours(
        p_room, net_no, autoroute_search_tree, p_autoroute_engine->generate_room_id_no());
    if (room_neighbours == nullptr)
    {
        // Unexpected expansion room type (already reported).
        return nullptr;
    }

    // Check, that each side of the room shape has at least one touching
    // neighbour. Otherwise, improve the room shape by enlarging.
    const bool edge_removed = room_neighbours->try_remove_edge(net_no, autoroute_search_tree);
    if (edge_removed)
    {
        // The room shape changed, so all doors calculated for the previous
        // shape are invalid and the calculation starts over with the enlarged
        // room. The previous complete room is not used anymore.
        p_autoroute_engine->remove_all_doors(room_neighbours->get_completed_room());
        return calculate(p_room, p_autoroute_engine);
    }

    // Now calculate the new incomplete rooms together with the doors between
    // this room and the sorted neighbours.
    CompleteExpansionRoom* result = room_neighbours->get_completed_room();
    if (room_neighbours->get_sorted_neighbours().empty())
    {
        if (dynamic_cast<ObstacleExpansionRoom*>(result) != nullptr)
        {
            calculate_incomplete_rooms_with_empty_neighbours(dynamic_cast<ObstacleExpansionRoom*>(result),
                p_autoroute_engine);
        }
    }
    else
    {
        room_neighbours->calculate_new_incomplete_rooms(p_autoroute_engine);
        if (result != nullptr && result->get_shape().dimension() < 2)
        {
            logger::FRLogger::instance().trace("AutorouteEngine.calculate_new_incomplete_rooms_with_more"
                                               "_than_1_neighbour: unexpected dimension for smoothened_shape");
        }
    }

    if (auto* free_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(result))
    {
        // Port of the Java calculateTargetDoors(): the room becomes net
        // dependent if it overlaps with net dependent objects, and a target
        // door is created for every such object.
        for (const datastructures::ShapeTree::TreeEntry& current_entry : room_neighbours->get_own_net_objects())
        {
            free_room->calculate_target_doors(current_entry, net_no, autoroute_search_tree);
        }
    }

    // A newly created complete free space room is owned by this instance, so
    // its ownership is transferred to the caller. If the from room was an
    // obstacle room, the completed room is the from room itself.
    CompleteFreeSpaceExpansionRoom* created_room = room_neighbours->release_completed_room();
    return created_room != nullptr ? static_cast<CompleteExpansionRoom*>(created_room) : result;
}

bool SortedRoomNeighbours::try_remove_edge(int p_net_no, board::ShapeSearchTree* p_autoroute_search_tree)
{
    // Port of Java SortedRoomNeighbours.tryRemoveEdge().
    auto* current_incomplete_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(from_room_);
    if (current_incomplete_room == nullptr)
    {
        // Only the shape of an incomplete free space room may be enlarged.
        return false;
    }
    const fp::Simplex room_simplex = current_incomplete_room->get_shape_tile();
    const double room_shape_area = room_simplex.area();

    int remove_edge_no = -1;
    int prev_edge_no = -1;
    int current_edge_no = 0;
    for (const SortedRoomNeighbour& next_neighbour : sorted_neighbours_)
    {
        if (next_neighbour.touching_side_no_of_room == prev_edge_no)
        {
            continue;
        }
        if (next_neighbour.touching_side_no_of_room == current_edge_no)
        {
            prev_edge_no = current_edge_no;
            ++current_edge_no;
        }
        else
        {
            // On the edge side with index current_edge_no is no touching
            // neighbour.
            remove_edge_no = current_edge_no;
            break;
        }
    }

    if (remove_edge_no < 0 && current_edge_no < room_simplex.border_line_count())
    {
        // missing touching neighbour at the last edge side.
        remove_edge_no = current_edge_no;
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
        + ", room_bounds=" + room_simplex.bounding_box().to_string());
    const fp::Simplex enlarged_shape = room_simplex.remove_border_line(remove_edge_no);

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
    input.shape = &enlarged_shape;
    input.contained_shape = &contained_shape;
    input.layer = current_incomplete_room->get_layer();
    input.net_no = p_net_no;
    input.ignore_object = nullptr;
    input.ignore_shape = nullptr;
    std::vector<board::ShapeSearchTree::CompleteShapePiece> new_rooms =
        p_autoroute_search_tree->complete_shape(input);
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE complete_shape, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", removeEdge=" + std::to_string(remove_edge_no)
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
    if (new_room.shape->area() <= room_shape_area)
    {
        return false;
    }
    logger::FRLogger::instance().trace("ROOM_EDGE_REMOVE applied, net=" + std::to_string(p_net_no)
        + ", layer=" + std::to_string(current_incomplete_room->get_layer())
        + ", removeEdge=" + std::to_string(remove_edge_no)
        + ", old_bounds=" + room_simplex.bounding_box().to_string()
        + ", newBounds=" + new_room.shape->bounding_box().to_string());
    current_incomplete_room->set_shape_tile(new_room.shape->to_Simplex());
    current_incomplete_room->set_contained_shape_tile(new_room.contained_shape->to_Simplex());
    return true;
}

void SortedRoomNeighbours::calculate_new_incomplete_rooms(AutorouteEngine* p_autoroute_engine)
{
    // Port of Java SortedRoomNeighbours.calculateNewIncompleteRooms().
    if (sorted_neighbours_.empty() || completed_room_ == nullptr || p_autoroute_engine == nullptr)
    {
        return;
    }
    const size_t last_neighbour_index = sorted_neighbours_.size() - 1;
    size_t prev_neighbour_index = last_neighbour_index;
    const fp::Simplex room_simplex = from_room_->get_shape_tile();

    for (size_t next_neighbour_index = 0; next_neighbour_index <= last_neighbour_index; next_neighbour_index++)
    {
        const SortedRoomNeighbour& prev_neighbour = sorted_neighbours_[prev_neighbour_index];
        const SortedRoomNeighbour& next_neighbour = sorted_neighbours_[next_neighbour_index];
        // Java compares the neighbour references to identify the last neighbour
        // of the sorted set.
        const bool prev_neighbour_is_last = prev_neighbour_index == last_neighbour_index;

        int first_touching_side_no = prev_neighbour.touching_side_no_of_room;
        int last_touching_side_no = next_neighbour.touching_side_no_of_room;

        const int current_next_no = room_simplex.next_no(first_touching_side_no);
        const bool intersection_with_prev_neighbour_ends_at_corner =
            (first_touching_side_no != last_touching_side_no || prev_neighbour_is_last)
            && prev_neighbour.last_corner() == room_simplex.corner(current_next_no);
        const bool intersection_with_next_neighbour_starts_at_corner =
            (first_touching_side_no != last_touching_side_no || prev_neighbour_is_last)
            && next_neighbour.first_corner() == room_simplex.corner(last_touching_side_no);

        if (intersection_with_prev_neighbour_ends_at_corner)
        {
            first_touching_side_no = current_next_no;
        }
        if (intersection_with_next_neighbour_starts_at_corner)
        {
            last_touching_side_no = room_simplex.prev_no(last_touching_side_no);
        }

        bool neighbours_touch = false;
        if (sorted_neighbours_.size() > 1)
        {
            neighbours_touch = prev_neighbour.last_corner() == next_neighbour.first_corner();
        }

        if (!neighbours_touch)
        {
            // Create a door to a new incomplete expansion room between the last
            // corner of the previous neighbour and the first corner of the
            // current neighbour.
            int last_bounding_line_no = prev_neighbour.touching_side_no_of_neighbour_room;
            if (!(intersection_with_prev_neighbour_ends_at_corner || prev_neighbour.room_touch_is_corner))
            {
                last_bounding_line_no = prev_neighbour.neighbour_shape->prev_no(last_bounding_line_no);
            }

            int first_bounding_line_no = next_neighbour.touching_side_no_of_neighbour_room;
            if (!(intersection_with_next_neighbour_starts_at_corner || next_neighbour.neighbour_room_touch_is_corner))
            {
                first_bounding_line_no = next_neighbour.neighbour_shape->next_no(first_bounding_line_no);
            }
            std::optional<fp::Line> start_edge_line =
                next_neighbour.neighbour_shape->border_line(first_bounding_line_no).opposite();
            // start_edge_line is only used for the first new incomplete room.
            std::optional<fp::Line> middle_edge_line;
            int current_touching_side_no = last_touching_side_no;
            bool first_time = true;
            // The loop goes backwards from the edge line of next_neighbour to
            // the edge line of prev_neighbour.
            for (;;)
            {
                bool corner_cut_off = false;
                auto* incomplete_room = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(from_room_);
                auto* free_space_room = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(completed_room_);
                if (incomplete_room != nullptr && free_space_room != nullptr)
                {
                    if (current_touching_side_no == last_touching_side_no
                        && first_touching_side_no != last_touching_side_no)
                    {
                        // Create a new line approximately from the last corner of
                        // the previous neighbour to the first corner of the next
                        // neighbour to cut off the outstanding corners of the room
                        // shape in the empty space. That is only tried in the first
                        // pass of the loop.
                        fp::IntPoint cut_line_start = prev_neighbour.last_corner().to_float().round();
                        fp::IntPoint cut_line_end = next_neighbour.first_corner().to_float().round();
                        fp::Line cut_line(cut_line_start, cut_line_end);
                        std::unique_ptr<fp::TileShape> cut_half_plane = fp::TileShape::get_instance(cut_line);
                        if (cut_half_plane != nullptr && !cut_half_plane->is_empty())
                        {
                            free_space_room->set_shape_tile(free_space_room->get_shape_tile().intersection(
                                cut_half_plane->to_Simplex()));
                        }
                        // Otherwise room.contained_shape would no longer be
                        // contained in the shape after cutting off the corner.
                        corner_cut_off = incomplete_room->get_contained_shape_tile().side_of(cut_line)
                            == fp::Side::ON_THE_LEFT;
                        if (corner_cut_off)
                        {
                            middle_edge_line = cut_line.opposite();
                        }
                    }
                }
                const int next_touching_side_no = room_simplex.prev_no(current_touching_side_no);

                if (!corner_cut_off)
                {
                    middle_edge_line = room_simplex.border_line(current_touching_side_no).opposite();
                }
                if (!middle_edge_line.has_value())
                {
                    // Unreachable: middle_edge_line is assigned in the
                    // corner cut off branch or by the line above.
                    break;
                }

                std::unique_ptr<fp::Direction> middle_line_direction =
                    line_direction_object(*middle_edge_line);

                // The expression on the first line handles the case, when all
                // neighbours are on 1 edge line.
                const bool last_time = (current_touching_side_no == first_touching_side_no
                                          && !(prev_neighbour_is_last && first_time))
                    || corner_cut_off;

                // end_edge_line is only used for the last new incomplete room.
                std::optional<fp::Line> end_edge_line;
                if (last_time)
                {
                    end_edge_line = prev_neighbour.neighbour_shape->border_line(last_bounding_line_no).opposite();
                    if (line_direction_object(*end_edge_line)->side_of(*middle_line_direction) != fp::Side::ON_THE_LEFT)
                    {
                        // Concave corner between the middle and the last line.
                        // Maybe there is a 1 point touch.
                        end_edge_line.reset();
                    }
                }

                if (start_edge_line.has_value()
                    && middle_line_direction->side_of(*line_direction_object(*start_edge_line))
                        != fp::Side::ON_THE_LEFT)
                {
                    // concave corner between the first and the middle line.
                    // May be there is a 1 point touch.
                    start_edge_line.reset();
                }
                std::vector<fp::Line> new_edge_lines;
                new_edge_lines.reserve(3);
                if (start_edge_line.has_value())
                {
                    new_edge_lines.push_back(*start_edge_line);
                }
                new_edge_lines.push_back(*middle_edge_line);
                if (end_edge_line.has_value())
                {
                    new_edge_lines.push_back(*end_edge_line);
                }
                const fp::Simplex new_room_shape = fp::Simplex::get_instance(new_edge_lines);
                if (!new_room_shape.is_empty())
                {
                    std::unique_ptr<fp::TileShape> new_contained_shape = fp::TileShape::intersection_of(
                        completed_room_->get_shape_tile(), new_room_shape);
                    if (new_contained_shape != nullptr && !new_contained_shape->is_empty())
                    {
                        IncompleteFreeSpaceExpansionRoom* new_room = p_autoroute_engine->add_incomplete_expansion_room(
                            new_room_shape, from_room_->get_layer(), new_contained_shape->to_Simplex());
                        if (new_room != nullptr)
                        {
                            auto* new_door = new ExpansionDoor(completed_room_, new_room, 1);
                            completed_room_->add_door(new_door);
                            new_room->add_door(new_door);
                        }
                    }
                }
                if (last_time)
                {
                    break;
                }
                current_touching_side_no = next_touching_side_no;
                start_edge_line.reset();
                first_time = false;
            }
        }
        prev_neighbour_index = next_neighbour_index;
    }
}

} // namespace freerouting::autoroute
