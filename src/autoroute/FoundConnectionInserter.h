#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "autoroute/AutorouteControl.h"
#include "autoroute/FoundConnectionLocator.h"
#include "geometry/planar/Point.h"

namespace freerouting {
namespace board {
class Pin;
class RoutingBoard;
} // namespace board

namespace autoroute {

/**
 * Inserts the traces and vias of the connection found by the autoroute
 * algorithm. Port of Java
 * app.freerouting.autoroute.path.FoundConnectionInserter (807 lines,
 * FoundConnectionInserter.java).
 *
 * Java names the locator class `FoundConnectionLocator` and its result items
 * are `FoundConnectionLocator.ResultItem` (corners + layer), which the C++
 * FoundConnectionLocator port provides.
 */
class FoundConnectionInserter
{
public:
    /**
     * Inserts the connection of p_connection into p_board. Returns nullptr,
     * if the insertion did not succeed. Port of Java
     * FoundConnectionInserter.getInstance().
     */
    static std::unique_ptr<FoundConnectionInserter> get_instance(
        const FoundConnectionLocator* p_connection,
        board::RoutingBoard* p_board,
        const AutorouteControl& p_ctrl);

    /** The last corner of the inserted connection, or nullptr if none. */
    const std::optional<geometry::planar::IntPoint>& last_corner() const
    {
        return last_corner_;
    }

    /** The first corner of the inserted connection, or nullptr if none. */
    const std::optional<geometry::planar::IntPoint>& first_corner() const
    {
        return first_corner_;
    }

private:
    FoundConnectionInserter(board::RoutingBoard* p_board, const AutorouteControl& p_ctrl);

    /**
     * Inserts the trace by shoving aside obstacle traces and vias. Returns
     * false, if that was not possible for the whole trace. Port of Java
     * FoundConnectionInserter.insertTrace().
     */
    bool insert_trace(const FoundConnectionLocator::ResultItem& p_trace);

    /**
     * Tries to insert a narrower trace to the target point, so that a trace
     * can leave a wide pin. Port of Java
     * FoundConnectionInserter.insertNeckdown().
     */
    bool insert_neckdown(const geometry::planar::IntPoint& p_from_corner,
                         const geometry::planar::IntPoint& p_to_corner,
                         int p_layer,
                         board::Pin* p_start_pin,
                         board::Pin* p_end_pin);

    /**
     * Tries to shrink a trace segment towards the pin, so that the trace can
     * reach the pin with its neckdown width. Returns p_from_corner if the
     * segment could not be necked down. Port of Java
     * FoundConnectionInserter.tryNeckDown().
     */
    std::optional<geometry::planar::IntPoint> try_neck_down(
        const geometry::planar::IntPoint& p_from_corner,
        const geometry::planar::IntPoint& p_to_corner,
        int p_layer,
        board::Pin* p_pin,
        bool p_at_start);

    /**
     * Tries a set of narrower trace widths for a fanout segment. Port of Java
     * FoundConnectionInserter.insertFanoutMicroNeckdown().
     */
    bool insert_fanout_micro_neckdown(const std::optional<geometry::planar::IntPoint>& p_ok_point,
                                      const geometry::planar::IntPoint& p_target_point,
                                      int p_layer,
                                      const std::vector<int>& p_net_nos,
                                      board::Pin* p_start_pin,
                                      board::Pin* p_end_pin);

    /**
     * Searches the cheapest via masks containing from_layer and to_layer, so
     * that a forced via is possible at location with this mask and inserts the
     * via. Returns false, if no suitable via mask was found or if the
     * algorithm failed. Port of Java FoundConnectionInserter.insertVia().
     */
    bool insert_via(const geometry::planar::IntPoint& p_location,
                    int p_input_from_layer,
                    int p_input_to_layer);

    bool should_trace_fanout_diagnostics() const;
    void trace_fanout_diagnostic(const std::string& p_event, const std::string& p_message) const;

    /** Trace half width of the routed layer, bounds checked. */
    int trace_half_width(int p_layer) const;

    /** Inserts a forced trace segment with the routed net and clearance class. */
    std::optional<geometry::planar::IntPoint> insert_forced_segment(
        const geometry::planar::IntPoint& p_from_corner,
        const geometry::planar::IntPoint& p_to_corner,
        int p_half_width,
        int p_layer) const;

    board::RoutingBoard* board_ = nullptr;
    const AutorouteControl* ctrl_ = nullptr;
    std::optional<geometry::planar::IntPoint> last_corner_;
    std::optional<geometry::planar::IntPoint> first_corner_;
};

} // namespace autoroute
} // namespace freerouting
