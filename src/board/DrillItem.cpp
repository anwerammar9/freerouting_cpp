#include "board/DrillItem.h"
#include "board/Item.h"
#include "board/Connectable.h"
#include "geometry/planar/Point.h"
#include "geometry/planar/Vector.h"
#include "geometry/planar/TileShape.h"
#include "geometry/planar/Shape.h"
#include "logger/FRLogger.h"

#include <algorithm>
#include <sstream>

namespace freerouting {
namespace board {

DrillItem::DrillItem(const std::shared_ptr<geometry::planar::Point>& p_center,
                     const std::vector<int>& p_net_no_arr,
                     int p_clearance_type, int p_id_no, int p_group_no,
                     FixedState p_fixed_state)
    : Item(p_net_no_arr, p_clearance_type, p_id_no, p_group_no, p_fixed_state),
      center_(p_center)
{
    logger::FRLogger::instance().info("Creating DrillItem");
}

double DrillItem::get_min_width() const
{
    if (precalculated_min_width_ < 0) {
        // Calculate minimal width from shapes on all layers
        // TODO: Implement proper width calculation
        precalculated_min_width_ = 0.0; // Placeholder
        
        // For now, just return a default value
        logger::FRLogger::instance().debug("DrillItem::get_min_width - using placeholder implementation");
    }
    return precalculated_min_width_;
}

void DrillItem::translate_by(const std::shared_ptr<geometry::planar::Vector>& p_vector)
{
    if (p_vector) {
        // TODO: Implement proper point translation
        // center_ = center_->translate_by(p_vector);
        logger::FRLogger::instance().info("DrillItem::translate_by - placeholder implementation");
    }
}

std::string DrillItem::to_string() const
{
    std::ostringstream oss;
    oss << "DrillItem{";
    
    if (center_) {
        oss << "center=" << center_->to_string();
    } else {
        oss << "center=null";
    }
    
    oss << ", min_width=" << get_min_width();
    oss << ", first_layer=" << get_first_layer();
    oss << ", last_layer=" << get_last_layer();
    
    // Add net information
    oss << ", nets=[";
    const auto& net_numbers = get_net_no_arr();
    for (size_t i = 0; i < net_numbers.size(); ++i) {
        if (i > 0) oss << ",";
        oss << net_numbers[i];
    }
    oss << "]";
    
    oss << "}";
    return oss.str();
}

bool DrillItem::contains_net(int p_net_no) const
{
    const auto& net_numbers = get_net_no_arr();
    return std::find(net_numbers.begin(), net_numbers.end(), p_net_no) != net_numbers.end();
}

bool DrillItem::shares_net_no(const std::vector<int>& p_net_no_arr) const
{
    const auto& my_net_numbers = get_net_no_arr();
    
    for (int net_no : p_net_no_arr) {
        if (contains_net(net_no)) {
            return true;
        }
    }
    return false;
}

} // namespace board
} // namespace freerouting
