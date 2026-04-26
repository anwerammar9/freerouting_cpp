#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>

#include "logger/FRLogger.h"
#include "designforms/specctra/Scanner.h"
#include "designforms/specctra/SessionWriter.h"
#include "board/BoardBuilder.h"
#include "board/BasicBoard.h"
#include "board/RoutingBoard.h"
#include "autoroute/BatchAutorouter.h"
#include "settings/RouterSettings.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"

namespace fs = std::filesystem;

namespace freerouting {

class Freerouting
{
public:
    static const std::string WEB_URL;
    static const std::string VERSION_NUMBER_STRING;

    static int main(const std::vector<std::string>& args);
};

const std::string Freerouting::WEB_URL = "https://www.freerouting.app";
const std::string Freerouting::VERSION_NUMBER_STRING = "v2.1.0";

void print_help()
{
    std::cout << "Freerouting - PCB Autorouter\n\n";
    std::cout << "Usage: freerouting [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -de <file>    Input DSN file\n";
    std::cout << "  -do <file>    Output SES file\n";
    std::cout << "  -dl           Disable logging\n";
    std::cout << "  -ll <level>   Set log level (trace, debug, info, warn, error, off)\n";
    std::cout << "  -help         Show this help\n";
    std::cout << "  -inc <nets>   Comma-separated list of net classes to ignore\n";
    std::cout << "  -passes <n>   Maximum number of routing passes (default: 9999)\n";
    std::cout << "\nExample:\n";
    std::cout << "  freerouting -de input.dsn -do output.ses\n";
}

int Freerouting::main(const std::vector<std::string>& args)
{
    auto& logger = logger::FRLogger::instance();

    std::string input_file;
    std::string output_file;
    int max_passes = 9999;

    for (size_t i = 0; i < args.size(); ++i)
    {
        if (args[i] == "-de" && i + 1 < args.size())
        {
            input_file = args[++i];
        }
        else if (args[i] == "-do" && i + 1 < args.size())
        {
            output_file = args[++i];
        }
        else if (args[i] == "-dl")
        {
            logger.disable_logging();
        }
        else if (args[i] == "-ll" && i + 1 < args.size())
        {
            logger.set_log_level(args[++i]);
        }
        else if (args[i] == "-passes" && i + 1 < args.size())
        {
            max_passes = std::stoi(args[++i]);
        }
        else if (args[i] == "-help" || args[i] == "--help")
        {
            print_help();
            return 0;
        }
    }

    logger.info("Freerouting " + VERSION_NUMBER_STRING);

    if (input_file.empty() || output_file.empty())
    {
        logger.error("Both input file (-de) and output file (-do) must be specified");
        print_help();
        return 1;
    }

    if (!fs::exists(input_file))
    {
        logger.error("Input file does not exist: " + input_file);
        return 1;
    }

    logger.info("Input file: " + input_file);
    logger.info("Output file: " + output_file);

    auto parse_result = freerouting::designforms::specctra::read_dsn(input_file);
    if (!parse_result.success)
    {
        logger.error("Failed to parse DSN file: " + parse_result.error_message);
        return 1;
    }

    logger.info("Parsed DSN file:");
    logger.info("  Board name: " + parse_result.board_name);
    logger.info("  Board size: " + std::to_string(parse_result.board_width) + " x " + std::to_string(parse_result.board_height));
    logger.info("  Layers: " + std::to_string(parse_result.layers.size()));
    logger.info("  Vias: " + std::to_string(parse_result.vias.size()));
    logger.info("  Nets: " + std::to_string(parse_result.nets.size()));
    logger.info("  Components: " + std::to_string(parse_result.components.size()));

    auto board = freerouting::board::BoardBuilder::build(parse_result);
    if (!board)
    {
        logger.error("Failed to build board from DSN file");
        return 1;
    }

    logger.info("Board created successfully");
    logger.info("  Layer count: " + std::to_string(board->get_layer_count()));

    auto routing_board = dynamic_cast<freerouting::board::RoutingBoard*>(board.get());
    if (!routing_board)
    {
        logger.info("RoutingBoard not available, skipping routing");
    }
    else
    {
        logger.info("Starting autorouting...");
        logger.info("  Max passes: " + std::to_string(max_passes));

        datastructures::StoppableImpl stoppable;
        autoroute::BatchAutorouter autorouter;
        autorouter.set_board(routing_board);

        settings::RouterSettings router_settings;
        router_settings.set_max_passes(max_passes);
        autorouter.set_settings(router_settings);

        auto route_result = autorouter.route_all(max_passes, &stoppable);

        if (route_result.success)
        {
            logger.info("Autorouting completed");
            logger.info("  Completion: " + std::to_string(route_result.complete_count) + "%");
            if (route_result.incomplete_count > 0)
            {
                logger.warn("  Incomplete connections: " + std::to_string(route_result.incomplete_count));
            }
        }
        else
        {
            logger.error("Autorouting failed");
        }

        logger.info("Writing session file...");
        designforms::specctra::SessionWriter session_writer(output_file);
        if (session_writer.write_session(*routing_board, parse_result.board_name))
        {
            logger.info("Session file written successfully: " + output_file);
        }
        else
        {
            logger.error("Failed to write session file");
        }
    }

    return 0;
}

} // namespace freerouting

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }

    return freerouting::Freerouting::main(args);
}