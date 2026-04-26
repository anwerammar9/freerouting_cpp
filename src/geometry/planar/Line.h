#pragma once

#include <memory>
#include <string>
#include <optional>

#include "geometry/planar/Point.h"
#include "geometry/planar/Direction.h"

namespace freerouting {
namespace geometry {
namespace planar {

/**
 * @brief Implements functionality for lines in the plane
 * 
 * This class represents a directed line defined by two points.
 * It provides methods for line operations like intersection, side determination,
 * and geometric calculations.
 */
class Line {
public:
    /**
     * @brief First point defining the line
     */
    const IntPoint a;
    
    /**
     * @brief Second point defining the line
     */
    const IntPoint b;

    /**
     * @brief Creates a directed Line from two Points
     * 
     * @param p_a First point
     * @param p_b Second point
     */
    Line(const IntPoint& p_a, const IntPoint& p_b);

    /**
     * @brief Creates a directed Line from four integer coordinates
     * 
     * @param p_a_x X coordinate of first point
     * @param p_a_y Y coordinate of first point
     * @param p_b_x X coordinate of second point
     * @param p_b_y Y coordinate of second point
     */
    Line(int p_a_x, int p_a_y, int p_b_x, int p_b_y);

    /**
     * @brief Creates a directed Line from a Point and a Direction
     * 
     * @param p_a Starting point
     * @param p_dir Direction of the line
     */
    Line(const IntPoint& p_a, const Direction& p_dir);

    /**
     * @brief Copy constructor
     */
    Line(const Line& other) = default;

    /**
     * @brief Assignment operator
     */
    Line& operator=(const Line& other) = delete;

    /**
     * @brief Creates a directed line from a Point and a Direction
     * 
     * @param p_a Starting point
     * @param p_dir Direction
     * @return New Line instance
     */
    static Line get_instance(const IntPoint& p_a, const Direction& p_dir);

    /**
     * @brief Returns true if this and p_ob define the same line
     * 
     * @param p_other Other line to compare with
     * @return true if lines are equal
     */
    bool equals(const Line& p_other) const;

    /**
     * @brief Fast equality check for lines consisting of IntPoints
     * 
     * @param p_other Other line to compare with
     * @return true if lines are equal
     */
    bool fast_equals(const Line& p_other) const;

    /**
     * @brief Gets the direction of this directed line
     * 
     * @return Direction of the line
     */
    Direction direction() const;

    /**
     * @brief Determines which side a point is on relative to this line
     * 
     * @param p_point Point to test
     * @return Side of the line the point is on
     */
    Side side_of(const IntPoint& p_point) const;

    /**
     * @brief Determines which side a float point is on with tolerance
     * 
     * @param p_point Point to test
     * @param p_tolerance Tolerance for collinearity
     * @return Side of the line the point is on
     */
    Side side_of(const FloatPoint& p_point, double p_tolerance) const;

    /**
     * @brief Determines which side a float point is on
     * 
     * @param p_point Point to test
     * @return Side of the line the point is on
     */
    Side side_of(const FloatPoint& p_point) const;

    /**
     * @brief Determines which side the intersection of two lines is on
     * 
     * @param p_1 First line
     * @param p_2 Second line
     * @return Side of the intersection
     */
    Side side_of_intersection(const Line& p_1, const Line& p_2) const;

    /**
     * @brief Returns true if the 2 lines define the same set of points
     * 
     * @param p_other Other line to check
     * @return true if lines overlap
     */
    bool overlaps(const Line& p_other) const;

    /**
     * @brief Returns the line defining the same set of points, but with opposite direction
     * 
     * @return Line with opposite direction
     */
    Line opposite() const;

    /**
     * @brief Returns the intersection point of the 2 lines
     * 
     * @param p_other Other line to intersect with
     * @return Intersection point (infinite if parallel)
     */
    std::shared_ptr<IntPoint> intersection(const Line& p_other) const;

    /**
     * @brief Returns the approximate intersection point
     * 
     * @param p_other Other line to intersect with
     * @return Approximate intersection point
     */
    FloatPoint intersection_approx(const Line& p_other) const;

    /**
     * @brief Checks if the line is vertical
     * 
     * @return true if vertical
     */
    bool is_vertical() const;

    /**
     * @brief Checks if the line is horizontal
     * 
     * @return true if horizontal
     */
    bool is_horizontal() const;

    /**
     * @brief Checks if the line is parallel to another line
     * 
     * @param p_other Other line to check
     * @return true if parallel
     */
    bool is_parallel_to(const Line& p_other) const;

    /**
     * @brief Checks if the line is perpendicular to another line
     * 
     * @param p_other Other line to check
     * @return true if perpendicular
     */
    bool is_perpendicular_to(const Line& p_other) const;

    /**
     * @brief Returns the line shifted by a vector
     * 
     * @param p_vector Vector to shift by
     * @return Shifted line
     */
    Line shift_by(const IntVector& p_vector) const;

    /**
     * @brief Returns the line shifted by a direction
     * 
     * @param p_dir Direction to shift by
     * @return Shifted line
     */
    Line shift_by(const Direction& p_dir) const;

    /**
     * @brief Returns the line shifted by a float vector
     * 
     * @param p_vector Vector to shift by
     * @return Shifted line
     */
    Line shift_by(const FloatPoint& p_vector) const;

    /**
     * @brief Returns the line shifted by an integer amount
     * 
     * @param p_amount Amount to shift by
     * @return Shifted line
     */
    Line shift_by(int p_amount) const;

    /**
     * @brief Calculates the determinant with another line
     * 
     * @param p_other Other line
     * @return Determinant value
     */
    double determinant(const Line& p_other) const;

    /**
     * @brief Returns the line as a string representation
     * 
     * @return String representation
     */
    std::string to_string() const;

    /**
     * @brief Comparison operator for sorting
     * 
     * @param p_other Other line to compare with
     * @return true if this line comes before the other
     */
    bool operator<(const Line& p_other) const;

private:
    /**
     * @brief Cached direction for performance
     */
    mutable std::optional<Direction> cached_direction_;

    /**
     * @brief Calculates the direction if not cached
     * 
     * @return Direction of the line
     */
    Direction calculate_direction() const;

    /**
     * @brief Calculates the determinant between two vectors
     * 
     * @param v1 First vector
     * @param v2 Second vector
     * @return Determinant value
     */
    static double calculate_determinant(const IntVector& v1, const IntVector& v2);

    /**
     * @brief Handles special case intersection for orthogonal and diagonal lines
     * 
     * @param p_other Other line
     * @return Intersection point if special case, nullptr otherwise
     */
    std::shared_ptr<IntPoint> handle_special_intersection(const Line& p_other) const;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
