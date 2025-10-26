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
    // 1️Setup 
    std::string endpoint = "http://127.0.0.1:5000/collect";
    int interval = 5; // seconds between samples

    std::ofstream log("build/logs/process_sample.log", std::ios::app);
    if (!log.is_open()) {
        std::cerr << "Could not open local log file!\n";
        return 1;
    }

    std::cout << "Starting collector. Sending data every "
              << interval << " seconds to " << endpoint << "\n";

    // 2Continuous collection loop 
    while (true) {
        // timestamp
        std::time_t now = std::time(nullptr);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf),
                      "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        // Active window title (from X11)
        std::string activeWindow = getActiveWindowTitle();

        // process count for reference
        auto processes = getProcessList();
        size_t procCount = processes.size();

        // Build JSON payload
        std::ostringstream json;
        json << "{"
             << "\"device\":\"laptop\","
             << "\"timestamp\":\"" << timeBuf << "\","
             << "\"active_window\":\"" << activeWindow << "\","
             << "\"process_count\":" << procCount
             << "}";

        // Send to server
        bool ok = sendSample(endpoint, json.str());
        std::cout << "[" << timeBuf << "] "
                  << (ok ? "Sent" : "Failed")
                  << " | Active window: " << activeWindow << "\n";

        // Log locally 
        log << "[" << timeBuf << "] "
            << activeWindow << " | "
            << procCount << " processes | "
            << (ok ? "sent" : "failed") << "\n";
        log.flush();

        // Wait until next sample
        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }

    return 0;
}
