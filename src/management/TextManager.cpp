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

} // namespace management
} // namespace freerouting