#pragma once

#include <string>

namespace freerouting::datastructures {

/**
 * Formats a double exactly like Java Double.toString: the shortest decimal
 * representation that round-trips, using plain notation for magnitudes in
 * [1e-3, 1e7) and computerized scientific notation (e.g. "1.0E7") outside that
 * range. Integral values always carry a fraction part ("1.0").
 *
 * Java parity: the DSN/SES writers call String.valueOf(double), so the shape
 * output is only token-equal to Java when these helpers are used.
 */
std::string java_double_to_string(double p_value);

/**
 * Formats a float exactly like Java Float.toString. Same rules as
 * java_double_to_string, on the shortest float round-trip representation.
 */
std::string java_float_to_string(float p_value);

} // namespace freerouting::datastructures
