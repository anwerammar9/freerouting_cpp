#pragma once

#include <string>
#include <locale>
#include <chrono>

namespace freerouting {
namespace management {

class TextManager
{
public:
    TextManager(const std::locale& locale);
    
    std::string get_text(const std::string& key) const;
    static std::string convert_instant_to_string(const std::chrono::system_clock::time_point& instant);

    /**
     * Removes leading and trailing double quotes from p_text if present.
     * Mirrors Java TextManager.removeQuotes().
     */
    static std::string remove_quotes(const std::string& p_text);

    /**
     * Replaces all \\uXXXX escape sequences by the corresponding unicode
     * characters. Mirrors Java TextManager.unescapeUnicode().
     */
    static std::string unescape_unicode(const std::string& p_text);
    
private:
    std::locale current_locale_;
};

} // namespace management
} // namespace freerouting