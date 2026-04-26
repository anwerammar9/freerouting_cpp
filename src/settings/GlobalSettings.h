#pragma once

#include <memory>
#include <string>
#include <vector>

#include "settings/RouterSettings.h"
#include "settings/RuntimeEnvironment.h"

namespace freerouting {
namespace settings {

class GlobalSettings
{
public:
    GlobalSettings();

    RuntimeEnvironment& get_runtime_environment() { return runtime_environment_; }
    RouterSettings& get_router_settings() { return router_settings_; }

    std::string& get_version() { return version_; }
    const std::vector<std::string>& get_supported_languages() const { return supported_languages_; }

    std::string get_user_data_path() const;
    void set_user_data_path(const std::string& p_path);

    bool save_to_file(const std::string& p_path) const;
    bool load_from_file(const std::string& p_path);

    std::string initial_input_file;
    std::string initial_output_file;
    std::string initial_rules_file;
    std::string design_session_filename;

    std::string to_string() const;

private:
    RuntimeEnvironment runtime_environment_;
    RouterSettings router_settings_;
    std::string version_;
    std::vector<std::string> supported_languages_;
    std::string user_data_path_;
};

} // namespace settings
} // namespace freerouting