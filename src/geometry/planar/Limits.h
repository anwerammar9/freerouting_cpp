#pragma once

#include <string>
#include <cmath>

namespace freerouting {
namespace geometry {
namespace planar {

/**
 * @brief Some numerical limits and values are stored here
 * 
 * This class contains important numerical constants and limits used
 * throughout the geometry system, including critical coordinate bounds
 * and precision thresholds.
 */
class Limits {
public:
    /**
     * @brief An upper bound (2^25) so that the product of two integers 
     * with absolute value at most CRIT_INT is contained in the mantissa 
     * of a double with some space left for addition
     */
    static constexpr int CRIT_INT = 33554432;

    /**
     * @brief The biggest double value (2^53), so that all integers 
     * smaller than this value are exactly represented as double value
     */
    static constexpr double CRIT_DOUBLE = 9007199254740992.0;

    /**
     * @brief Square root of 2
     */
    static const double SQRT2;

    /**
     * @brief Check if an integer value is within critical bounds
     * 
     * @param value The integer value to check
     * @return true if the value is within safe bounds for double precision
     */
    static constexpr bool is_within_crit_int(int value) {
        return std::abs(value) < CRIT_INT;
    }

    /**
     * @brief Check if a double value can exactly represent all integers 
     * up to its magnitude
     * 
     * @param value The double value to check
     * @return true if the value is within exact integer representation bounds
     */
    static constexpr bool is_within_crit_double(double value) {
        return std::abs(value) < CRIT_DOUBLE;
    }

    /**
     * @brief Get description of critical integer limit
     * 
     * @return Human-readable description of CRIT_INT purpose
     */
    static std::string get_crit_int_description();

    /**
     * @brief Get description of critical double limit
     * 
     * @return Human-readable description of CRIT_DOUBLE purpose
     */
    static std::string get_crit_double_description();

private:
    // Disallow instantiation - this is a utility class with only static members
    Limits() = delete;
    Limits(const Limits&) = delete;
    Limits& operator=(const Limits&) = delete;
    ~Limits() = delete;
};

} // namespace planar
} // namespace geometry
} // namespace freerouting
