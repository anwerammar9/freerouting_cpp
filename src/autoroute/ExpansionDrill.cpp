#include "autoroute/ExpansionDrill.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "board/ShapeSearchTree.h"
#include "logger/FRLogger.h"

#include <set>

namespace freerouting::autoroute {

ExpansionDrill::ExpansionDrill(const geometry::planar::IntBox& p_shape,
                               const geometry::planar::IntPoint& p_location,
                               int p_first_layer,
                               int p_last_layer)
    : shape_(p_shape),
      location(p_location),
      first_layer(p_first_layer),
      last_layer(p_last_layer)
{
    int layer_count = p_last_layer - p_first_layer + 1;
    room_arr.resize(layer_count, nullptr);
    maze_search_info_arr_.resize(layer_count);
}

geometry::planar::IntBox ExpansionDrill::get_shape() const
{
    return shape_;
}

int ExpansionDrill::maze_search_element_count() const
{
    return static_cast<int>(maze_search_info_arr_.size());
}

MazeSearchElement* ExpansionDrill::get_maze_search_element(int p_no)
{
    if (p_no >= 0 && p_no < static_cast<int>(maze_search_info_arr_.size()))
    {
        return &maze_search_info_arr_[p_no];
    }
    return nullptr;
}

void ExpansionDrill::reset()
{
    for (auto& elem : maze_search_info_arr_)
    {
        elem.reset();
    }
}

int64_t ExpansionDrill::get_id() const
{
    // Port of Java ExpansionDrill.getId(): stable hash of location and layers.
    return 31 * (31 * location.get_id() + first_layer) + last_layer;
}

bool ExpansionDrill::calculate_expansion_rooms(AutorouteEngine* p_autoroute_engine)
{
    if (!p_autoroute_engine)
    {
        return false;
    }

    auto* search_tree = p_autoroute_engine->get_autoroute_search_tree();
    if (!search_tree)
    {
        return false;
    }

    // Look up all search tree objects overlapping the drill location. The
    // autoroute search tree is queried without a layer restriction so that
    // rooms of all layers are found. Mirrors Java
    // ExpansionDrill.calculateExpansionRooms().
    const geometry::planar::IntBox search_shape = geometry::planar::TileShape::get_instance(location);
    std::set<board::SearchTreeObject*, board::TreeStorableLess> overlaps =
        search_tree->overlapping_objects(search_shape, -1);

    for (int i = first_layer; i <= last_layer; ++i)
    {
        int idx = i - first_layer;
        CompleteExpansionRoom* found_room = nullptr;

        for (auto* current_object : overlaps)
        {
            auto* current_room = dynamic_cast<CompleteExpansionRoom*>(current_object);
            if (current_room == nullptr)
            {
                continue;
            }
            if (current_room->get_layer() == i)
            {
                // Java reuses the first complete room of the layer overlapping
                // the drill location; complete rooms are maximal free spaces,
                // so the location is always inside the reused room.
                found_room = current_room;
                break;
            }
        }

        if (found_room == nullptr)
        {
            // Create a new expansion room on this layer. Java
            // ExpansionDrill.calculateExpansionRooms(): the room shape is null,
            // i.e. the whole plane, and the unit search shape of the location
            // is the contained shape.
            IncompleteFreeSpaceExpansionRoom new_incomplete_room(
                nullptr, i, kWholePlaneShape, search_shape.to_Simplex());
            std::vector<CompleteFreeSpaceExpansionRoom*> new_rooms =
                p_autoroute_engine->complete_expansion_room(&new_incomplete_room);
            if (new_rooms.size() != 1)
            {
                // the size may be 0 because of an obstacle in the compensated
                // tree at this.location
                return false;
            }
            found_room = new_rooms[0];
        }
        room_arr[idx] = found_room;
    }

    return true;
}

} // namespace freerouting::autoroute
