#include "board/Communication.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace freerouting::board {

namespace {

std::string to_lower(std::string p_value)
{
    std::transform(p_value.begin(), p_value.end(), p_value.begin(),
        [](unsigned char p_char) { return static_cast<char>(std::tolower(p_char)); });
    return p_value;
}

} // namespace

Communication::Communication() = default;

Communication::Communication(Unit p_unit, int p_resolution, SpecctraParserInfo p_parser_info,
    double p_transform_scale_factor, double p_transform_base_x, double p_transform_base_y)
    : resolution_(p_resolution)
    , unit_(p_unit)
    , parser_info_(std::move(p_parser_info))
    , transform_scale_factor_(p_transform_scale_factor)
    , transform_base_x_(p_transform_base_x)
    , transform_base_y_(p_transform_base_y)
{
}

bool Communication::host_cad_is_fusion() const
{
    if (!parser_info_.has_host_cad())
    {
        return false;
    }
    const std::string cad = to_lower(parser_info_.host_cad);
    return cad.find("fusion") != std::string::npos || cad.find("cadsoft") != std::string::npos
        || cad.find("eagle") != std::string::npos;
}

bool Communication::host_is_old_kicad() const
{
    if (!parser_info_.has_host_cad() || parser_info_.host_version.empty())
    {
        return false;
    }
    if (to_lower(parser_info_.host_cad).find("kicad") == std::string::npos)
    {
        return false;
    }
    // Java: Pattern.compile("\\d+").matcher(hostVersion).find() -> Integer.parseInt(group) <= 5
    std::smatch match;
    if (std::regex_search(parser_info_.host_version, match, std::regex("\\d+")))
    {
        try
        {
            return std::stoi(match.str()) <= 5;
        }
        catch (const std::out_of_range&)
        {
            return false; // Java would throw NumberFormatException; treat as not old
        }
    }
    return false;
}

} // namespace freerouting::board
