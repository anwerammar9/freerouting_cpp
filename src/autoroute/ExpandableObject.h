#ifndef AUTOROUTE_EXPANDABLE_OBJECT_H
#define AUTOROUTE_EXPANDABLE_OBJECT_H

#include "geometry/planar/IntBox.h"

namespace freerouting::autoroute {

class ExpansionRoom;
class CompleteExpansionRoom;
class MazeSearchElement;

class ExpandableObject {
public:
    virtual ~ExpandableObject() = default;

    virtual geometry::planar::IntBox get_shape() const = 0;
    virtual int get_dimension() const = 0;
    virtual ExpansionRoom* other_room(ExpansionRoom* p_room) = 0;
    virtual CompleteExpansionRoom* other_room(CompleteExpansionRoom* p_room) = 0;
    virtual int maze_search_element_count() const = 0;
    virtual MazeSearchElement* get_maze_search_element(int p_no) = 0;
    virtual void reset() = 0;
};

} // namespace freerouting::autoroute

#endif // AUTOROUTE_EXPANDABLE_OBJECT_H
