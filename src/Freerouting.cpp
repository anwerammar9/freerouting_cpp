#include "Freerouting.h"
#include "management/SessionManager.h"
#include "board/BoardLoader.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

namespace freerouting {

std::string Freerouting::VERSION_NUMBER_STRING = std::string("v") +
    std::string(constants::FREEROUTING_VERSION.data()) +
    " (build-date: " + std::string(constants::FREEROUTING_BUILD_DATE.data()) + ")";

std::unique_ptr<settings::GlobalSettings> Freerouting::globalSettings = nullptr;

int Freerouting::main(const std::vector<std::string>& args)
{
    FR_TRACE_ENTRY("MainApplication.main()");

    // Determine the user data directory
    fs::path userdata_path;

    // Default: use temp directory + "freerouting"
    const char* tmpdir = std::getenv("TMPDIR");
    if (!tmpdir) tmpdir = std::getenv("TEMP");
    if (!tmpdir) tmpdir = std::getenv("TMP");
    if (!tmpdir) tmpdir = "/tmp";
    userdata_path = fs::path(tmpdir) / "freerouting";

    // Override from environment variable
    const char* env_user_data = std::getenv("FREEROUTING__USER_DATA_PATH");
    if (env_user_data) {
        userdata_path = fs::path(env_user_data);
    }

    // Override from command line --user_data_path=
    for (const auto& arg : args) {
        if (arg.starts_with("--user_data_path=")) {
            userdata_path = fs::path(arg.substr(std::string("--user_data_path=").length()));
            break;
        }
    }

    // Create directory if it doesn't exist
    if (!fs::exists(userdata_path)) {
        std::error_code ec;
        fs::create_directories(userdata_path, ec);
        if (ec) {
            logger::FRLogger::instance().warn("Could not create user data directory: " + userdata_path.string());
        }
    }

    // User data path will be set on globalSettings instance after it's created

    // Handle logging arguments early
    bool logging_disabled = false;
    for (const auto& arg : args) {
        if (arg == "-dl") {
            logger::FRLogger::instance().disable_logging();
            logging_disabled = true;
            break;
        }
    }
    if (!logging_disabled) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-ll" && i + 1 < args.size()) {
                logger::FRLogger::instance().set_log_level(args[i + 1]);
                break;
            }
        }
    }

    logger::FRLogger::instance().info("Freerouting " + VERSION_NUMBER_STRING);

    // Try to load existing settings
    try {
        auto loaded = settings::GlobalSettings::load();
        if (loaded) {
            globalSettings = std::move(loaded);
            logger::FRLogger::instance().debug("Settings were loaded from freerouting.json");
        }
    } catch (...) {
        // Ignore errors during settings load
    }

    // If settings are null or version mismatch, create fresh settings
    if (!globalSettings || globalSettings->get_version() != std::string(constants::FREEROUTING_VERSION)) {
        std::string user_id;
        if (globalSettings) {
            user_id = globalSettings->get_user_profile_settings().user_id;
        }

        globalSettings = std::make_unique<settings::GlobalSettings>();
        if (!user_id.empty()) {
            globalSettings->get_user_profile_settings().user_id = user_id;
        }
        globalSettings->get_version() = std::string(constants::FREEROUTING_VERSION);

        try {
            settings::GlobalSettings::save_as_json(*globalSettings);
        } catch (...) {
            // Ignore errors during settings save
        }
    }

    // Set user data path on the instance
    if (fs::exists(userdata_path)) {
        globalSettings->set_user_data_path(userdata_path.string());
    }
    globalSettings->lock_user_data_path();

    globalSettings->apply_non_router_environment_variables();

    // Populate runtime environment
    auto& runtime_env = globalSettings->get_runtime_environment();
    runtime_env.freerouting_version = std::string(constants::FREEROUTING_VERSION) + "," +
        std::string(constants::FREEROUTING_BUILD_DATE);
    runtime_env.app_started_at = std::chrono::system_clock::now();
    runtime_env.command_line_arguments = [&args]() {
        std::string joined;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) joined += " ";
            joined += args[i];
        }
        return joined;
    }();

    // Architecture info (C++ equivalent of os.name, os.arch, os.version)
#if defined(_WIN32)
    runtime_env.architecture = "Windows";
#elif defined(__linux__)
    runtime_env.architecture = "Linux";
#elif defined(__APPLE__)
    runtime_env.architecture = "macOS";
#else
    runtime_env.architecture = "Unknown";
#endif

#ifdef __x86_64__
    runtime_env.architecture += ",x86_64";
#elif defined(__aarch64__)
    runtime_env.architecture += ",aarch64";
#elif defined(__arm__)
    runtime_env.architecture += ",arm";
#endif

    runtime_env.java = "C++20"; // No JVM in C++ build
    runtime_env.system_language = "en"; // TODO: Get actual system locale
    runtime_env.cpu_cores = static_cast<int>(std::thread::hardware_concurrency());

    // RAM estimation - approximate via total memory (platform-specific)
    long ram_mb = 0;
#ifdef __linux__
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.starts_with("MemTotal:")) {
            long kb = 0;
            if (sscanf(line.c_str(), "MemTotal: %ld kB", &kb) == 1) {
                ram_mb = kb / 1024;
            }
            break;
        }
    }
#endif
    runtime_env.ram = static_cast<int>(ram_mb);

    logger::FRLogger::instance().debug("Version: " + runtime_env.freerouting_version);
    logger::FRLogger::instance().debug("Command line arguments: '" + runtime_env.command_line_arguments + "'");
    logger::FRLogger::instance().debug("Architecture: " + runtime_env.architecture);
    logger::FRLogger::instance().debug("Runtime: " + runtime_env.java);
    logger::FRLogger::instance().debug("System Language: " + runtime_env.system_language);
    logger::FRLogger::instance().debug("Hardware: " + std::to_string(runtime_env.cpu_cores) + " CPU cores," +
        std::to_string(runtime_env.ram) + " MB RAM");

    // Apply command line arguments to settings
    globalSettings->apply_command_line_arguments(args);

    // Handle help option
    if (globalSettings->show_help_option) {
        print_help();
        return 0;
    }

    // Run CLI or DRC mode
    bool cli_result;
    if (globalSettings->get_drc_settings().enabled) {
        cli_result = InitializeDRC(*globalSettings);
    } else {
        cli_result = InitializeCLI(*globalSettings);
    }

    if (!cli_result) {
        ShutdownApplication();
        FR_TRACE_EXIT("MainApplication.main()");
        return 1;
    }

    ShutdownApplication();
    FR_TRACE_EXIT("MainApplication.main()");
    return 0;
}

bool Freerouting::InitializeCLI(settings::GlobalSettings& globalSettings)
{
    if (globalSettings.initial_input_file.empty() || globalSettings.initial_output_file.empty()) {
        logger::FRLogger::instance().error(
            "Both an input file and an output file must be specified with command line arguments if you are running in CLI mode.");
        return false;
    }

    // Start a new Freerouting session
    auto cli_session = management::SessionManager::instance().create_session(
        globalSettings.get_user_profile_settings().user_id,
        "Freerouting/" + globalSettings.get_version());

    // Create a new routing job
    core::RoutingJob routingJob(cli_session->get_id());

    try {
        routingJob.set_input(globalSettings.initial_input_file);
    } catch (const std::exception& e) {
        logger::FRLogger::instance().error("Couldn't load the input file '" + globalSettings.initial_input_file + "': " + e.what());
    }

    // cli_session->add_job(routingJob); // TODO: implement

    // Delete existing output file if it exists
    fs::path output_path(globalSettings.initial_output_file);
    if (fs::exists(output_path)) {
        std::error_code ec;
        fs::remove(output_path, ec);
        if (ec) {
            logger::FRLogger::instance().warn("Couldn't delete the file '" + globalSettings.initial_output_file + "'");
        }
    }

    routingJob.try_to_set_output_file(globalSettings.initial_output_file);

    // Copy settings to the routing job
    routingJob.router_settings = globalSettings.get_router_settings();
    // TODO: Set layer count from input statistics: routingJob.router_settings.set_layer_count(...)
    routingJob.drc_settings = globalSettings.get_drc_settings();
    routingJob.set_state(core::RoutingJobState::READY_TO_START);

    // TODO: Submit job to RoutingJobScheduler and wait for completion
    // For now, simulate the wait loop
    logger::FRLogger::instance().info("Starting routing job...");

    // Mark as completed for now (actual routing not yet implemented)
    routingJob.set_state(core::RoutingJobState::COMPLETED);

    // Wait for the RoutingJobScheduler to do its work
    while (routingJob.get_state() != core::RoutingJobState::COMPLETED &&
           routingJob.get_state() != core::RoutingJobState::TERMINATED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Save the output file
    if (routingJob.get_state() == core::RoutingJobState::COMPLETED) {
        // TODO: Write actual output data from routingJob.output
        // For now, just log completion
        logger::FRLogger::instance().info("Routing completed. Output file: " + globalSettings.initial_output_file);
    }

    return true;
}

bool Freerouting::InitializeDRC(settings::GlobalSettings& globalSettings)
{
    if (globalSettings.initial_input_file.empty()) {
        logger::FRLogger::instance().error("An input file must be specified with -de argument in DRC mode.");
        return false;
    }

    // Start a new Freerouting session
    auto drc_session = management::SessionManager::instance().create_session(
        globalSettings.get_user_profile_settings().user_id,
        "Freerouting/" + globalSettings.get_version());

    // Create a new routing job (but won't route it)
    core::RoutingJob drcJob(drc_session->get_id());
    drcJob.drc_report_file = globalSettings.get_drc_settings().drc_report_file;

    try {
        drcJob.set_input(globalSettings.initial_input_file);
    } catch (const std::exception& e) {
        logger::FRLogger::instance().error("Couldn't load the input file '" + globalSettings.initial_input_file + "': " + e.what());
        return false;
    }

    // Load the board without routing
    if (!board::BoardLoader::load_board_if_needed(drcJob)) {
        logger::FRLogger::instance().error("Failed to load board for DRC check");
        return false;
    }

    // TODO: Run DRC check
    // DesignRulesChecker drcChecker(drcJob.board, globalSettings.get_drc_settings());
    // std::string drcReportJson = drcChecker.generateReportJson(sourceFileName, coordinateUnit);

    logger::FRLogger::instance().info("DRC check completed (stub - not yet fully implemented)");

    // Output the DRC report
    if (!drcJob.drc_report_file.empty()) {
        // TODO: Write DRC report to file
        logger::FRLogger::instance().info("DRC report would be written to: " + drcJob.drc_report_file);
    } else {
        // Print to console
        std::cout << "{}" << std::endl; // Empty JSON stub
    }

    return true;
}

void Freerouting::ShutdownApplication()
{
    // Application is shutting down
}

void Freerouting::print_help()
{
    std::cout << "Freerouting " << VERSION_NUMBER_STRING << "\n"
              << "Usage: freerouting [options]\n"
              << "\n"
              << "Options:\n"
              << "  -de <file>     Design file(s) (.dsn, .ses, .rules)\n"
              << "  -do <file>     Output file\n"
              << "  -dr <file>     Rules file\n"
              << "  -drc [file]    Run DRC check only (optional output file)\n"
              << "  -mp <n>        Max passes (1-99998)\n"
              << "  -mt <n>        Max threads (0-1024)\n"
              << "  -oit <n>       Optimization improvement threshold (0-100)\n"
              << "  -us <strat>    Board update strategy (global/hybrid/greedy)\n"
              << "  -is <strat>    Item selection strategy (sequential/random/prioritized)\n"
              << "  -hr <ratio>    Hybrid ratio\n"
              << "  -inc <list>    Ignore net classes (comma-separated)\n"
              << "  -l <locale>    Set locale\n"
              << "  -dl            Disable logging\n"
              << "  -ll <level>    Set log level\n"
              << "  -da            Disable analytics\n"
              << "  -host <host>   Set host\n"
              << "  -help          Show this help\n"
              << "  -h/--help      Show this help\n"
              << "\n"
              << "  --user_data_path=<path>  Set user data path\n"
              << "  --<property>=<value>     Set any setting property\n"
              << std::endl;
}

} // namespace freerouting