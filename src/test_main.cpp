#include <iostream>
#include "core/FileFormat.h"
#include "logger/FRLogger.h"
#include "Freerouting.h"

int main()
{
    freerouting::logger::FRLogger::instance().info("FreeRouting starting...");
    
    std::cout << "Testing FileFormat..." << std::endl;
    
    auto format = freerouting::core::FileFormat::DSN;
    
    if (format == freerouting::core::FileFormat::DSN)
    {
        freerouting::logger::FRLogger::instance().info("FileFormat test passed!");
        std::cout << "FileFormat test passed!" << std::endl;
        return 0;
    }
    
    std::cout << "FileFormat test FAILED!" << std::endl;
    return 1;
}