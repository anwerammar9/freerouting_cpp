#include "board/BoardLibrary.h"

namespace freerouting::board {

BoardLibrary::BoardLibrary(int p_board_layer_count)
    : padstacks_(p_board_layer_count)
{
}

Padstack* BoardLibrary::get_via_padstack(int p_no) const
{
    if (p_no < 0 || p_no >= via_padstack_count())
    {
        return nullptr;
    }
    return via_padstacks_[p_no];
}

Padstack* BoardLibrary::get_via_padstack(const std::string& p_name) const
{
    for (Padstack* current_padstack : via_padstacks_)
    {
        if (current_padstack->get_name() == p_name)
        {
            return current_padstack;
        }
    }
    return nullptr;
}

bool BoardLibrary::add_via_padstack(Padstack* p_padstack)
{
    if (p_padstack == nullptr || get_via_padstack(p_padstack->get_name()) != nullptr)
    {
        return false;
    }
    via_padstacks_.push_back(p_padstack);
    return true;
}

Padstack* BoardLibrary::get_mirrored_via_padstack(Padstack* p_via_padstack) const
{
    const int layer_count = padstacks_.board_layer_count();
    if (p_via_padstack->get_from_layer() == 0 && p_via_padstack->get_to_layer() == layer_count - 1)
    {
        return p_via_padstack;
    }
    const int new_from_layer = layer_count - p_via_padstack->get_to_layer() - 1;
    const int new_to_layer = layer_count - p_via_padstack->get_from_layer() - 1;
    for (Padstack* current_via_padstack : via_padstacks_)
    {
        if (current_via_padstack->get_from_layer() == new_from_layer
            && current_via_padstack->get_to_layer() == new_to_layer)
        {
            return current_via_padstack;
        }
    }
    return nullptr;
}

} // namespace freerouting::board
