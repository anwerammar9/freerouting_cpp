#pragma once

#include <array>
#include <cstdint>
#include <boost/multiprecision/cpp_int.hpp>

namespace freerouting {
namespace datastructures {

using BigInt = boost::multiprecision::cpp_int;

class BigIntAux
{
public:
    /**
     * Calculates the GCD of two values interpreted as unsigned 32-bit integers.
     */
    static int binary_gcd(int p_a, int p_b)
    {
        std::uint32_t a = static_cast<std::uint32_t>(p_a);
        std::uint32_t b = static_cast<std::uint32_t>(p_b);
        if (a == 0)
        {
            return static_cast<int>(b);
        }
        if (b == 0)
        {
            return static_cast<int>(a);
        }

        while (b != 0)
        {
            std::uint32_t remainder = a % b;
            a = b;
            b = remainder;
        }
        return static_cast<int>(a);
    }

    /**
     * Calculates the determinant of the vectors (p_x_1, p_y_1) and (p_x_2, p_y_2).
     */
    static BigInt determinant(const BigInt& p_x_1, const BigInt& p_y_1,
                              const BigInt& p_x_2, const BigInt& p_y_2)
    {
        return p_x_1 * p_y_2 - p_x_2 * p_y_1;
    }

    /**
     * Auxiliary function to implement addition and translation in the classes
     * RationalVector and RationalPoint.
     *
     * Both inputs are 3-element arrays [x, y, z] representing rational numbers
     * x/z. The result is the sum of the two rational numbers.
     */
    static std::array<BigInt, 3> add_rational_coordinates(
        const std::array<BigInt, 3>& p_first,
        const std::array<BigInt, 3>& p_second)
    {
        std::array<BigInt, 3> result;
        if (p_first[2] == p_second[2])
        {
            result[2] = p_first[2];
            result[0] = p_first[0] + p_second[0];
            result[1] = p_first[1] + p_second[1];
        }
        else
        {
            result[2] = p_first[2] * p_second[2];
            result[0] = p_first[0] * p_second[2] + p_second[0] * p_first[2];
            result[1] = p_first[1] * p_second[2] + p_second[1] * p_first[2];
        }
        return result;
    }
};

} // namespace datastructures
} // namespace freerouting
