#include "proc_utils.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>

int main() {
    std::ofstream log("build/logs/process_sample.log");
    if (!log.is_open()) {
        std::cerr << "Could not open log file!\n";
        return 1;
    }

    while (true) {
        auto list = getProcessList();
        std::time_t now = std::time(nullptr);
        log << "SAMPLE @ " << std::ctime(&now) << "\n";
        for (auto& p : list)
            log << p.pid << " " << p.name << " " << p.memoryKB << " KB\n";
        log.flush();

        std::cout << "Captured " << list.size() << " processes\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}
