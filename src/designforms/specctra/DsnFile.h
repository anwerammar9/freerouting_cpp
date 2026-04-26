#pragma once

#include <memory>
#include <string>
#include <istream>
#include <ostream>

namespace freerouting {
namespace designforms {
namespace specctra {

enum class ReadResult
{
    OK,
    ERROR,
    INVALID_FORMAT
};

class DsnFile
{
public:
    static ReadResult read(std::istream& input_stream,
                    const std::shared_ptr<class BoardManager>& board_manager);

    static bool write(const std::shared_ptr<class BasicBoard>& p_board,
                 std::ostream& output_stream,
                 const std::string& design_name);

    static std::string get_last_error() { return last_error_; }
    static void set_last_error(const std::string& p_error) { last_error_ = p_error; }

private:
    static std::string last_error_;
};

} // namespace specctra
} // namespace designforms
} // namespace freerouting