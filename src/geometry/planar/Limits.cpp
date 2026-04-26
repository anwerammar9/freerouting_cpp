#include "Limits.h"
#include <sstream>
#include <cmath>

namespace freerouting {
namespace geometry {
namespace planar {

// Define the static const member
const double Limits::SQRT2 = std::sqrt(2.0);

std::string Limits::get_crit_int_description() {
    std::ostringstream oss;
    oss << "CRIT_INT (" << CRIT_INT << ") is an upper bound (2^25) so that ";
    oss << "the product of two integers with absolute value at most CRIT_INT ";
    oss << "is contained in the mantissa of a double with some space left for addition.";
    return oss.str();
}

std::string Limits::get_crit_double_description() {
    std::ostringstream oss;
    oss << "CRIT_DOUBLE (" << CRIT_DOUBLE << ") is the biggest double value (2^53), ";
    oss << "so that all integers smaller than this value are exactly represented as double value.";
    return oss.str();
}

} // namespace planar
} // namespace geometry
} // namespace freerouting
