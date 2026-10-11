#pragma once

#include <memory>
#include <string>

#include "settings/RouterSettings.h"

namespace freerouting {
namespace settings {

/**
 * Base interface for all settings sources (Java settings/SettingsSource.java). Each source
 * provides router settings from a specific origin; only the settings a source explicitly provides
 * are non-null. Lower priority numbers are applied first: 0 = defaults, 10 = JSON file,
 * 20 = DSN file, 30 = SES file, 40 = RULES file, 55 = environment variables, 60 = CLI.
 * The implementations live in settings/sources/ (DefaultSettings; the others follow in T306).
 */
class SettingsSource
{
public:
    virtual ~SettingsSource() = default;

    virtual std::shared_ptr<RouterSettings> get_settings() = 0;
    virtual std::string get_source_name() const = 0;
    virtual int get_priority() const = 0;
};

} // namespace settings
} // namespace freerouting
