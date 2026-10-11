#pragma once

#include "autoroute/FoundConnectionLocator.h"

#include <vector>

namespace freerouting {
namespace autoroute {

/**
 * Locates and constructs 45-degree trace connection geometries from maze
 * search backtrack paths. Port of Java
 * app.freerouting.autoroute.path.FoundConnectionLocator45Degree.
 */
class FoundConnectionLocator45Degree : public FoundConnectionLocator
{
public:
    ~FoundConnectionLocator45Degree() override = default;

protected:
    std::vector<geometry::planar::FloatPoint> calculate_next_trace_corners() override;

private:
    /**
     * Calculates if the next 45-degree angle should be horizontal first when
     * coming from p_from_point on p_from_door. Port of Java
     * FoundConnectionLocator45Degree.calcHorizontalFirstFromDoor().
     */
    static bool calc_horizontal_first_from_door(const ExpandableObject* p_from_door,
                                                const geometry::planar::FloatPoint& p_from_point,
                                                const geometry::planar::FloatPoint& p_to_point);

    /**
     * Calculates, if the 45-degree angle to the next door shape should be
     * horizontal first when coming from p_from_point. Port of Java
     * FoundConnectionLocator45Degree.calcHorizontalFirstToDoor().
     */
    static bool calc_horizontal_first_to_door(const ExpandableObject* p_to_door,
                                              const geometry::planar::FloatPoint& p_from_point,
                                              const geometry::planar::FloatPoint& p_to_point);
};

} // namespace autoroute
} // namespace freerouting