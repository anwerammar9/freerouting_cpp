#pragma once

#include <string>

namespace freerouting {
namespace logger {

enum class LogEntryType
{
    Error,
    Warning,
    Info,
    Trace,
    Debug
};

std::string to_string(LogEntryType p_type);

} // namespace logger
} // namespace freerouting