#include "board/Padstacks.h"

#include <algorithm>
#include <cctype>

#include "logger/FRLogger.h"

namespace freerouting::board {

Padstacks::Padstacks(int p_board_layer_count)
    : board_layer_count_(p_board_layer_count)
{
}

Padstack* Padstacks::get(const std::string& p_name) const
{
    for (const auto& current_padstack : padstacks_)
    {
        const std::string& name = current_padstack->get_name();
        if (name.size() == p_name.size()
            && std::equal(name.begin(), name.end(), p_name.begin(), [](unsigned char a, unsigned char b) {
                   return std::tolower(a) == std::tolower(b);
               }))
        {
            return current_padstack.get();
        }
    }
    return nullptr;
}

Padstack* Padstacks::get(int p_padstack_id) const
{
    if (p_padstack_id <= 0 || p_padstack_id > count())
    {
        logger::FRLogger::instance().warn(
            "Padstacks.get: 1 <= padstackId <= " + std::to_string(count()) + " expected");
        return nullptr;
    }
    Padstack* result = padstacks_[p_padstack_id - 1].get();
    if (result->get_id() != p_padstack_id)
    {
        logger::FRLogger::instance().warn("Padstacks.get: inconsistent padstack ID");
    }
    return result;
}

Padstack* Padstacks::add(const std::string& p_name,
    std::vector<std::unique_ptr<geometry::planar::Shape>> p_shapes, bool p_drill_allowed,
    bool p_placed_absolute)
{
    padstacks_.push_back(
        std::make_unique<Padstack>(p_name, count() + 1, std::move(p_shapes), p_drill_allowed, p_placed_absolute));
    return padstacks_.back().get();
}

Padstack* Padstacks::add(std::vector<std::unique_ptr<geometry::planar::Shape>> p_shapes)
{
    return add("padstack#" + std::to_string(count() + 1), std::move(p_shapes), false, false);
}

} // namespace freerouting::board
