#include "designforms/specctra/SessionWriter.h"
#include "board/RoutingBoard.h"
#include "board/PolylineTrace.h"
#include "board/Trace.h"
#include "board/Item.h"
#include "logger/FRLogger.h"

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace freerouting {
namespace designforms {
namespace specctra {

SessionWriter::SessionWriter(const std::string& p_filename) : filename_(p_filename) {}

bool SessionWriter::write_session(board::RoutingBoard& p_board, const std::string& p_board_name)
{
    std::ofstream out(filename_);
    if (!out.is_open()) {
        logger::FRLogger::instance().error("SessionWriter: Failed to open file for writing: " + filename_);
        return false;
    }

    // Group all traces by their primary net number.
    std::map<int, std::vector<board::PolylineTrace*>> traces_by_net;
    const std::vector<board::Item*> items = p_board.get_connectable_items();
    for (board::Item* item : items)
    {
        if (item == nullptr || !item->is_trace())
        {
            continue;
        }
        auto* trace = dynamic_cast<board::PolylineTrace*>(item);
        if (trace == nullptr)
        {
            continue;
        }
        const std::vector<int>& net_nos = trace->get_net_nos();
        if (net_nos.empty())
        {
            continue;
        }
        traces_by_net[net_nos[0]].push_back(trace);
    }

    std::string base_name = p_board_name;
    if (base_name.size() < 4 || base_name.compare(base_name.size() - 4, 4, ".dsn") != 0)
    {
        base_name += ".dsn";
    }

    out << "(session freerouting_session\n";
    out << "  (base_design \"" << base_name << "\")\n";

    out << "  (routes\n";
    out << "    (resolution_clearance 10)\n";

    for (const auto& [net_no, traces] : traces_by_net)
    {
        const std::string net_name = p_board.get_net_name(net_no);
        out << "    (net \"" << net_name << "\"\n";
        for (const board::PolylineTrace* trace : traces)
        {
            out << "      (path " << (2 * trace->get_half_width()) << " ";
            const std::vector<geometry::planar::IntPoint> corners = trace->polyline().corner_arr();
            for (const geometry::planar::IntPoint& corner : corners)
            {
                out << corner.x << " " << corner.y << " ";
            }
            out << ")\n";
        }
        out << "    )\n";
    }

    out << "  )\n";
    out << ")\n";
    out.close();

    logger::FRLogger::instance().info("SessionWriter: SES file written successfully: " + filename_
        + " (" + std::to_string(traces_by_net.size()) + " nets routed)");
    return true;
}

} // namespace specctra
} // namespace designforms
} // namespace freerouting