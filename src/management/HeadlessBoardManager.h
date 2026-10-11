#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "designforms/specctra/BoardReadResult.h"

namespace freerouting::board {
class RoutingBoard;
} // namespace freerouting::board

namespace freerouting::core {
class RoutingJob;
} // namespace freerouting::core

namespace freerouting::management {

/**
 * Owns the board of a routing job without any GUI: loads it from a Specctra DSN file and saves the
 * routing result as a Specctra session file. Port of the headless core of Java
 * management.HeadlessBoardManager (constructor, getRoutingBoard, replaceRoutingBoard,
 * getCurrentRoutingJob, loadFromSpecctraDsn, applyParsedBoardResult, saveAsSpecctraSessionSes).
 * The router-settings overrides and the post-load processing follow in T124b / T124c.
 */
class HeadlessBoardManager
{
public:
    /** Java HeadlessBoardManager(RoutingJob); p_routing_job may be nullptr. */
    explicit HeadlessBoardManager(core::RoutingJob* p_routing_job);

    /** The routing board, or nullptr if no board was loaded. Java getRoutingBoard. */
    board::RoutingBoard* get_routing_board() const { return board_.get(); }
    /** The board with shared ownership (for RoutingJob::board). */
    const std::shared_ptr<board::RoutingBoard>& get_shared_routing_board() const { return board_; }

    /** Java replaceRoutingBoard. */
    void replace_routing_board(std::shared_ptr<board::RoutingBoard> p_new_routing_board);

    /** Java getCurrentRoutingJob. */
    core::RoutingJob* get_current_routing_job() const { return routing_job_; }

    /**
     * Reads a DSN stream with DsnReader and takes the board over (Java loadFromSpecctraDsn).
     * p_input_file_name is the job's input file name used in log messages (may be empty).
     * On Success / OutlineMissing the returned result's board has been moved into this manager.
     */
    designforms::specctra::BoardReadResult load_from_specctra_dsn(
        std::istream& p_input_stream, const std::string& p_input_file_name = {});

    /**
     * Takes the board of a parsed result over (Java applyParsedBoardResult); parse and I/O errors are
     * logged. The router-settings overrides and post-load processing are not ported yet (T124b/c).
     */
    designforms::specctra::BoardReadResult apply_parsed_board_result(
        designforms::specctra::BoardReadResult p_dsn_result, const std::string& p_input_file_name);

    /** Writes the routing result as a Specctra session file. Java saveAsSpecctraSessionSes. */
    bool save_as_specctra_session_ses(std::ostream& p_output_stream, const std::string& p_design_name);

private:
    core::RoutingJob* routing_job_;
    std::shared_ptr<board::RoutingBoard> board_;
};

} // namespace freerouting::management
