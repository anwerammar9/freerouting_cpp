#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/TargetItemExpansionDoor.h"

namespace freerouting {
namespace autoroute {

IncompleteFreeSpaceExpansionRoom::IncompleteFreeSpaceExpansionRoom(
    std::shared_ptr<TileShape> shape, 
    int layer, 
    std::shared_ptr<TileShape> contained_shape)
    : FreeSpaceExpansionRoom(std::move(shape), layer)
    , contained_shape_(std::move(contained_shape))
{
    // Shape can be null, which means the whole plane
    // contained_shape can also be null
}

std::shared_ptr<TileShape> IncompleteFreeSpaceExpansionRoom::get_contained_shape() const {
    return contained_shape_;
}

void IncompleteFreeSpaceExpansionRoom::set_contained_shape(std::shared_ptr<TileShape> shape) {
    contained_shape_ = std::move(shape);
}

std::vector<std::shared_ptr<TargetItemExpansionDoor>> 
IncompleteFreeSpaceExpansionRoom::get_target_doors() const {
    // Incomplete rooms have no target doors
    return std::vector<std::shared_ptr<TargetItemExpansionDoor>>();
}

} // namespace autoroute
} // namespace freerouting
