#include "board/Via.h"

namespace freerouting::board {

DrillItem::DrillItem(const geometry::planar::IntPoint& p_center,
                       const std::vector<int>& p_net_nos,
                       int p_clearance_class,
                       int p_id_no,
                       int p_component_no,
                       FixedState p_fixed_state,
                       BasicBoard* p_board)
    : Item(p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      center_(p_center),
      from_layer_(0),
      to_layer_(0),
      padstack_(nullptr)
{
}

Via::Via(Padstack* p_padstack,
             const geometry::planar::IntPoint& p_center,
             const std::vector<int>& p_net_nos,
             int p_clearance_class,
             int p_id_no,
             int p_component_no,
             FixedState p_fixed_state,
             bool p_attach_allowed,
             BasicBoard* p_board)
    : DrillItem(p_center, p_net_nos, p_clearance_class, p_id_no, p_component_no, p_fixed_state, p_board),
      attach_allowed_(p_attach_allowed)
{
    padstack_ = p_padstack;
}

Item* Via::copy(int p_id_no) const
{
    return new Via(padstack_, get_center(), net_nos_, clearance_class_,
                   p_id_no, component_no_, fixed_state_, attach_allowed_, board_);
}

std::string Via::to_string() const
{
    return "Via[id=" + std::to_string(id_no_) + "]";
}

} // namespace freerouting::board