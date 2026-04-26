#include "board/LayerStructure.h"

#include <algorithm>

namespace freerouting {
namespace board {

LayerStructure::LayerStructure(const std::vector<std::shared_ptr<Layer>>& p_layers)
    : layers_(p_layers)
{
}

int LayerStructure::get_no(const std::string& p_name) const
{
    for (size_t i = 0; i < layers_.size(); ++i)
    {
        if (layers_[i]->name == p_name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int LayerStructure::get_no(const std::shared_ptr<Layer>& p_layer) const
{
    for (size_t i = 0; i < layers_.size(); ++i)
    {
        if (layers_[i] == p_layer)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int LayerStructure::signal_layer_count() const
{
    int count = 0;
    for (const auto& layer : layers_)
    {
        if (layer->is_signal)
        {
            ++count;
        }
    }
    return count;
}

std::shared_ptr<Layer> LayerStructure::get_signal_layer(int p_no) const
{
    int found = 0;
    for (const auto& layer : layers_)
    {
        if (layer->is_signal)
        {
            if (found == p_no)
            {
                return layer;
            }
            ++found;
        }
    }
    return layers_.empty() ? nullptr : layers_.back();
}

int LayerStructure::get_signal_layer_no(const std::shared_ptr<Layer>& p_layer) const
{
    int found = 0;
    for (const auto& layer : layers_)
    {
        if (layer == p_layer)
        {
            return found;
        }
        if (layer->is_signal)
        {
            ++found;
        }
    }
    return -1;
}

int LayerStructure::get_layer_no(int p_signal_layer_no) const
{
    auto layer = get_signal_layer(p_signal_layer_no);
    return layer ? get_no(layer) : -1;
}

const std::shared_ptr<Layer>& LayerStructure::get_layer(int p_no) const
{
    static std::shared_ptr<Layer> null;
    if (p_no < 0 || p_no >= layer_count())
    {
        return null;
    }
    return layers_[p_no];
}

std::string LayerStructure::to_string() const
{
    return "LayerStructure(" + std::to_string(layer_count()) + " layers)";
}

} // namespace board
} // namespace freerouting