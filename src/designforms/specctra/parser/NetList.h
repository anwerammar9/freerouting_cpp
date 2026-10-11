#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "designforms/specctra/parser/Net.h"

namespace freerouting::designforms::specctra::parser {

/** The nets of a dsn file sorted by their id. Port of Java parser.NetList (TreeMap<Net.Id, Net>). */
class NetList
{
public:
    /** True, if the net list contains a net with this id. Java contains. */
    bool contains(const Net::Id& p_net_id) const { return nets_.contains(p_net_id); }

    /** Adds a new net; returns nullptr (Java null) if a net with this id exists already. Java addNet. */
    Net* add_net(const Net::Id& p_net_id);

    /** The net with this id, or nullptr. Java getNet. */
    Net* get_net(const Net::Id& p_net_id) const;

    /** All nets containing the pin, in id order. Java getNets(componentName, pinName). */
    std::vector<Net*> get_nets(const std::string& p_component_name, const std::string& p_pin_name) const;

private:
    std::map<Net::Id, std::unique_ptr<Net>> nets_;
};

} // namespace freerouting::designforms::specctra::parser
