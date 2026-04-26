#pragma once

#include <string>

namespace freerouting {
namespace settings {

class UserProfileSettings
{
public:
    UserProfileSettings() = default;

    std::string user_id;
    std::string user_email;
    bool is_telemetry_allowed = true;
    bool is_contact_allowed = true;

    std::string to_string() const;
};

class DesignRulesCheckerSettings
{
public:
    DesignRulesCheckerSettings() = default;

    bool enabled = true;
    bool check_clearances = true;
    bool check_min_trace_width = true;

    std::string to_string() const;
};

class FeatureFlagsSettings
{
public:
    FeatureFlagsSettings() = default;

    bool enable_batch_autorouter = true;
    bool enable_interactive_routing = true;

    std::string to_string() const;
};

class StatisticsSettings
{
public:
    StatisticsSettings() = default;

    bool track_routing_time = true;
    bool track_memory_usage = true;

    std::string to_string() const;
};

class UsageAndDiagnosticDataSettings
{
public:
    UsageAndDiagnosticDataSettings() = default;

    bool allow_telemetry = true;
    bool allow_error_reports = true;

    std::string to_string() const;
};

} // namespace settings
} // namespace freerouting