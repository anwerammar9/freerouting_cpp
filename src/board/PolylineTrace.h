#pragma once

#include <memory>
#include <vector>

#include "board/Trace.h"
#include "geometry/planar/Polyline.h"

namespace freerouting {
namespace board {

class PolylineTrace : public Trace
{
public:
    PolylineTrace(const std::shared_ptr<geometry::planar::Polyline>& p_polyline,
                int p_layer, int p_half_width,
                const std::vector<int>& p_net_no_arr,
                int p_clearance_type, int p_id_no, int p_group_no,
                FixedState p_fixed_state);

    const geometry::planar::Point& first_corner() const override;
    const geometry::planar::Point& last_corner() const override;
    const std::vector<std::shared_ptr<geometry::planar::Point>>& corners() const override;
    int corner_count() const override;
    const geometry::planar::Point& corner(int p_index) const override;

    std::shared_ptr<geometry::planar::TileShape> get_tile_shape() const override;

    std::unique_ptr<Item> clone() const override;
    std::unique_ptr<Item> copy(int p_id_no) const override;

    void translate_by(const std::shared_ptr<geometry::planar::Vector>& p_vector) override;

private:
    std::shared_ptr<geometry::planar::Polyline> polyline_;
};

} // namespace board
} // namespace freerouting