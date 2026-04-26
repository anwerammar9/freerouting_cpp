#pragma once

#include <chrono>
#include <string>
#include <optional>
#include <memory>

#include "logger/LogEntryType.h"

namespace freerouting {
namespace logger {

class LogEntry
{
public:
    LogEntry() = default;

    LogEntry(LogEntryType p_type, const std::string& p_message,
            const std::exception* p_exception = nullptr,
            const std::string& p_topic = "");

    LogEntryType get_type() const { return type_; }
    const std::string& get_message() const { return message_; }
    const std::string& get_topic() const { return topic_; }
    const std::chrono::time_point<std::chrono::system_clock>& get_timestamp() const { return timestamp_; }
    const std::exception* get_exception() const { return exception_.get(); }

    std::string to_string() const;

private:
    std::chrono::time_point<std::chrono::system_clock> timestamp_;
    LogEntryType type_ = LogEntryType::Info;
    std::string message_;
    std::shared_ptr<std::exception> exception_;
    std::string topic_;
};

} // namespace logger
} // namespace freerouting