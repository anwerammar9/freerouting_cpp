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

LayerStructure::LayerStructure(const std::vector<Layer>& p_layer_arr)
    : arr(p_layer_arr)
{
}

int LayerStructure::get_no(const std::string& p_name) const
{
    for (int i = 0; i < static_cast<int>(arr.size()); ++i)
    {
        if (arr[i].name == p_name)
            return i;
    }
    return -1;
}

int LayerStructure::get_no(const Layer& p_layer) const
{
    for (int i = 0; i < static_cast<int>(arr.size()); ++i)
    {
        if (&arr[i] == &p_layer)
            return i;
    }
    return -1;
}

int LayerStructure::signal_layer_count() const
{
    int count = 0;
    for (const auto& layer : arr)
    {
        if (layer.is_signal)
            ++count;
    }
    return count;
}

Layer LayerStructure::get_signal_layer(int p_no) const
{
    int found = 0;
    for (const auto& layer : arr)
    {
        if (layer.is_signal)
        {
            if (found == p_no)
                return layer;
            ++found;
        }
    }
    return arr[0];
}

} // namespace board
} // namespace freerouting