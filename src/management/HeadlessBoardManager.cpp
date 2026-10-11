#include "management/HeadlessBoardManager.h"

#include "board/RoutingBoard.h"
#include "core/RoutingJob.h"
#include "designforms/specctra/DsnReader.h"
#include "designforms/specctra/SessionWriter.h"
#include "logger/FRLogger.h"

namespace freerouting::management {

namespace bread = designforms::specctra::board_read_result;

HeadlessBoardManager::HeadlessBoardManager(core::RoutingJob* p_routing_job)
    : routing_job_(p_routing_job)
{
}

void HeadlessBoardManager::replace_routing_board(std::shared_ptr<board::RoutingBoard> p_new_routing_board)
{
    board_ = std::move(p_new_routing_board);
}

designforms::specctra::BoardReadResult HeadlessBoardManager::load_from_specctra_dsn(
    std::istream& p_input_stream, const std::string& p_input_file_name)
{
    logger::FRLogger::instance().info(
        "Loading board file" + (p_input_file_name.empty() ? std::string() : " '" + p_input_file_name + "'") + "...");
    designforms::specctra::BoardReadResult dsn_result =
        designforms::specctra::DsnReader::read_board(p_input_stream, p_input_file_name);
    return apply_parsed_board_result(std::move(dsn_result), p_input_file_name);
}

designforms::specctra::BoardReadResult HeadlessBoardManager::apply_parsed_board_result(
    designforms::specctra::BoardReadResult p_dsn_result, const std::string& /*p_input_file_name*/)
{
    if (auto* success = std::get_if<bread::Success>(&p_dsn_result))
    {
        board_ = std::move(success->board);
    }
    else if (auto* outline_missing = std::get_if<bread::OutlineMissing>(&p_dsn_result))
    {
        board_ = std::move(outline_missing->board);
    }
    else if (const auto* io_error = std::get_if<bread::IoError>(&p_dsn_result))
    {
        logger::FRLogger::instance().error("There was an IO error while reading board file: " + io_error->cause);
        return p_dsn_result;
    }
    else if (const auto* parse_error = std::get_if<bread::ParseError>(&p_dsn_result))
    {
        logger::FRLogger::instance().error("There was a parse error while reading board file at '"
            + parse_error->location + "': " + parse_error->detail);
        return p_dsn_result;
    }
    // T124b: applyRouterSettingsForLoadedBoard; T124c: immediate/deferred post-load processing.
    return p_dsn_result;
}

bool HeadlessBoardManager::save_as_specctra_session_ses(std::ostream& p_output_stream, const std::string& p_design_name)
{
    if (board_ == nullptr)
    {
        logger::FRLogger::instance().error("unable to write session file: no board loaded");
        return false;
    }
    designforms::specctra::SessionWriter::write(*board_, p_output_stream, p_design_name);
    if (!p_output_stream.good())
    {
        logger::FRLogger::instance().error("unable to write session file");
        return false;
    }
    // Java: originalBoardChecksum = calculateCrc32() (needs the DSN writer, T204c).
    return true;
}

} // namespace freerouting::management
