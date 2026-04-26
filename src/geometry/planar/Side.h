#ifndef GEOMETRY_PLANAR_SIDE_H
#define GEOMETRY_PLANAR_SIDE_H

#include <string>

namespace freerouting::geometry::planar {

/**
 * Implementation of an enum class Side with the three values ON_THE_LEFT, ON_THE_RIGHT, COLLINEAR.
 */
class Side
{
public:
    static const Side& ON_THE_LEFT();
    static const Side& ON_THE_RIGHT();
    static const Side& COLLINEAR();

    /**
     * returns ON_THE_LEFT, if p_value < 0, ON_THE_RIGHT, if p_value > 0 and COLLINEAR, if p_value == 0
     */
    static Side of(double p_value);

    /**
     * returns the string of this instance
     */
    std::string to_string() const;

    /**
     * returns the opposite side of this side
     */
    const Side& negate() const;

    bool operator==(const Side& other) const { return id_ == other.id_; }
    bool operator!=(const Side& other) const { return id_ != other.id_; }

private:
    enum class Type { ON_THE_LEFT, ON_THE_RIGHT, COLLINEAR };
    
    constexpr Side(Type p_id) : id_(p_id) {}
    
    Type id_;
};

} // namespace freerouting::geometry::planar

#endif // GEOMETRY_PLANAR_SIDE_H