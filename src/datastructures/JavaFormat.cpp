#include "datastructures/JavaFormat.h"

#include <charconv>
#include <cmath>
#include <cstddef>

namespace freerouting::datastructures {

namespace {

/**
 * Shared implementation for Java Double.toString / Float.toString. The shortest
 * round-trip digits are taken from std::to_chars in scientific notation (the
 * same digit sequence Java's Ryu/Schubfach produces) and then laid out with
 * Java's fixed/scientific thresholds.
 */
template <typename T>
std::string format_java_number(T p_value)
{
    if (std::isnan(p_value))
    {
        return "NaN";
    }
    if (std::isinf(p_value))
    {
        return std::signbit(p_value) ? "-Infinity" : "Infinity";
    }
    if (p_value == static_cast<T>(0))
    {
        return std::signbit(p_value) ? "-0.0" : "0.0";
    }

    const bool negative = std::signbit(p_value);
    const T magnitude = negative ? -p_value : p_value;

    char buffer[64];
    const std::to_chars_result result =
        std::to_chars(buffer, buffer + sizeof(buffer), magnitude, std::chars_format::scientific);
    const std::string scientific(buffer, result.ptr);

    // scientific = "D[.DDD]e[+/-]XX"
    const std::size_t exponent_pos = scientific.find('e');
    std::string digits;
    digits.reserve(exponent_pos);
    for (std::size_t i = 0; i < exponent_pos; ++i)
    {
        if (scientific[i] != '.')
        {
            digits.push_back(scientific[i]);
        }
    }
    std::string exponent_text = scientific.substr(exponent_pos + 1);
    if (!exponent_text.empty() && exponent_text.front() == '+')
    {
        exponent_text.erase(exponent_text.begin());
    }
    const int exponent = std::stoi(exponent_text);
    const int digit_count = static_cast<int>(digits.size());

    std::string result_text;
    if (exponent >= -3 && exponent < 7)
    {
        // Java plain notation (10^-3 <= magnitude < 10^7).
        if (exponent >= 0)
        {
            const int integer_digits = exponent + 1;
            if (integer_digits >= digit_count)
            {
                result_text = digits + std::string(static_cast<std::size_t>(integer_digits - digit_count), '0') + ".0";
            }
            else
            {
                result_text = digits.substr(0, static_cast<std::size_t>(integer_digits)) + "."
                    + digits.substr(static_cast<std::size_t>(integer_digits));
            }
        }
        else
        {
            result_text = "0." + std::string(static_cast<std::size_t>(-exponent - 1), '0') + digits;
        }
    }
    else
    {
        // Java computerized scientific notation.
        result_text.push_back(digits.front());
        if (digit_count > 1)
        {
            result_text += "." + digits.substr(1);
        }
        else
        {
            result_text += ".0";
        }
        result_text += "E" + std::to_string(exponent);
    }

    return negative ? "-" + result_text : result_text;
}

} // namespace

std::string java_double_to_string(double p_value)
{
    return format_java_number<double>(p_value);
}

std::string java_float_to_string(float p_value)
{
    return format_java_number<float>(p_value);
}

} // namespace freerouting::datastructures
