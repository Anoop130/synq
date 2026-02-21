#include "proc_utils.h"
#include "config_utils.h"
#include "x11_utils.h"
#include "net_utils.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>

int main() {
    // Initialize configuration and endpoints
    std::string baseEndpoint = getEndpoint();
    std::string collectEndpoint = baseEndpoint + "/collect";
    int interval = 5; // Sampling interval in seconds

    std::ofstream log("build/logs/process_sample.log", std::ios::app);
    if (!log.is_open()) {
        std::cerr << "Could not open local log file!\n";
        return 1;
    }

    std::cout << "Starting Synq collector...\n";
    std::cout << "Endpoint: " << baseEndpoint << "\n";

    // Retrieve or register device with server
    std::string deviceId = getDeviceId();
    std::string deviceName = getDeviceName();
    
    std::cout << "Device ID: " << deviceId << "\n";
    std::cout << "Device Name: " << deviceName << "\n";

    // Attempt device registration (upsert operation on server)
    std::string registeredId = registerDevice(baseEndpoint, deviceName, "linux");
    if (!registeredId.empty() && registeredId != deviceId) {
        // Server returned updated ID, persist locally
        deviceId = registeredId;
        saveDeviceId(deviceId);
        std::cout << "Registered with server. Device ID: " << deviceId << "\n";
    } else if (registeredId.empty()) {
        std::cerr << "Warning: Could not register with server, using cached ID\n";
    } else {
        std::cout << "Device already registered\n";
    }

    std::cout << "\nCollecting samples every " << interval << " seconds...\n\n";

    // Continuous activity collection loop
    while (true) {
        // Generate timestamp for current sample
        std::time_t now = std::time(nullptr);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf),
                      "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        // Capture active window title from X11
        std::string activeWindow = getActiveWindowTitle();

        // Build JSON payload with device identification
        std::ostringstream json;
        json << "{"
             << "\"device_id\":\"" << deviceId << "\","
             << "\"timestamp\":\"" << timeBuf << "\","
             << "\"active_window\":\"" << activeWindow << "\""
             << "}";

        // Transmit sample to server
        bool ok = sendSample(collectEndpoint, json.str());
        std::cout << "[" << timeBuf << "] "
                  << (ok ? "[OK]" : "[FAILED]")
                  << " | " << activeWindow << "\n";

        // Write to local log file
        log << "[" << timeBuf << "] "
            << activeWindow << " | "
            << (ok ? "sent" : "failed") << "\n";
        log.flush();

        // Wait for next sampling interval
        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }

    return 0;
}
