#include "config_utils.h"
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

// returns the path to the synq XDG config directory, creating it if absent.
// input:  none
// output: std::string, absolute path e.g. /home/user/.config/synq, unbounded length
std::string getConfigDir() {
    const char* home = getenv("HOME");
    if (!home) home = ".";
    std::string configDir = std::string(home) + "/.config/synq";
    mkdir(configDir.c_str(), 0755);
    return configDir;
}

// generates a random UUID v4 string.
// input:  none
// output: std::string, 36-char UUID in the form xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
static std::string generateUUID() {
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

// reads the interval value from a key=value config file.
// returns 5 if the file does not exist or the key is absent.
// input:  filepath (const char ptr) path to config file, null terminated
// output: int, the parsed interval value in seconds, minimum 1
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

// reads the persisted device UUID from disk, generating and saving a new one if absent.
// input:  none
// output: std::string, 36-char UUID v4
std::string getDeviceId() {
    std::string configDir  = getConfigDir();
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

    // generate new UUID if no existing configuration found
    std::string newId = generateUUID();
    saveDeviceId(newId);
    return newId;
}

// writes the device UUID to the config file, overwriting any existing value.
// input:  deviceId (const std::string ref) UUID v4, 36 chars
// output: none
void saveDeviceId(const std::string& deviceId) {
    std::string configDir  = getConfigDir();
    std::string deviceFile = configDir + "/device.conf";

    std::ofstream file(deviceFile);
    if (file.is_open()) {
        file << "device_id=" << deviceId << "\n";
    }
}

// returns the machine hostname as the human readable device name.
// input:  none
// output: std::string, hostname string or "Unknown Device" on failure, unbounded length
std::string getDeviceName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown Device";
}

// reads the server base URL from the SYNQ_ENDPOINT environment variable.
// input:  none
// output: std::string, URL string; defaults to http://127.0.0.1:5001
std::string getEndpoint() {
    const char* endpoint = getenv("SYNQ_ENDPOINT");
    if (endpoint) {
        return std::string(endpoint);
    }
    return "http://127.0.0.1:5001";
}
