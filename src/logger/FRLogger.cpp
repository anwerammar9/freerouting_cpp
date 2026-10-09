#include "logger/FRLogger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace freerouting {
namespace logger {

FRLogger& FRLogger::instance()
{
    static FRLogger logger;
    return logger;
}

void FRLogger::set_log_level(LogLevel level)
{
    log_level_.store(level);
}

void FRLogger::set_log_level(std::string_view level_str)
{
    std::string upper;
    upper.reserve(level_str.size());
    for (char c : level_str)
    {
        upper += static_cast<char>(std::toupper(c));
    }

    if (upper == "OFF" || upper == "0")
    {
        disable_logging();
    }
    else if (upper == "FATAL" || upper == "1")
    {
        set_log_level(LogLevel::Error);
    }
    else if (upper == "ERROR" || upper == "2")
    {
        set_log_level(LogLevel::Error);
    }
    else if (upper == "WARN" || upper == "3")
    {
        set_log_level(LogLevel::Warn);
    }
    else if (upper == "INFO" || upper == "4")
    {
        set_log_level(LogLevel::Info);
    }
    else if (upper == "DEBUG" || upper == "5")
    {
        set_log_level(LogLevel::Debug);
    }
    else if (upper == "TRACE" || upper == "6")
    {
        set_log_level(LogLevel::Trace);
    }
    else if (upper == "ALL" || upper == "7")
    {
        set_log_level(LogLevel::Trace);
    }
}

void FRLogger::disable_logging()
{
    enabled_.store(false);
}

void FRLogger::enable_logging()
{
    enabled_.store(true);
}

void FRLogger::log(LogLevel level, std::string_view msg)
{
    if (!enabled_.load() || level < log_level_.load())
    {
        return;
    }

    LogEntry entry;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.level = level;
    entry.message = msg;

    // Console output
    std::printf("[%s] %s: %s\n",
        get_timestamp().c_str(),
        level_to_string(level).c_str(),
        msg.data());

    // Store entry
    std::lock_guard<std::mutex> lock(entries_mutex_);
    entries_.push_back(std::move(entry));
}

void FRLogger::trace(std::string_view msg)
{
    log(LogLevel::Trace, msg);
}

void FRLogger::debug(std::string_view msg)
{
    log(LogLevel::Debug, msg);
}

void FRLogger::info(std::string_view msg)
{
    log(LogLevel::Info, msg);
}

void FRLogger::warn(std::string_view msg)
{
    log(LogLevel::Warn, msg);
}

void FRLogger::error(std::string_view msg)
{
    log(LogLevel::Error, msg);
}

void FRLogger::trace_entry(std::string_view perf_id)
{
    if (!enabled_.load())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(perf_mutex_);
    int64_t hash = std::hash<std::string_view>{}(perf_id);
    perf_data_[hash] = std::chrono::steady_clock::now();
}

double FRLogger::trace_exit(std::string_view perf_id)
{
    if (!enabled_.load())
    {
        return 0.0;
    }

    int64_t hash = std::hash<std::string_view>{}(perf_id);

    std::chrono::steady_clock::time_point start_time;
    {
        std::lock_guard<std::mutex> lock(perf_mutex_);
        auto it = perf_data_.find(hash);
        if (it == perf_data_.end())
        {
            return 0.0;
        }
        start_time = it->second;
        perf_data_.erase(it);
    }

    auto elapsed = std::chrono::steady_clock::now() - start_time;
    double seconds = std::chrono::duration<double>(elapsed).count();

    std::string msg = "Method '" + std::string(perf_id) + "' was performed in " + format_duration(seconds) + ".";
    log(LogLevel::Trace, msg);

    return seconds;
}

bool FRLogger::is_enabled() const
{
    return enabled_.load();
}

LogLevel FRLogger::current_level() const
{
    return log_level_.load();
}

const std::vector<LogEntry>& FRLogger::entries() const
{
    return entries_;
}

void FRLogger::clear_entries()
{
    std::lock_guard<std::mutex> lock(entries_mutex_);
    entries_.clear();
}

std::string FRLogger::get_timestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string FRLogger::level_to_string(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::string FRLogger::format_duration(double total_seconds)
{
    double seconds = total_seconds;
    double minutes = seconds / 60.0;
    double hours = minutes / 60.0;

    hours = std::floor(hours);
    minutes = std::floor(std::fmod(minutes, 60.0));
    seconds = std::fmod(seconds, 60.0);

    std::string result;
    if (hours > 0)
    {
        result += std::to_string(static_cast<int>(hours));
        result += (hours == 1.0) ? " hour " : " hours ";
    }
    if (minutes > 0)
    {
        result += std::to_string(static_cast<int>(minutes));
        result += (minutes == 1.0) ? " minute " : " minutes ";
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << seconds;
    result += ss.str() + " seconds";
    return result;
}

std::string FRLogger::format_score(float score, int incomplete, int violations)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << score;
    std::string result = ss.str();

    if (incomplete > 0 || violations > 0)
    {
        result += " (";

        if (incomplete > 0)
        {
            result += std::to_string(incomplete) + " unrouted";
        }

        if (incomplete > 0 && violations > 0)
        {
            result += " and ";
        }

        if (violations > 0)
        {
            result += std::to_string(violations);
            result += (violations == 1) ? " violation" : " violations";
        }

        result += ")";
    }

    return result;
}

} // namespace logger
} // namespace freerouting