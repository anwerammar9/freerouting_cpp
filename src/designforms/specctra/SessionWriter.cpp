#include "designforms/specctra/SessionWriter.h"
#include "board/RoutingBoard.h"
#include "logger/FRLogger.h"

#include <fstream>
#include <iostream>

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

    // Write minimal SES header
    out << "(session freerouting_session\n";
    out << "  (base_design \"" << p_board_name << ".dsn\")\n";
    
    // Write routes
    out << "  (routes\n";
    out << "    (resolution_clearance 10)\n";
    // TODO: Iterate over routed nets and write (net ...) paths here
    out << "  )\n";
    
    out << ")\n";
    out.close();

    logger::FRLogger::instance().info("SessionWriter: SES file written successfully: " + filename_);
    return true;
}

} // namespace specctra
} // namespace designforms
} // namespace freerouting