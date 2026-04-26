#pragma once

#include <string>
#include <vector>

namespace freerouting {
namespace board {

class Layer
{
public:
    const std::string name;
    const bool is_signal;

    Layer(const std::string& p_name, bool p_is_signal);

    std::string to_string() const;
};

class LayerStructure
{
public:
    std::vector<Layer> arr;

    LayerStructure(const std::vector<Layer>& p_layer_arr);

    int get_no(const std::string& p_name) const;
    int get_no(const Layer& p_layer) const;
    int signal_layer_count() const;
    Layer get_signal_layer(int p_no) const;
    int layer_count() const { return static_cast<int>(arr.size()); }
};

} // namespace board
} // namespace freerouting