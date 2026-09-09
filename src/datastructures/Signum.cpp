#include "datastructures/Signum.h"

namespace freerouting {
namespace datastructures {

const Signum Signum::POSITIVE{"positive"};
const Signum Signum::NEGATIVE{"negative"};
const Signum Signum::ZERO{"zero"};

Signum::Signum(const std::string& p_name)
    : name(p_name)
{
}

Signum Signum::of(double p_value)
{
    if (p_value > 0)
    {
        return POSITIVE;
    }
    else if (p_value < 0)
    {
        return NEGATIVE;
    }
    else
    {
        return ZERO;
    }
}

int Signum::as_int(double p_value)
{
    if (p_value > 0)
    {
        return 1;
    }
    else if (p_value < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

Signum Signum::of(int p_value)
{
    return of(static_cast<double>(p_value));
}

int Signum::as_int(int p_value)
{
    return as_int(static_cast<double>(p_value));
}

std::string Signum::to_string() const
{
    return name;
}

Signum Signum::negate() const
{
    if (this == &POSITIVE)
    {
        return NEGATIVE;
    }
    else if (this == &NEGATIVE)
    {
        return POSITIVE;
    }
    else
    {
        return *this;
    }
}

} // namespace datastructures
} // namespace freerouting