#include "management/TextManager.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace freerouting {
namespace management {

TextManager::TextManager(const std::locale& locale)
    : current_locale_(locale)
{
}

std::string TextManager::get_text(const std::string& key) const
{
    // TODO: Implement proper resource lookup
    return key;
}

std::string TextManager::convert_instant_to_string(const std::chrono::system_clock::time_point& instant)
{
    // Simple datetime to string conversion
    auto time_t = std::chrono::system_clock::to_time_t(instant);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string TextManager::remove_quotes(const std::string& p_text)
{
    if (p_text.size() < 2 || p_text.front() != '"' || p_text.back() != '"') {
        return p_text;
    }
    return p_text.substr(1, p_text.size() - 2);
}

std::string TextManager::unescape_unicode(const std::string& p_text)
{
    std::string result;
    result.reserve(p_text.size());
    for (std::size_t i = 0; i < p_text.size();) {
        if (i + 6 <= p_text.size() && p_text[i] == '\\' && p_text[i + 1] == 'u') {
            bool valid_hex = true;
            unsigned int value = 0;
            for (int k = 0; k < 4; ++k) {
                char c = p_text[i + 2 + k];
                unsigned int digit;
                if (c >= '0' && c <= '9') {
                    digit = static_cast<unsigned int>(c - '0');
                } else if (c >= 'a' && c <= 'f') {
                    digit = static_cast<unsigned int>(c - 'a' + 10);
                } else if (c >= 'A' && c <= 'F') {
                    digit = static_cast<unsigned int>(c - 'A' + 10);
                } else {
                    valid_hex = false;
                    break;
                }
                value = value * 16 + digit;
            }
            if (valid_hex) {
                if (value < 0x80) {
                    result.push_back(static_cast<char>(value));
                } else if (value < 0x800) {
                    result.push_back(static_cast<char>(0xC0 | (value >> 6)));
                    result.push_back(static_cast<char>(0x80 | (value & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xE0 | (value >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (value & 0x3F)));
                }
                i += 6;
                continue;
            }
        }
        result.push_back(p_text[i]);
        ++i;
    }
    return result;
}

} // namespace management
} // namespace freerouting