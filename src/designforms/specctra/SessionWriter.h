#pragma once

#include <string>

namespace freerouting {

namespace board {
class RoutingBoard;
}

namespace designforms {
namespace specctra {

class SessionWriter {
public:
    explicit SessionWriter(const std::string& p_filename);
    bool write_session(board::RoutingBoard& p_board, const std::string& p_board_name);
private:
    std::string filename_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting