#include "autoroute/MazeFanoutDiagnostics.h"

#include "autoroute/AutorouteControl.h"
#include "logger/FRLogger.h"

namespace freerouting::autoroute {

bool MazeFanoutDiagnostics::enabled() const
{
    return ctrl_.is_fanout && !ctrl_.fanout_start_pin_name.empty()
        && ctrl_.fanout_start_pin_name.rfind("U27-", 0) == 0;
}

std::string MazeFanoutDiagnostics::label() const
{
    if (ctrl_.fanout_start_pin_name.empty())
    {
        return "fanout-pin(net=" + std::to_string(ctrl_.net_no) + ")";
    }
    return ctrl_.fanout_start_pin_name;
}

void MazeFanoutDiagnostics::trace(const std::string& p_event, const std::string& p_message) const
{
    if (!enabled())
    {
        return;
    }
    logger::FRLogger::instance().trace("FANOUT_DIAG event=" + p_event + ", pin=" + label()
        + ", net=" + std::to_string(ctrl_.net_no) + ", " + p_message);
}

} // namespace freerouting::autoroute