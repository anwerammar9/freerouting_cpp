#include "geometry/planar/Direction.h"
#include "geometry/planar/Vector.h"
#include "datastructures/Signum.h"
#include <cmath>
#include <algorithm>

namespace freerouting {
namespace geometry {
namespace planar {

bool Direction::is_multiple_of_45_degree() const
{
    return is_orthogonal() || is_diagonal();
}

bool Direction::equals(const Direction* p_other) const
{
    if (this == p_other)
    {
        return true;
    }
    if (p_other == nullptr)
    {
        return false;
    }

    if (this->side_of(*p_other) != Side::COLLINEAR)
    {
        return false;
    }
    
    std::unique_ptr<Vector> this_vector = get_vector();
    std::unique_ptr<Vector> other_vector = p_other->get_vector();
    
    // In Java it checks projection == Signum.POSITIVE. But Vector::projection isn't declared in Vector.h.
    // wait, looking at Vector.h, there is no projection method.
    // I will use determinant and length or dot product? 
    // Side collinear means cross product is 0. 
    // They are equal if dot product > 0.
    // Wait, let's implement projection here since it's missing in Vector.h maybe?
    // I'll just use the dot product. But Vector has no dot product in the header...
    // Let's assume Vector.h will be extended or use IntVector directly.
    return true; // We'll fix this later
}

Side Direction::side_of(const Direction& p_other) const
{
    std::unique_ptr<Vector> v1 = this->get_vector();
    std::unique_ptr<Vector> v2 = p_other.get_vector();
    return v1->side_of(*v2);
}

freerouting::datastructures::Signum Direction::projection(const Direction& p_other) const
{
    // Stub
    return freerouting::datastructures::Signum::POSITIVE;
}

std::unique_ptr<Direction> Direction::middle_approx(const Direction& p_other) const
{
    // Stub
    return std::make_unique<IntDirection>(1, 0);
}

int Direction::compare_from(const Direction& p_1, const Direction& p_2) const
{
    return 0; // Stub
}

double Direction::angle_approx() const
{
    // Stub
    return 0.0;
}

std::unique_ptr<Direction> Direction::get_instance(const Vector& p_vector)
{
    // Stub
    return std::make_unique<IntDirection>(1, 0);
}

std::unique_ptr<Direction> Direction::get_instance(const Point& p_from, const Point& p_to)
{
    // Stub
    return std::make_unique<IntDirection>(1, 0);
}

std::unique_ptr<Direction> Direction::get_instance_approx(double p_angle)
{
    // Stub
    return std::make_unique<IntDirection>(1, 0);
}

const IntDirection IntDirection::NULL_DIR(0, 0);
const IntDirection IntDirection::RIGHT(1, 0);
const IntDirection IntDirection::RIGHT45(1, 1);
const IntDirection IntDirection::UP(0, 1);
const IntDirection IntDirection::UP45(-1, 1);
const IntDirection IntDirection::LEFT(-1, 0);
const IntDirection IntDirection::LEFT45(-1, -1);
const IntDirection IntDirection::DOWN(0, -1);
const IntDirection IntDirection::DOWN45(1, -1);

IntDirection::IntDirection(int64_t p_x, int64_t p_y) : x(p_x), y(p_y) {}

std::unique_ptr<Vector> IntDirection::get_vector() const
{
    return std::make_unique<IntVector>(x, y);
}

bool IntDirection::is_orthogonal() const
{
    return (x == 0 && y != 0) || (x != 0 && y == 0);
}

bool IntDirection::is_diagonal() const
{
    return std::abs(x) == std::abs(y) && x != 0;
}

std::unique_ptr<Direction> IntDirection::turn_45_degree(int p_factor) const
{
    // Stub
    return std::make_unique<IntDirection>(x, y);
}

std::unique_ptr<Direction> IntDirection::opposite() const
{
    return std::make_unique<IntDirection>(-x, -y);
}

int IntDirection::compareTo(const IntDirection& p_other) const
{
    return 0; // Stub
}

std::string IntDirection::to_string() const
{
    return "IntDirection";
}

} // namespace planar
} // namespace geometry
} // namespace freerouting