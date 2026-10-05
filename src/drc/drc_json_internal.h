#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

/**
 * Internal hand-rolled JSON helpers shared by the DRC report/summary
 * serializers. The project has no JSON library; this keeps formatting
 * identical between DrcReport and DrcSummaryResponse.
 */
namespace freerouting {
namespace drc {
namespace detail {

inline std::string json_escape(const std::string& p_value)
{
    std::string result;
    result.reserve(p_value.size() + 8);
    for (char c : p_value)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                std::ostringstream hex;
                hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(c));
                result += hex.str();
            }
            else
            {
                result += c;
            }
        }
    }
    return result;
}

inline void json_append_string(std::string& o_json, const std::string& p_value)
{
    o_json.push_back('"');
    o_json += json_escape(p_value);
    o_json.push_back('"');
}

inline void json_append_double(std::string& o_json, double p_value)
{
    const int64_t as_int = static_cast<int64_t>(p_value);
    if (static_cast<double>(as_int) == p_value)
    {
        o_json += std::to_string(as_int);
    }
    else
    {
        std::ostringstream stream;
        stream << std::setprecision(12) << p_value;
        o_json += stream.str();
    }
}

} // namespace detail
} // namespace drc
} // namespace freerouting