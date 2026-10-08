#include "designforms/specctra/parser/Layer.h"

#include <algorithm>

namespace freerouting::designforms::specctra::parser {

const std::shared_ptr<Layer>& Layer::pcb()
{
    static const std::shared_ptr<Layer> kPcb = std::make_shared<Layer>("pcb", -1, false);
    return kPcb;
}

const std::shared_ptr<Layer>& Layer::signal()
{
    static const std::shared_ptr<Layer> kSignal = std::make_shared<Layer>("signal", -1, true);
    return kSignal;
}

int LayerStructure::get_no(const std::string& p_name) const
{
    for (size_t i = 0; i < layers.size(); ++i)
    {
        if (p_name == layers[i]->name)
        {
            return static_cast<int>(i);
        }
    }
    if (p_name.find("Top") != std::string::npos)
    {
        return 0;
    }
    if (p_name.find("Bottom") != std::string::npos)
    {
        return static_cast<int>(layers.size()) - 1;
    }
    return -1;
}

int LayerStructure::signal_layer_count() const
{
    return static_cast<int>(std::count_if(layers.begin(), layers.end(), [](const auto& l) { return l->is_signal; }));
}

bool LayerStructure::contains_plane(const std::string& p_net_name) const
{
    for (const auto& current_layer : layers)
    {
        if (!current_layer->is_signal
            && std::find(current_layer->net_names.begin(), current_layer->net_names.end(), p_net_name)
                != current_layer->net_names.end())
        {
            return true;
        }
    }
    return false;
}

} // namespace freerouting::designforms::specctra::parser
