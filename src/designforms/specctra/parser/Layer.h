#pragma once

#include <memory>
#include <string>
#include <vector>

namespace freerouting::designforms::specctra::parser {

/** A layer as read from a DSN file. Port of Java parser.Layer. */
class Layer
{
public:
    Layer(std::string p_name, int p_no, bool p_is_signal, std::vector<std::string> p_net_names = {})
        : name(std::move(p_name)), no(p_no), is_signal(p_is_signal), net_names(std::move(p_net_names)) {}

    /** Java Layer.PCB: all layers of the board. */
    static const std::shared_ptr<Layer>& pcb();
    /** Java Layer.SIGNAL: all signal layers. */
    static const std::shared_ptr<Layer>& signal();

    const std::string name;
    const int no;
    const bool is_signal;
    /** The nets of a power plane layer. */
    std::vector<std::string> net_names;
};

/** The layers of a DSN file. Port of Java parser.LayerStructure. */
class LayerStructure
{
public:
    explicit LayerStructure(std::vector<std::shared_ptr<Layer>> p_layers) : layers(std::move(p_layers)) {}

    /**
     * The index of the layer named p_name, or -1. Java quirk kept: an unknown
     * name containing "Top" maps to the first and "Bottom" to the last layer.
     */
    int get_no(const std::string& p_name) const;
    int signal_layer_count() const;
    /** True, if a plane layer lists p_net_name. */
    bool contains_plane(const std::string& p_net_name) const;

    std::vector<std::shared_ptr<Layer>> layers;
};

} // namespace freerouting::designforms::specctra::parser
