#pragma once

#include <memory>
#include <vector>
#include <string>

namespace freerouting {
namespace autoroute {

class ExpansionRoom;
class TargetItemExpansionDoor;
class SearchTreeObject;

namespace graphics {
    class GraphicsContext;
}

/**
 * @brief Complete expansion room interface for autorouting
 * 
 * This interface extends the basic ExpansionRoom to provide
 * functionality for complete expansion rooms with target doors
 * and drawing capabilities.
 */
class CompleteExpansionRoom : public ExpansionRoom {
public:
    virtual ~CompleteExpansionRoom() = default;

    /**
     * @brief Returns the list of doors to target items of this room
     * 
     * @return Collection of target item expansion doors
     */
    virtual std::vector<std::shared_ptr<TargetItemExpansionDoor>> get_target_doors() const = 0;

    /**
     * @brief Returns the object of this complete expansion room
     * 
     * @return Pointer to the search tree object
     */
    virtual std::shared_ptr<SearchTreeObject> get_object() const = 0;

    /**
     * @brief Draws the shape of this room for test purposes
     * 
     * @param graphics Graphics context for drawing
     * @param graphics_context Additional graphics context information
     * @param intensity Drawing intensity (0.0 to 1.0)
     */
    virtual void draw(void* graphics, 
                     const graphics::GraphicsContext& graphics_context, 
                     double intensity) const = 0;
};

} // namespace autoroute
} // namespace freerouting
