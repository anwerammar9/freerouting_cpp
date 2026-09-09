#pragma once

#include <string>

namespace freerouting {
namespace datastructures {

/**
 * Implements the mathematical signum function.
 */
class Signum
{
public:
    static const Signum POSITIVE;
    static const Signum NEGATIVE;
    static const Signum ZERO;

    static Signum of(double p_value);
    static int as_int(double p_value);
    static Signum of(int p_value);
    static int as_int(int p_value);

    std::string to_string() const;
    Signum negate() const;

    bool operator==(const Signum& other) const { return name == other.name; }
    bool operator!=(const Signum& other) const { return !(*this == other); }

private:
    explicit Signum(const std::string& p_name);

    const std::string name;
};

} // namespace datastructures
} // namespace freerouting
