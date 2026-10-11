#pragma once

#include <string>
#include <memory>
#include <vector>

#include "constants/Constants.h"
#include "logger/FRLogger.h"
#include "core/RoutingJob.h"
#include "settings/GlobalSettings.h"

namespace freerouting::management {
class HeadlessBoardManager;
} // namespace freerouting::management

namespace freerouting {

class Freerouting
{
public:
    static constexpr const char* WEB_URL = "https://www.freerouting.app";
    static std::string VERSION_NUMBER_STRING;

    static std::unique_ptr<settings::GlobalSettings> globalSettings;

    static int main(const std::vector<std::string>& args);

private:
    static bool InitializeCLI(settings::GlobalSettings& globalSettings);
    static bool InitializeDRC(settings::GlobalSettings& globalSettings);
    static void ShutdownApplication();
    static void print_help();

    /** Loads the DSN p_input_file through p_board_manager and shares the board with p_job. */
    static bool load_board(management::HeadlessBoardManager& p_board_manager, core::RoutingJob& p_job,
                           const std::string& p_input_file);

    /**
     * `--dump-board-counts <dsn> [out.json] [--omit-items]`: loads the DSN with DsnReader and prints
     * the board-count JSON of the Java oracle harness (tools/java_ref/BoardCounts.java) to out.json,
     * or to stdout after a `===JSON===` marker line. Returns the process exit code.
     */
    static int dump_board_counts(const std::vector<std::string>& args);

    /**
     * `--write-ses <dsn> <out.ses>`: loads the DSN with DsnReader and writes its session file without
     * routing (the C++ counterpart of DsnReader.readBoard + SesWriter.write). Returns the exit code.
     */
    static int write_ses_without_routing(const std::vector<std::string>& args);
};

} // namespace freerouting