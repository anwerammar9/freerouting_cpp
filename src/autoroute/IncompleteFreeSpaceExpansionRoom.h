#pragma once

#include <memory>
#include <vector>
#include "autoroute/FreeSpaceExpansionRoom.h"
#include "geometry/planar/TileShape.h"

namespace freerouting {
namespace autoroute {

class TargetItemExpansionDoor;

/**
 * @brief An expansion room whose shape is not yet completely calculated
 * 
 * This class extends FreeSpaceExpansionRoom to provide functionality for
 * incomplete expansion rooms that have a contained shape but haven't
 * been fully calculated yet.
 */
class IncompleteFreeSpaceExpansionRoom : public FreeSpaceExpansionRoom {
public:
    /**
     * @brief Creates a new instance of IncompleteFreeSpaceExpansionRoom
     * 
     * @param shape The initial shape (null means the whole plane)
     * @param layer The layer number
     * @param contained_shape A shape which should be contained in the completed shape
     */
    IncompleteFreeSpaceExpansionRoom(std::shared_ptr<TileShape> shape, 
                                   int layer, 
                                   std::shared_ptr<TileShape> contained_shape);

    /**
     * @brief Gets the contained shape
     * 
     * @return The contained shape
     */
    std::shared_ptr<TileShape> get_contained_shape() const;

    /**
     * @brief Sets the contained shape
     * 
     * @param shape The new contained shape
     */
    void set_contained_shape(std::shared_ptr<TileShape> shape);

    /**
     * @brief Gets the target doors (returns empty collection for incomplete rooms)
     * 
     * @return Empty collection of target item expansion doors
     */
    std::vector<std::shared_ptr<TargetItemExpansionDoor>> get_target_doors() const override;

private:
    std::shared_ptr<TileShape> contained_shape_;
};

} // namespace autoroute
} // namespace freerouting
