#include "config_utils.h"
#include "x11_utils.h"
#include "net_utils.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <X11/Xlib.h>

// escapes special characters in s for safe embedding in a JSON string value.
// input:  s (const std::string ref) raw window title, unbounded length
// output: std::string, JSON safe escaped copy of s
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

int main() {
    XInitThreads();

    bool verbose = (std::getenv("SYNQ_VERBOSE") != nullptr);

    std::string baseEndpoint = getEndpoint();
    std::string collectEndpoint = baseEndpoint + "/collect";

    std::string configDir = getConfigDir();
    int interval = readInterval((configDir + "/config.ini").c_str());
    if (interval <= 0) interval = 5;

    const char* home = std::getenv("HOME");
    std::string logDir = home ? (std::string(home) + "/.local/share/synq") : ".";
    mkdir(logDir.c_str(), 0755);
    std::string logPath = logDir + "/collector.log";

    std::ofstream logFile(logPath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "[ERROR] cannot open log file: " << logPath << "\n";
        return 1;
    }

    std::cout << "[INFO] starting Synq collector\n";
    std::cout << "[INFO] endpoint: " << baseEndpoint << "\n";
    std::cout << "[INFO] log: " << logPath << "\n";
    std::cout << "[INFO] interval: " << interval << "s\n";

    std::string deviceId   = getDeviceId();
    std::string deviceName = getDeviceName();

    std::cout << "[INFO] device id: " << deviceId << "\n";
    std::cout << "[INFO] device name: " << deviceName << "\n";

    std::string registeredId = registerDevice(baseEndpoint, deviceId, deviceName, "linux");
    if (!registeredId.empty() && registeredId != deviceId) {
        deviceId = registeredId;
        saveDeviceId(deviceId);
        std::cout << "[INFO] registered with server, device id: " << deviceId << "\n";
    } else if (registeredId.empty()) {
        std::cerr << "[WARN] registration failed, using cached device id\n";
    } else {
        std::cout << "[INFO] device already registered\n";
    }

    std::cout << "[INFO] collecting samples every " << interval << " seconds\n";

    while (true) {
        std::time_t now = std::time(nullptr);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        std::string activeWindow = getActiveWindowTitle();

        std::ostringstream json;
        json << "{"
             << "\"device_id\":\""    << jsonEscape(deviceId)     << "\","
             << "\"timestamp\":\""    << timeBuf                  << "\","
             << "\"active_window\":\"" << jsonEscape(activeWindow) << "\""
             << "}";

        bool ok = sendSample(collectEndpoint, json.str());

        if (!ok) {
            std::cerr << "[WARN] sample send failed at " << timeBuf << "\n";
        }

        logFile << "[" << timeBuf << "] "
                << (ok ? "sent" : "failed") << " | " << activeWindow << "\n";
        logFile.flush();

        if (verbose) {
            std::cout << "[DEBUG] " << timeBuf << " "
                      << (ok ? "ok" : "failed") << " | " << activeWindow << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }

    return 0;
}
