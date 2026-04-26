#pragma once

#include <memory>

namespace freerouting {
namespace core { class RoutingJob; }
namespace board { class RoutingBoard; }

namespace board {

class BoardLoader
{
public:
    static bool load_board_if_needed(core::RoutingJob& job);
    static std::unique_ptr<RoutingBoard> load_board(const std::string& filepath);
};

} // namespace board
} // namespace freerouting