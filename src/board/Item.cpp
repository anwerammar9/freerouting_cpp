#include "board/Item.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/BasicBoard.h"
#include <algorithm>

namespace freerouting::board {

Item::Item(const std::vector<int>& p_net_nos, int p_clearance_class, int p_id_no,
             int p_component_no, FixedState p_fixed_state, BasicBoard* p_board)
    : id_no_(p_id_no),
      net_nos_(p_net_nos),
      clearance_class_(p_clearance_class),
      component_no_(p_component_no),
      fixed_state_(p_fixed_state),
      on_board_(false),
      board_(p_board),
      smallest_clearance_(0)
{
}

std::string Item::to_string() const
{
    return "Item[id=" + std::to_string(id_no_) + "]";
}

std::shared_ptr<autoroute::ItemAutorouteInfo> Item::get_autoroute_info()
{
    if (!autoroute_info_)
    {
        autoroute_info_ = std::make_shared<autoroute::ItemAutorouteInfo>(this);
    }
    return autoroute_info_;
}

std::vector<Item*> Item::get_normal_contacts() const
{
    if (!board_) return {};
    // Stub: actual implementation would query the board's search trees
    return {};
}

int Item::first_common_layer(const Item* p_other) const
{
    if (!p_other) return -1;
    // Stub: simplified for now
    return -1;
}

bool Item::shares_net(const Item* p_other) const
{
    if (!p_other) return false;
    for (int net1 : net_nos_)
    {
        for (int net2 : p_other->net_nos_)
        {
            if (net1 == net2) return true;
        }
    }
    return false;
}

::freerouting::geometry::planar::IntOctagon Item::get_tile_shape(int p_index) const
{
    (void)p_index;
    // Return bounding octagon as default
    return bounding_box_.bounding_octagon();
}

::freerouting::geometry::planar::IntBox Item::surrounding_octagon() const
{
    return bounding_box_;
}

Trace::Trace(int p_layer, int p_half_width, const std::vector<int>& p_net_nos,
                 int p_clearance_class, int p_id_no, int p_component_no,
                 FixedState p_fixed_state, BasicBoard* p_board)
    : Item(p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      layer_(p_layer),
      half_width_(p_half_width),
      polyline_(nullptr)
{
}

void Trace::set_layer(int p_layer)
{
    layer_ = p_layer;
}

::freerouting::geometry::planar::IntBox Trace::surrounding_octagon() const
{
    return ::freerouting::geometry::planar::IntBox::EMPTY;
}

double Trace::get_length() const
{
    // Stub: return 0 or actual length if polyline is implemented
    return 0.0;
}

} // namespace freerouting::board