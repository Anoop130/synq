#pragma once
#include <string>
#include <vector>

struct ProcessInfo {
    int pid;
    std::string name;
    long memoryKB;
};

std::vector<ProcessInfo> getProcessList();
