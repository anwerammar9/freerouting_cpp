#pragma once

#include <string>
#include <memory>
#include <vector>

#include "constants/Constants.h"
#include "logger/FRLogger.h"
#include "core/RoutingJob.h"
#include "settings/GlobalSettings.h"

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
};

} // namespace freerouting