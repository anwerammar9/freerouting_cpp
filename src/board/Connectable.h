#pragma once

#include <set>
#include <memory>
#include <vector>

namespace freerouting {
namespace board {

class Item;
class ShapeSearchTree;
class TileShape;

class Connectable
{
public:
    virtual ~Connectable() = default;

    virtual bool contains_net(int p_net_no) const = 0;
    virtual bool contains_net(int p_net_no, int p_layer) const = 0;
    virtual bool shares_net_no(const std::vector<int>& p_net_no_arr) const = 0;

    virtual std::set<std::shared_ptr<Item>> get_all_contacts() const = 0;
    virtual std::set<std::shared_ptr<Item>> get_all_contacts(int p_layer) const = 0;
    virtual std::set<std::shared_ptr<Item>> get_normal_contacts() const = 0;
    virtual std::set<std::shared_ptr<Item>> get_connected_set(int p_net_no) const = 0;

    virtual std::shared_ptr<TileShape> get_trace_connection_shape(
        const std::shared_ptr<ShapeSearchTree>& p_tree, int p_index) const = 0;
};

} // namespace board
} // namespace freerouting