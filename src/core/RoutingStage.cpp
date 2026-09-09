#include "RoutingStage.h"

namespace freerouting {
namespace core {

std::string to_string(RoutingStage stage) {
    switch (stage) {
        case RoutingStage::IDLE:
            return "IDLE";
        case RoutingStage::ROUTING:
            return "ROUTING";
        case RoutingStage::OPTIMIZATION:
            return "OPTIMIZATION";
        default:
            return "IDLE";
    }
}

std::string get_stage_description(RoutingStage stage) {
    switch (stage) {
        case RoutingStage::IDLE:
            return "The job hasn't started yet";
        case RoutingStage::ROUTING:
            return "The board is currently being processed by the auto-router";
        case RoutingStage::OPTIMIZATION:
            return "The board is routed, and it is currently being optimized";
        default:
            return "Unknown routing stage";
    }
}

} // namespace core
} // namespace freerouting
