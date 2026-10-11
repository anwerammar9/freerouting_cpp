#pragma once

#include <string>

namespace freerouting::autoroute {

class AutorouteControl;

/**
 * Optional targeted diagnostics for fanout maze expansion. Port of Java
 * app.freerouting.autoroute.maze.MazeFanoutDiagnostics (44 lines).
 *
 * The Java class only emits trace output for the fanout of one specific pin
 * ("U27-"), so it is a debug aid with no effect on the routing result.
 */
class MazeFanoutDiagnostics
{
public:
    explicit MazeFanoutDiagnostics(const AutorouteControl& p_ctrl) : ctrl_(p_ctrl) {}

    /** Returns true, if the targeted fanout diagnostics are enabled. */
    bool enabled() const;

    /** Returns the label of the fanout source pin used in the log output. */
    std::string label() const;

    /** Returns the label of the fanout source pin used in the log output. */
    std::string label_for_log() const { return label(); }

    /** Logs p_message for p_event, if the diagnostics are enabled. */
    void trace(const std::string& p_event, const std::string& p_message) const;

private:
    /** The control of the fanout pass this diagnostic belongs to. */
    const AutorouteControl& ctrl_;
};

} // namespace freerouting::autoroute