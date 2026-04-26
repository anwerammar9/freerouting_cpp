#pragma once

#include <string>
#include <vector>
#include <memory>

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
    LayerStructure(const std::vector<std::shared_ptr<Layer>>& p_layers);

    int get_no(const std::string& p_name) const;
    int get_no(const std::shared_ptr<Layer>& p_layer) const;

    int signal_layer_count() const;
    std::shared_ptr<Layer> get_signal_layer(int p_no) const;
    int get_signal_layer_no(const std::shared_ptr<Layer>& p_layer) const;
    int get_layer_no(int p_signal_layer_no) const;

    int layer_count() const;
    const std::shared_ptr<Layer>& get_layer(int p_no) const;

    std::string to_string() const;

private:
    std::vector<std::shared_ptr<Layer>> layers_;
};

} // namespace board
} // namespace freerouting