#pragma once

#include <vector>
#include <memory>
#include <string>

#include "board/FixedState.h"
#include "board/Layer.h"
#include "board/BasicBoard.h"
#include "geometry/planar/IntBox.h"

namespace freerouting::geometry::planar {
class Polyline;
class Point;
}

namespace freerouting::autoroute {
class ItemAutorouteInfo;
}

namespace freerouting::board {

class Item
{
public:
    virtual ~Item() = default;

    int get_id() const { return id_no_; }

    const std::vector<int>& get_net_nos() const { return net_nos_; }

    int get_clearance_class() const { return clearance_class_; }
    void set_clearance_class(int p_class) { clearance_class_ = p_class; }

    FixedState get_fixed_state() const { return fixed_state_; }
    void set_fixed_state(FixedState p_state) { fixed_state_ = p_state; }

    int get_component_no() const { return component_no_; }
    void set_component_no(int p_no) { component_no_ = p_no; }

    ::freerouting::geometry::planar::IntBox get_bounding_box() const { return bounding_box_; }

    bool is_on_board() const { return on_board_; }
    void set_on_board(bool p_value) { on_board_ = p_value; }

    BasicBoard* get_board() const { return board_; }
    void set_board(BasicBoard* p_board) { board_ = p_board; }

    virtual int get_layer() const { return -1; }
    virtual bool is_routable() const { return true; }
    virtual bool is_trace() const { return false; }
    virtual bool is_via() const { return false; }
    virtual bool is_pin() const { return false; }

    virtual int tile_shape_count() const { return 0; }
    virtual ::freerouting::geometry::planar::IntOctagon get_tile_shape(int p_index) const;
    virtual int shape_layer(int p_index) const { return -1; }

    virtual std::shared_ptr<autoroute::ItemAutorouteInfo> get_autoroute_info();
    virtual std::vector<Item*> get_normal_contacts() const;
    virtual int first_common_layer(const Item* p_other) const;
    bool shares_net(const Item* p_other) const;

    virtual ::freerouting::geometry::planar::IntBox surrounding_octagon() const;
    virtual std::string to_string() const;

protected:
    Item(const std::vector<int>& p_net_nos, int p_clearance_class, int p_id_no,
         int p_component_no, FixedState p_fixed_state, BasicBoard* p_board);

    int id_no_;
    std::vector<int> net_nos_;
    int clearance_class_;
    int component_no_;
    FixedState fixed_state_;
    ::freerouting::geometry::planar::IntBox bounding_box_;
    bool on_board_;
    BasicBoard* board_;
    double smallest_clearance_;
    std::shared_ptr<autoroute::ItemAutorouteInfo> autoroute_info_;
};

class Trace : public Item
{
public:
    int get_layer() const override { return layer_; }
    void set_layer(int p_layer);

    int get_half_width() const { return half_width_; }

    virtual bool is_trace() const override { return true; }
    virtual double get_length() const;

    void set_polyline(geometry::planar::Polyline* p_polyline) { polyline_ = p_polyline; }

    virtual ::freerouting::geometry::planar::IntBox surrounding_octagon() const override;

protected:
    Trace(int p_layer, int p_half_width, const std::vector<int>& p_net_nos,
          int p_clearance_class, int p_id_no, int p_component_no,
          FixedState p_fixed_state, BasicBoard* p_board);

    int layer_;
    int half_width_;
    geometry::planar::Polyline* polyline_;
};

} // namespace freerouting::board