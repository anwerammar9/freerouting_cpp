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
    
private:
    std::locale current_locale_;
};

} // namespace management
} // namespace freerouting