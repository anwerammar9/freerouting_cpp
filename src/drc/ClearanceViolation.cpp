#include "drc/ClearanceViolation.h"

#include <sstream>

namespace freerouting::drc {

ClearanceViolation::ClearanceViolation(board::Item* p_first_item,
                                       board::Item* p_second_item,
                                       geometry::planar::Shape* p_shape,
                                       int p_layer,
                                       double p_expected_clearance,
                                       double p_actual_clearance)
    : first_item_(p_first_item)
    , second_item_(p_second_item)
    , shape_(p_shape)
    , layer_(p_layer)
    , expected_clearance_(p_expected_clearance)
    , actual_clearance_(p_actual_clearance)
{
}

std::string ClearanceViolation::to_string() const
{
    std::ostringstream ss;
    ss << "ClearanceViolation[layer=" << layer_
       << ", expected=" << expected_clearance_
       << ", actual=" << actual_clearance_ << "]";
    return ss.str();
}

} // namespace freerouting::drc