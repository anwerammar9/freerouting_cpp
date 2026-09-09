#pragma once

#include <string>
#include <chrono>
#include <ctime>

namespace freerouting {
namespace settings {

class RuntimeEnvironment
{
public:
    RuntimeEnvironment();

    std::string freerouting_version;
    std::chrono::time_point<std::chrono::system_clock> app_started_at;
    std::string command_line_arguments;
    std::string architecture;
    std::string java;
    std::string system_language;
    int cpu_cores = 0;
    int ram = 0;
    std::string host = "N/A";

    std::string to_string() const;
};

} // namespace settings
} // namespace freerouting