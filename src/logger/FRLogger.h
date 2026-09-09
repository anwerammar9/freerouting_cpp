#pragma once

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace freerouting {
namespace logger {

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warn,
    Error
};

struct LogEntry
{
    std::chrono::steady_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::optional<std::string> topic;
    std::optional<std::string> exception;
};

class FRLogger
{
public:
    static FRLogger& instance();

    void set_log_level(LogLevel level);
    void set_log_level(std::string_view level_str);
    void disable_logging();
    void enable_logging();

    void trace(std::string_view msg);
    void debug(std::string_view msg);
    void info(std::string_view msg);
    void warn(std::string_view msg);
    void error(std::string_view msg);

    void trace_entry(std::string_view perf_id);
    double trace_exit(std::string_view perf_id);

    bool is_enabled() const;
    LogLevel current_level() const;

    const std::vector<LogEntry>& entries() const;
    void clear_entries();

    static std::string format_duration(double total_seconds);
    static std::string format_score(float score, int incomplete, int violations);

private:
    FRLogger() = default;

    void log(LogLevel level, std::string_view msg);

    std::string get_timestamp() const;
    std::string level_to_string(LogLevel level) const;

    std::atomic<bool> enabled_{true};
    std::atomic<LogLevel> log_level_{LogLevel::Info};

    std::unordered_map<int64_t, std::chrono::steady_clock::time_point> perf_data_;
    mutable std::mutex perf_mutex_;

    std::vector<LogEntry> entries_;
    mutable std::mutex entries_mutex_;
};

} // namespace logger
} // namespace freerouting

// Convenience macros
#define FR_LOG_TRACE(msg) freerouting::logger::FRLogger::instance().trace(msg)
#define FR_LOG_DEBUG(msg) freerouting::logger::FRLogger::instance().debug(msg)
#define FR_LOG_INFO(msg) freerouting::logger::FRLogger::instance().info(msg)
#define FR_LOG_WARN(msg) freerouting::logger::FRLogger::instance().warn(msg)
#define FR_LOG_ERROR(msg) freerouting::logger::FRLogger::instance().error(msg)

#define FR_TRACE_ENTRY(id) freerouting::logger::FRLogger::instance().trace_entry(id)
#define FR_TRACE_EXIT(id) freerouting::logger::FRLogger::instance().trace_exit(id)