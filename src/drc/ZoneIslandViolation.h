#pragma once

#include <memory>
#include <string>
#include <vector>

#include "board/ConductionArea.h"
#include "board/Item.h"
#include "geometry/planar/Point.h"

namespace freerouting {
namespace drc {

/**
 * Represents a DRC violation where a copper pour (conduction area) is
 * fragmented into disconnected islands, causing isolated pins or floating
 * dead copper. Port of Java app.freerouting.drc.ZoneIslandViolation.
 */
class ZoneIslandViolation
{
public:
    ZoneIslandViolation(board::ConductionArea* p_conduction_area,
                        std::vector<geometry::planar::FloatPoint> p_island_contour,
                        int p_layer,
                        int p_net_number,
                        const std::string& p_type,
                        std::vector<board::Item*> p_items_in_island);

    board::ConductionArea* get_conduction_area() const { return conduction_area_; }
    const std::vector<geometry::planar::FloatPoint>& get_island_contour() const
    {
        return island_contour_;
    }
    int get_layer() const { return layer_; }
    int get_net_number() const { return net_number_; }
    const std::string& get_type() const { return type_; }
    const std::vector<board::Item*>& get_items_in_island() const { return items_in_island_; }

    /** Returns the center of the bounding box of this island in board units. */
    double get_center_x() const;
    double get_center_y() const;

    /** Returns the approximate area in square board units (bounding box). */
    double get_area_in_board_units() const;

    /** True, if the island contour is empty. */
    bool is_empty() const { return island_contour_.empty(); }

private:
    board::ConductionArea* conduction_area_;
    std::vector<geometry::planar::FloatPoint> island_contour_;
    int layer_;
    int net_number_;
    std::string type_;
    std::vector<board::Item*> items_in_island_;
};

} // namespace drc
} // namespace freerouting