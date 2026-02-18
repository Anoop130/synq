#include "config_utils.h"
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

std::string getConfigDir() {
    const char* home = getenv("HOME");
    if (!home) home = ".";
    std::string configDir = std::string(home) + "/.config/synq";
    mkdir(configDir.c_str(), 0755);
    return configDir;
}

std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

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

std::string getDeviceId() {
    std::string configDir = getConfigDir();
    std::string deviceFile = configDir + "/device.conf";
    
    std::ifstream file(deviceFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("device_id=", 0) == 0) {
                return line.substr(10);
            }
        }
    }
    
    // Generate new UUID if configuration not found
    std::string newId = generateUUID();
    saveDeviceId(newId);
    return newId;
}

void saveDeviceId(const std::string& deviceId) {
    std::string configDir = getConfigDir();
    std::string deviceFile = configDir + "/device.conf";
    
    std::ofstream file(deviceFile);
    if (file.is_open()) {
        file << "device_id=" << deviceId << "\n";
    }
}

std::string getDeviceName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown Device";
}

std::string getEndpoint() {
    const char* endpoint = getenv("SYNQ_ENDPOINT");
    if (endpoint) {
        return std::string(endpoint);
    }
    return "http://127.0.0.1:5001";
}