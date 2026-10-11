#pragma once

#include "autoroute/FoundConnectionLocator.h"

#include <optional>
#include <vector>

namespace freerouting {
namespace autoroute {

/**
 * Calculates from the backtrack list the location of the traces and vias,
 * which realize a connection found by the maze search algorithm in any angle
 * mode. Port of Java
 * app.freerouting.autoroute.path.FoundConnectionLocatorAnyAngle.
 */
class FoundConnectionLocatorAnyAngle : public FoundConnectionLocator
{
public:
    ~FoundConnectionLocatorAnyAngle() override = default;

protected:
    std::vector<geometry::planar::FloatPoint> calculate_next_trace_corners() override;

private:
    /** The tolerance used for the clearance correction of the visibility range. */
    static constexpr double C_TOLERANCE = 1.0;

    /**
     * Calculates the left most corner of the shape of p_to_info.door seen from
     * the center of the common room with the previous door. Port of Java
     * FoundConnectionLocatorAnyAngle.calcDoorLeftCorner().
     */
    static std::optional<geometry::planar::FloatPoint> calc_door_left_corner(
        const BacktrackElement& p_to_info);

    /**
     * Calculates the right most corner of the shape of p_to_info.door seen from
     * the center of the common room with the previous door. Port of Java
     * FoundConnectionLocatorAnyAngle.calcDoorRightCorner().
     */
    static std::optional<geometry::planar::FloatPoint> calc_door_right_corner(
        const BacktrackElement& p_to_info);

    /**
     * Calculates as first line the left side tangent from p_from_corner to the
     * circle with center p_to_corner and radius p_dist. As second line the right
     * side tangent from p_to_corner to the circle with center p_next_corner and
     * radius 2 * p_dist is constructed. The second line is than translated by
     * the distance p_dist to the left. Returned is the intersection of the
     * first and the second line. Port of Java
     * FoundConnectionLocatorAnyAngle.rightTurnNextCorner().
     */
    std::optional<geometry::planar::FloatPoint> right_turn_next_corner(
        const geometry::planar::FloatPoint& p_from_corner,
        double p_dist,
        const std::optional<geometry::planar::FloatPoint>& p_to_corner,
        const std::optional<geometry::planar::FloatPoint>& p_next_corner);

    /**
     * Calculates as first line the right side tangent from p_from_corner to the
     * circle with center p_to_corner and radius p_dist. As second line the left
     * side tangent from p_to_corner to the circle with center p_next_corner and
     * radius 2 * p_dist is constructed. The second line is than translated by
     * the distance p_dist to the right. Returned is the intersection of the
     * first and the second line. Port of Java
     * FoundConnectionLocatorAnyAngle.leftTurnNextCorner().
     */
    std::optional<geometry::planar::FloatPoint> left_turn_next_corner(
        const geometry::planar::FloatPoint& p_from_corner,
        double p_dist,
        const std::optional<geometry::planar::FloatPoint>& p_to_corner,
        const std::optional<geometry::planar::FloatPoint>& p_next_corner);

    /**
     * Calculates the right tangential line from p_from_point and the left
     * tangential line from p_to_point to the circle with center p_center and
     * radius p_dist. Returns the intersection of the 2 lines. Port of Java
     * FoundConnectionLocatorAnyAngle.rightLeftTangentialPoint().
     */
    static std::optional<geometry::planar::FloatPoint> right_left_tangential_point(
        const geometry::planar::FloatPoint& p_from_point,
        const geometry::planar::FloatPoint& p_to_point,
        const geometry::planar::FloatPoint& p_center,
        double p_dist);

    /**
     * Calculates the left tangential line from p_from_point and the right
     * tangential line from p_to_point to the circle with center p_center and
     * radius p_dist. Returns the intersection of the 2 lines. Port of Java
     * FoundConnectionLocatorAnyAngle.leftRightTangentialPoint().
     */
    static std::optional<geometry::planar::FloatPoint> left_right_tangential_point(
        const geometry::planar::FloatPoint& p_from_point,
        const geometry::planar::FloatPoint& p_to_point,
        const geometry::planar::FloatPoint& p_center,
        double p_dist);
};

} // namespace autoroute
} // namespace freerouting