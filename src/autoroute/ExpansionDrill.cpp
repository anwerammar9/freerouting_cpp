#include "autoroute/ExpansionDrill.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionRoom.h"
#include "board/ShapeSearchTree.h"
#include "logger/FRLogger.h"

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

    for (int i = first_layer; i <= last_layer; ++i)
    {
        int idx = i - first_layer;
        room_arr[idx] = nullptr;
    }

    return true;
}

} // namespace freerouting::autoroute
