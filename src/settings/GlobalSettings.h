#pragma once

#include <memory>
#include <string>
#include <vector>

#include "settings/RouterSettings.h"
#include "settings/RuntimeEnvironment.h"
#include "settings/ProfileSettings.h"

namespace freerouting {
namespace settings {

class GlobalSettings
{
public:
    GlobalSettings();

    RuntimeEnvironment& get_runtime_environment() { return runtime_environment_; }
    RouterSettings& get_router_settings() { return router_settings_; }
    DesignRulesCheckerSettings& get_drc_settings() { return drc_settings_; }
    UserProfileSettings& get_user_profile_settings() { return user_profile_settings_; }
    FeatureFlagsSettings& get_feature_flags() { return feature_flags_; }
    UsageAndDiagnosticDataSettings& get_usage_and_diagnostic_data() { return usage_and_diagnostic_data_; }

    std::string& get_version() { return version_; }
    const std::vector<std::string>& get_supported_languages() const { return supported_languages_; }

    std::string get_user_data_path() const;
    void set_user_data_path(const std::string& p_path);
    void lock_user_data_path();

    bool save_to_file(const std::string& p_path) const;
    bool load_from_file(const std::string& p_path);

    void apply_command_line_arguments(const std::vector<std::string>& args);
    void apply_non_router_environment_variables();

    static std::unique_ptr<GlobalSettings> load();
    static bool save_as_json(const GlobalSettings& settings);

    std::string initial_input_file;
    std::string initial_output_file;
    std::string initial_rules_file;
    std::string design_session_filename;

    bool show_help_option = false;

    std::string to_string() const;

private:
    RuntimeEnvironment runtime_environment_;
    RouterSettings router_settings_;
    DesignRulesCheckerSettings drc_settings_;
    UserProfileSettings user_profile_settings_;
    FeatureFlagsSettings feature_flags_;
    UsageAndDiagnosticDataSettings usage_and_diagnostic_data_;
    std::string version_;
    std::vector<std::string> supported_languages_;
    std::string user_data_path_;
    bool user_data_path_locked_ = false;
};

} // namespace settings
} // namespace freerouting