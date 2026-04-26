#pragma once

#include <memory>
#include <string>

#include "settings/RouterSettings.h"

namespace freerouting {
namespace settings {

class SettingsSource
{
public:
    virtual ~SettingsSource() = default;

    virtual std::shared_ptr<RouterSettings> get_settings() = 0;
    virtual std::string get_source_name() const = 0;
    virtual int get_priority() const = 0;
};

class DefaultSettings : public SettingsSource
{
public:
    DefaultSettings();

    std::shared_ptr<RouterSettings> get_settings() override;
    std::string get_source_name() const override { return "Default Settings"; }
    int get_priority() const override { return 0; }
};

class CliSettings : public SettingsSource
{
public:
    CliSettings(int argc, char** argv);

    std::shared_ptr<RouterSettings> get_settings() override;
    std::string get_source_name() const override { return "CLI Arguments"; }
    int get_priority() const override { return 60; }

private:
    int argc_ = 0;
    char** argv_ = nullptr;
};

} // namespace settings
} // namespace freerouting