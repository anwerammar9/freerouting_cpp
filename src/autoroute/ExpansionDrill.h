#pragma once

#include "autoroute/ExpandableObject.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/IntBox.h"
#include <memory>
#include <vector>

namespace freerouting::board {
class SearchTreeObject;
}

namespace freerouting::autoroute {

class AutorouteEngine;
class CompleteExpansionRoom;

class ExpansionDrill : public ExpandableObject {
public:
    ExpansionDrill(const geometry::planar::IntBox& p_shape,
                   const geometry::planar::IntPoint& p_location,
                   int p_first_layer,
                   int p_last_layer);

    geometry::planar::IntBox get_shape() const override;
    int get_dimension() const override { return 2; }
    CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) override { (void)p_room; return nullptr; }
    int maze_search_element_count() const override;
    MazeSearchElement* get_maze_search_element(int p_no) override;
    void reset() override;

    /**
     * Returns a unique identification number for this drill: a stable hash of
     * the drill location and the drill layer range. Port of Java
     * ExpansionDrill.getId().
     */
    int64_t get_id() const override;

    bool calculate_expansion_rooms(AutorouteEngine* p_autoroute_engine);

    const geometry::planar::IntPoint location;
    const int first_layer;
    const int last_layer;
    std::vector<CompleteExpansionRoom*> room_arr;

private:
    geometry::planar::IntBox shape_;
    std::vector<MazeSearchElement> maze_search_info_arr_;
};

} // namespace freerouting::autoroute
