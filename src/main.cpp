#include <iostream>
#include <vector>
#include <string>
#include "Freerouting.h"

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    return freerouting::Freerouting::main(args);
}