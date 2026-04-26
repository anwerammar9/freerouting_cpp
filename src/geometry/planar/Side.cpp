#include "geometry/planar/Side.h"
#include <stdexcept>

namespace freerouting::geometry::planar {

const Side& Side::ON_THE_LEFT()
{
    static const Side instance(Type::ON_THE_LEFT);
    return instance;
}

const Side& Side::ON_THE_RIGHT()
{
    static const Side instance(Type::ON_THE_RIGHT);
    return instance;
}

const Side& Side::COLLINEAR()
{
    static const Side instance(Type::COLLINEAR);
    return instance;
}

Side Side::of(double p_value)
{
    if (p_value > 0)
    {
        return ON_THE_LEFT();
    }
    else if (p_value < 0)
    {
        return ON_THE_RIGHT();
    }
    else
    {
        return COLLINEAR();
    }
}

std::string Side::to_string() const
{
    switch (id_)
    {
        case Type::ON_THE_LEFT:
            return "on_the_left";
        case Type::ON_THE_RIGHT:
            return "on_the_right";
        case Type::COLLINEAR:
            return "collinear";
        default:
            return "unknown";
    }
}

const Side& Side::negate() const
{
    if (id_ == Type::ON_THE_LEFT)
    {
        return ON_THE_RIGHT();
    }
    else if (id_ == Type::ON_THE_RIGHT)
    {
        return ON_THE_LEFT();
    }
    else
    {
        return *this;
    }
}

} // namespace freerouting::geometry::planar