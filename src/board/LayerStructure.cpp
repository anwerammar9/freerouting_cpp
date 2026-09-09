#include "board/Layer.h"

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
        if (layers_[i].get() == p_layer.get())
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
    int signal_count = 0;
    for (const auto& layer : layers_)
    {
        if (layer->is_signal)
        {
            if (signal_count == p_no)
            {
                return layer;
            }
            ++signal_count;
        }
    }
    return nullptr;
}

int LayerStructure::get_signal_layer_no(const std::shared_ptr<Layer>& p_layer) const
{
    int signal_no = 0;
    for (const auto& layer : layers_)
    {
        if (layer->is_signal)
        {
            if (layer.get() == p_layer.get())
            {
                return signal_no;
            }
            ++signal_no;
        }
    }
    return -1;
}

int LayerStructure::get_layer_no(int p_signal_layer_no) const
{
    int signal_count = 0;
    for (size_t i = 0; i < layers_.size(); ++i)
    {
        if (layers_[i]->is_signal)
        {
            if (signal_count == p_signal_layer_no)
            {
                return static_cast<int>(i);
            }
            ++signal_count;
        }
    }
    return -1;
}

int LayerStructure::layer_count() const
{
    return static_cast<int>(layers_.size());
}

const std::shared_ptr<Layer>& LayerStructure::get_layer(int p_no) const
{
    return layers_[p_no];
}

std::string LayerStructure::to_string() const
{
    std::string result = "LayerStructure: ";
    result += std::to_string(layers_.size()) + " layers";
    return result;
}

} // namespace board
} // namespace freerouting