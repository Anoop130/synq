#include "config_utils.h"
#include <fstream>
#include <string>

int readInterval(const char* filepath) {
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("interval=", 0) == 0) {
            return std::stoi(line.substr(9));
        }
    }
    return 5;
}