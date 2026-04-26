#pragma once

#include <memory>
#include <vector>
#include <string>

namespace freerouting {
namespace board {

class Layer;

class LayerStructure
{
public:
    explicit LayerStructure(const std::vector<std::shared_ptr<Layer>>& p_layers);

    int get_no(const std::string& p_name) const;
    int get_no(const std::shared_ptr<Layer>& p_layer) const;

    int signal_layer_count() const;
    std::shared_ptr<Layer> get_signal_layer(int p_no) const;
    int get_signal_layer_no(const std::shared_ptr<Layer>& p_layer) const;
    int get_layer_no(int p_signal_layer_no) const;

    int layer_count() const { return static_cast<int>(layers_.size()); }
    const std::shared_ptr<Layer>& get_layer(int p_no) const;

    std::string to_string() const;

private:
    std::vector<std::shared_ptr<Layer>> layers_;
};

} // namespace board
} // namespace freerouting