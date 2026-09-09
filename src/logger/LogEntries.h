#pragma once

#include <vector>
#include <memory>

#include "logger/LogEntry.h"

namespace freerouting {
namespace logger {

class LogEntries
{
public:
    LogEntries() = default;

    void add(const LogEntry& p_entry);
    void add(LogEntry&& p_entry);

    int count() const { return static_cast<int>(entries_.size()); }

    const LogEntry& get(int p_index) const;
    const LogEntry& get_last() const;

    void clear() { entries_.clear(); }

    bool empty() const { return entries_.empty(); }

    int get_warning_count() const;
    int get_error_count() const;

private:
    std::vector<LogEntry> entries_;
};

} // namespace logger
} // namespace freerouting