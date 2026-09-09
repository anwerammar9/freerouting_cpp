#include "board/Layer.h"

#include <algorithm>

namespace freerouting {
namespace board {

Layer::Layer(const std::string& p_name, bool p_is_signal)
    : name(p_name), is_signal(p_is_signal)
{
}

std::string Layer::to_string() const
{
    return name;
}

} // namespace board
} // namespace freerouting