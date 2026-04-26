#include "logger/LogEntry.h"

#include <sstream>

namespace freerouting {
namespace logger {

LogEntry::LogEntry(LogEntryType p_type, const std::string& p_message,
                 const std::exception* p_exception, const std::string& p_topic)
    : timestamp_(std::chrono::system_clock::now())
    , type_(p_type)
    , message_(p_message)
    , topic_(p_topic)
{
    if (p_exception)
    {
        exception_ = std::shared_ptr<std::exception>(p_exception, [](const std::exception*){});
    }
}

std::string LogEntry::to_string() const
{
    std::ostringstream ss;
    ss << to_string(type_) << " " << message_;
    return ss.str();
}

} // namespace logger
} // namespace freerouting