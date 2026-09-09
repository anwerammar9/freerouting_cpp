#include "logger/LogEntryType.h"

#include <string>
#include <unordered_map>

namespace freerouting {
namespace logger {

std::string to_string(LogEntryType p_type)
{
    switch (p_type)
    {
        case LogEntryType::Error: return "Error";
        case LogEntryType::Warning: return "Warning";
        case LogEntryType::Info: return "Info";
        case LogEntryType::Trace: return "Trace";
        case LogEntryType::Debug: return "Debug";
        default: return "Unknown";
    }
}

} // namespace logger
} // namespace freerouting