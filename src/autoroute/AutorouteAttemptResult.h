#ifndef AUTOROUTE_ATTEMPT_RESULT_H
#define AUTOROUTE_ATTEMPT_RESULT_H

#include <string>
#include "autoroute/AutorouteAttemptState.h"

namespace freerouting {
namespace autoroute {

/**
 * The result of an auto-route attempt.
 */
class AutorouteAttemptResult {
public:
    AutorouteAttemptState state;
    std::string details;

    AutorouteAttemptResult();
    AutorouteAttemptResult(AutorouteAttemptState p_state);
    AutorouteAttemptResult(AutorouteAttemptState p_state, const std::string& p_details);

    std::string to_string() const;
};

} // namespace autoroute
} // namespace freerouting

#endif // AUTOROUTE_ATTEMPT_RESULT_H