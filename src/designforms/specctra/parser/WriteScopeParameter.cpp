#include "designforms/specctra/parser/WriteScopeParameter.h"

#include <utility>

namespace freerouting::designforms::specctra::parser {

WriteScopeParameter::WriteScopeParameter(board::BasicBoard* p_board,
    settings::RouterSettings* p_autoroute_settings,
    datastructures::IndentFileWriter& p_file,
    std::string p_string_quote,
    CoordinateTransform p_coordinate_transform,
    bool p_compat_mode)
    : board(p_board)
    , autoroute_settings(p_autoroute_settings)
    , file(p_file)
    , coordinate_transform(p_coordinate_transform)
    , compat_mode(p_compat_mode)
    , identifier_type({ "(", ")", " ", ";", "-", "_" }, std::move(p_string_quote))
{
}

} // namespace freerouting::designforms::specctra::parser
