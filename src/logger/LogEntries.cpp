#include "logger/LogEntries.h"

#include <stdexcept>

namespace freerouting {
namespace logger {

void LogEntries::add(const LogEntry& p_entry)
{
    entries_.push_back(p_entry);
}

void LogEntries::add(LogEntry&& p_entry)
{
    entries_.push_back(std::move(p_entry));
}

const LogEntry& LogEntries::get(int p_index) const
{
    if (p_index < 0 || p_index >= static_cast<int>(entries_.size()))
    {
        throw std::out_of_range("LogEntries index out of range");
    }
    return entries_[p_index];
}

const LogEntry& LogEntries::get_last() const
{
    if (entries_.empty())
    {
        throw std::out_of_range("LogEntries is empty");
    }
    return entries_.back();
}

int LogEntries::get_warning_count() const
{
    int count = 0;
    for (const auto& entry : entries_)
    {
        if (entry.get_type() == LogEntryType::Warning)
        {
            ++count;
        }
    }
    return count;
}

int LogEntries::get_error_count() const
{
    int count = 0;
    for (const auto& entry : entries_)
    {
        if (entry.get_type() == LogEntryType::Error)
        {
            ++count;
        }
    }
    return count;
}

} // namespace logger
} // namespace freerouting