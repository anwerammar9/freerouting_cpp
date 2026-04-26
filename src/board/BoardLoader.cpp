#include "board/BoardLoader.h"
#include "core/RoutingJob.h"
#include "core/FileFormat.h"
#include "logger/FRLogger.h"
#include "board/RoutingBoard.h"

#include <fstream>

namespace freerouting {
namespace board {

bool BoardLoader::load_board_if_needed(core::RoutingJob& job)
{
    logger::FRLogger::instance().info("BoardLoader stub - skipping board load");
    return false;
}

std::unique_ptr<RoutingBoard> BoardLoader::load_board(const std::string& filepath)
{
    logger::FRLogger::instance().info("Loading board from: " + filepath);
    return nullptr;
}

} // namespace board
} // namespace freerouting