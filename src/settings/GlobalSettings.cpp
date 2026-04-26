#include "settings/GlobalSettings.h"
#include "logger/FRLogger.h"
#include "constants/Constants.h"

#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace freerouting {
namespace settings {

namespace {

std::string generate_uuid()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    auto hex8 = [&](uint32_t v) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(8) << v;
        return oss.str();
    };
    auto hex4 = [&](uint32_t v) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(4) << (v & 0xFFFF);
        return oss.str();
    };

    uint32_t time_low = dist(gen);
    uint32_t time_mid = dist(gen) & 0xFFFF;
    uint32_t time_hi = (dist(gen) & 0x0FFF) | 0x4000; // Version 4
    uint32_t clock_seq = (dist(gen) & 0x3FFF) | 0x8000; // Variant 1
    uint32_t node1 = dist(gen);
    uint32_t node2 = dist(gen);

    std::ostringstream oss;
    oss << hex8(time_low) << "-"
        << hex4(time_mid) << "-"
        << hex4(time_hi) << "-"
        << hex4(clock_seq) << "-"
        << hex8(node1) << hex8(node2).substr(0, 4);
    return oss.str();
}

} // anonymous namespace

GlobalSettings::GlobalSettings()
{
    version_ = std::string(constants::FREEROUTING_VERSION);
    user_profile_settings_.user_id = generate_uuid();
}

std::string GlobalSettings::get_user_data_path() const
{
    return user_data_path_;
}

void GlobalSettings::set_user_data_path(const std::string& p_path)
{
    if (!user_data_path_locked_) {
        user_data_path_ = p_path;
    }
}

void GlobalSettings::lock_user_data_path()
{
    user_data_path_locked_ = true;
}

bool GlobalSettings::save_to_file(const std::string& p_path) const
{
    // TODO: Implement JSON serialization
    return false;
}

bool GlobalSettings::load_from_file(const std::string& p_path)
{
    // TODO: Implement JSON deserialization
    return false;
}

std::unique_ptr<GlobalSettings> GlobalSettings::load()
{
    // TODO: Load from freerouting.json in user data path
    return nullptr;
}

bool GlobalSettings::save_as_json(const GlobalSettings& /*settings*/)
{
    // TODO: Implement JSON save
    return false;
}

void GlobalSettings::apply_non_router_environment_variables()
{
    // Read environment variables for non-router settings
    const char* disable_analytics = std::getenv("FREEROUTING_DISABLE_ANALYTICS");
    if (disable_analytics && std::string(disable_analytics) == "1") {
        usage_and_diagnostic_data_.allow_telemetry = false;
    }
}

void GlobalSettings::apply_command_line_arguments(const std::vector<std::string>& args)
{
    for (size_t i = 0; i < args.size(); ++i) {
        try {
            const std::string& arg = args[i];

            if (arg.starts_with("--")) {
                // --property=value format
                auto eq_pos = arg.find('=');
                if (eq_pos != std::string::npos && arg.substr(2, eq_pos - 2) != "user_data_path") {
                    std::string property_name = arg.substr(2, eq_pos - 2);
                    std::string value = arg.substr(eq_pos + 1);
                    // TODO: Implement setValue() for generic property setting
                    logger::FRLogger::instance().debug("CLI setting: " + property_name + "=" + value);
                }
            } else if (arg.starts_with("-de")) {
                // Design file(s) - can be DSN, SES, and/or RULES files
                bool has_dsn = false;
                bool has_ses = false;
                bool has_rules = false;

                size_t j = i + 1;
                while (j < args.size() && !args[j].starts_with("-")) {
                    std::string file = args[j];
                    std::string lower_file = file;
                    std::transform(lower_file.begin(), lower_file.end(), lower_file.begin(),
                                   [](unsigned char c) { return std::tolower(c); });

                    if (lower_file.ends_with(".dsn")) {
                        if (has_dsn) {
                            logger::FRLogger::instance().warn("Multiple DSN files provided in -de argument. Only the last one will be used.");
                        }
                        initial_input_file = file;
                        has_dsn = true;
                    } else if (lower_file.ends_with(".ses")) {
                        if (has_ses) {
                            logger::FRLogger::instance().warn("Multiple SES files provided in -de argument. Only the last one will be used.");
                        }
                        design_session_filename = file;
                        has_ses = true;
                    } else if (lower_file.ends_with(".rules")) {
                        if (has_rules) {
                            logger::FRLogger::instance().warn("Multiple RULES files provided in -de argument. Only the last one will be used.");
                        }
                        initial_rules_file = file;
                        has_rules = true;
                    } else {
                        logger::FRLogger::instance().warn("Unknown file type in -de argument: " + file + ". Expected .dsn, .ses, or .rules");
                    }
                    j++;
                }
                i = j - 1;

            } else if (arg.starts_with("-di")) {
                // Design directory - ignored in headless mode
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    i++;
                }
            } else if (arg.starts_with("-do")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    initial_output_file = args[i + 1];
                    i++;
                }
            } else if (arg.starts_with("-drc")) {
                // DRC-only mode
                router_settings_.set_enabled(false);
                drc_settings_.enabled = true;
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    drc_settings_.drc_report_file = args[i + 1];
                    i++;
                }
            } else if (arg.starts_with("-dr")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    initial_rules_file = args[i + 1];
                    i++;
                }
            } else if (arg.starts_with("-mp")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    int max_passes = std::stoi(args[i + 1]);
                    if (max_passes < 1) max_passes = 1;
                    if (max_passes > 99998) max_passes = 99998;
                    router_settings_.set_max_passes(max_passes);
                    i++;
                }
            } else if (arg.starts_with("-mt")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    int max_threads = std::stoi(args[i + 1]);
                    if (max_threads <= 0) max_threads = 0;
                    if (max_threads > 1024) max_threads = 1024;
                    router_settings_.set_max_threads(max_threads);
                    i++;
                }
            } else if (arg.starts_with("-oit")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    float threshold = std::stof(args[i + 1]) / 100.0f;
                    if (threshold <= 0.0f) threshold = 0.0f;
                    // TODO: set optimizer improvement threshold
                    i++;
                }
            } else if (arg.starts_with("-us")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    // TODO: set board update strategy (global/hybrid/greedy)
                    i++;
                }
            } else if (arg.starts_with("-is")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    // TODO: set item selection strategy (sequential/random/prioritized)
                    i++;
                }
            } else if (arg.starts_with("-hr")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    // TODO: set hybrid ratio
                    i++;
                }
            } else if (arg == "-l") {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    // TODO: set locale
                    i++;
                }
            } else if (arg.starts_with("-dl")) {
                feature_flags_.enable_batch_autorouter = false;
                // Also disable logging
                logger::FRLogger::instance().disable_logging();
            } else if (arg.starts_with("-da")) {
                usage_and_diagnostic_data_.allow_telemetry = false;
            } else if (arg.starts_with("-host")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    runtime_environment_.host = args[i + 1];
                    i++;
                }
            } else if (arg.starts_with("-help")) {
                show_help_option = true;
            } else if (arg.starts_with("-inc")) {
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    // TODO: set ignore net classes
                    i++;
                }
            } else if (arg.starts_with("-dct")) {
                // Dialog confirmation timeout - ignored in headless mode
                if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                    i++;
                }
            } else if (arg == "-ll") {
                if (i + 1 < args.size()) {
                    logger::FRLogger::instance().set_log_level(args[i + 1]);
                    i++;
                }
            } else {
                if (arg != "-h" && arg != "--help") {
                    logger::FRLogger::instance().warn("Unknown command line argument: " + arg);
                }
            }
        } catch (const std::exception& e) {
            logger::FRLogger::instance().error("There was a problem parsing the '" + args[i] + "' parameter: " + std::string(e.what()));
        }
    }
}

} // namespace settings
} // namespace freerouting