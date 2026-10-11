#include "Freerouting.h"
#include "management/SessionManager.h"
#include "management/HeadlessBoardManager.h"
#include "board/RoutingBoard.h"
#include "autoroute/BatchAutorouter.h"
#include "designforms/specctra/BoardCountsDump.h"
#include "designforms/specctra/DsnReader.h"
#include "designforms/specctra/SessionWriter.h"
#include "settings/sources/DefaultSettings.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <climits>
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

    if (!args.empty() && args[0] == "--dump-board-counts") {
        return dump_board_counts(args);
    }
    if (!args.empty() && args[0] == "--write-ses") {
        return write_ses_without_routing(args);
    }

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

    // Copy settings to the routing job. Java: routingJob.routerSettings = settingsMerger.merge(),
    // where the CLI values (GlobalSettings.routerSettings) are applied over DefaultSettings.
    // Interim until SettingsMerger lands (T303): only the Default < CLI part of the merge.
    routingJob.router_settings = settings::sources::DefaultSettings::create_settings().clone();
    routingJob.router_settings.apply_new_values_from(globalSettings.get_router_settings());
    routingJob.router_settings.validate();
    routingJob.drc_settings = globalSettings.get_drc_settings();
    routingJob.set_state(core::RoutingJobState::READY_TO_START);

    // Load the board from the input file (Java: HeadlessBoardManager.loadFromSpecctraDsn).
    management::HeadlessBoardManager board_manager(&routingJob);
    if (!load_board(board_manager, routingJob, globalSettings.initial_input_file)) {
        logger::FRLogger::instance().error("Failed to load board for routing");
        return false;
    }

    // Java: DsnFileSettings supplies the layers array from the DSN layer count, and
    // applyBoardSpecificOptimizations (T307) later tunes the per-layer costs.
    routingJob.router_settings.set_layer_count(routingJob.board->get_layer_count());

    routingJob.set_state(core::RoutingJobState::RUNNING);

    // Run the autorouter over the loaded board.
    autoroute::BatchAutorouter autorouter;
    autorouter.set_board(routingJob.board.get());
    autorouter.set_settings(routingJob.router_settings);

    // Java: autorouter.maxPasses == 0 means "no limit".
    const int configured_max_passes = routingJob.router_settings.get_max_passes();
    const int max_passes = configured_max_passes > 0 ? configured_max_passes : INT_MAX;
    autoroute::RouteResult route_result = autorouter.route_all(max_passes, nullptr);

    logger::FRLogger::instance().info("DEBUG board items after route: " + std::to_string(routingJob.board->get_item_count()));

    // Write the resulting routes into the output SES file. Java writes the session with the job
    // name, the input file name without extension.
    const std::string design_name = fs::path(globalSettings.initial_input_file).stem().string();
    std::ofstream ses_stream(globalSettings.initial_output_file, std::ios::binary);
    if (!ses_stream.is_open() || !board_manager.save_as_specctra_session_ses(ses_stream, design_name)) {
        logger::FRLogger::instance().error("Failed to write SES output file");
        return false;
    }

    routingJob.set_state(core::RoutingJobState::COMPLETED);

    logger::FRLogger::instance().info("Routing completed. Output file: " + globalSettings.initial_output_file);
    logger::FRLogger::instance().info(
        "  Routed: " + std::to_string(route_result.complete_count) +
        "%  Incomplete: " + std::to_string(route_result.incomplete_count) + "%");

    return true;
}

bool Freerouting::load_board(management::HeadlessBoardManager& p_board_manager, core::RoutingJob& p_job,
                             const std::string& p_input_file)
{
    std::ifstream dsn_stream(p_input_file, std::ios::binary);
    if (!dsn_stream.good()) {
        logger::FRLogger::instance().error("Couldn't open the input file '" + p_input_file + "'");
        return false;
    }
    p_board_manager.load_from_specctra_dsn(dsn_stream, fs::path(p_input_file).filename().string());
    p_job.board = p_board_manager.get_shared_routing_board();
    return p_job.board != nullptr;
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
    management::HeadlessBoardManager board_manager(&drcJob);
    if (!load_board(board_manager, drcJob, globalSettings.initial_input_file)) {
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

int Freerouting::dump_board_counts(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "Usage: freerouting --dump-board-counts <dsn> [out.json] [--omit-items]\n";
        return 2;
    }
    const std::string dsn_path = args[1];
    std::string out_path;
    bool omit_items = false;
    for (std::size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--omit-items") {
            omit_items = true;
        } else {
            out_path = args[i];
        }
    }

    std::ifstream dsn_stream(dsn_path, std::ios::binary);
    designforms::specctra::BoardReadResult result = dsn_stream.good()
        ? designforms::specctra::DsnReader::read_board(dsn_stream, fs::path(dsn_path).filename().string())
        : designforms::specctra::BoardReadResult(
              designforms::specctra::board_read_result::IoError{"cannot open " + dsn_path});
    const std::string json = designforms::specctra::BoardCountsDump::render(result, dsn_path, omit_items);

    if (!out_path.empty()) {
        std::ofstream out(out_path, std::ios::binary);
        out << json;
        return out.good() ? 0 : 1;
    }
    std::cout << "===JSON===\n" << json;
    return 0;
}

int Freerouting::write_ses_without_routing(const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        std::cerr << "Usage: freerouting --write-ses <dsn> <out.ses>\n";
        return 2;
    }
    std::ifstream dsn_stream(args[1], std::ios::binary);
    if (!dsn_stream.good()) {
        std::cerr << "Cannot open " << args[1] << "\n";
        return 1;
    }
    const std::string design_name = fs::path(args[1]).filename().string();
    designforms::specctra::BoardReadResult result = designforms::specctra::DsnReader::read_board(dsn_stream, design_name);
    auto* success = std::get_if<designforms::specctra::board_read_result::Success>(&result);
    if (success == nullptr || success->board == nullptr) {
        std::cerr << "The DSN file could not be loaded\n";
        return 1;
    }
    std::ofstream out(args[2], std::ios::binary);
    designforms::specctra::SessionWriter::write(*success->board, out, design_name);
    return out.good() ? 0 : 1;
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