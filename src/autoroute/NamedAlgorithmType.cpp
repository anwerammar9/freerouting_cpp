#include "autoroute/NamedAlgorithmType.h"

namespace freerouting {
namespace autoroute {

std::string to_string(NamedAlgorithmType type) {
    switch (type) {
        case NamedAlgorithmType::ROUTER:
            return "ROUTER";
        case NamedAlgorithmType::OPTIMIZER:
            return "OPTIMIZER";
        default:
            return "ROUTER";
    }
}

std::string get_algorithm_description(NamedAlgorithmType type) {
    switch (type) {
        case NamedAlgorithmType::ROUTER:
            return "Algorithm for routing PCB traces and connections";
        case NamedAlgorithmType::OPTIMIZER:
            return "Algorithm for optimizing existing PCB routes";
        default:
            return "Unknown algorithm type";
    }
}

} // namespace autoroute
} // namespace freerouting
