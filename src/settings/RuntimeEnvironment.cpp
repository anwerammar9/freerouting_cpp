#include "settings/RuntimeEnvironment.h"

namespace freerouting {
namespace settings {

RuntimeEnvironment::RuntimeEnvironment()
{
    app_started_at = std::chrono::system_clock::now();
}

std::string RuntimeEnvironment::to_string() const
{
    return "RuntimeEnvironment";
}

} // namespace settings
} // namespace freerouting