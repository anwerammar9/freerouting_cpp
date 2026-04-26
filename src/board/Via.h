#pragma once

#include "board/Item.h"
#include "geometry/planar/Point.h"

namespace freerouting::board {

class Padstack;

class DrillItem : public Item
{
public:
    const geometry::planar::IntPoint& get_center() const { return center_; }

    int get_layer() const override { return from_layer_; }
    int get_from_layer() const { return from_layer_; }
    int get_to_layer() const { return to_layer_; }

    Padstack* get_padstack() const { return padstack_; }
    void set_padstack(Padstack* p_padstack) { padstack_ = p_padstack; }

protected:
    DrillItem(const geometry::planar::IntPoint& p_center,
              const std::vector<int>& p_net_nos,
              int p_clearance_class,
              int p_id_no,
              int p_component_no,
              FixedState p_fixed_state,
              BasicBoard* p_board);

    geometry::planar::IntPoint center_;
    int from_layer_;
    int to_layer_;
    Padstack* padstack_;
};

class Via : public DrillItem
{
public:
    Via(Padstack* p_padstack,
        const geometry::planar::IntPoint& p_center,
        const std::vector<int>& p_net_nos,
        int p_clearance_class,
        int p_id_no,
        int p_component_no,
        FixedState p_fixed_state,
        bool p_attach_allowed,
        BasicBoard* p_board);

    bool is_via() const override { return true; }
    bool is_attach_allowed() const { return attach_allowed_; }

    Item* copy(int p_id_no) const;

    std::string to_string() const override;

private:
    bool attach_allowed_;
};

} // namespace freerouting::board