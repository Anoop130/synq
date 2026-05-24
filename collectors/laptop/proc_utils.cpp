#include <iostream>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

struct ProcessInfo {
    int pid;
    std::string name;
    long memoryKB;
};

// reads CPU and memory stats for a single process from /proc/<pid>/stat.
// input:  pid (int) a valid process ID, range 1 to INT_MAX
// output: ProcessInfo with pid, name, and memoryKB fields populated;
//         name is empty and memoryKB is 0 if the stat file cannot be opened
ProcessInfo readProcessInfo(int pid) {
    ProcessInfo p;
    p.pid = pid;
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream statFile(statPath);
    if (!statFile.is_open()) return p;

    int ignore;
    std::string name;
    char state;
    long rss;

    statFile >> ignore >> name >> state;

    // skip fields before RSS
    std::string tmp;
    for (int i = 0; i < 20; ++i)
        statFile >> tmp;

    statFile >> rss;

    // remove parentheses around name
    if (name.front() == '(' && name.back() == ')')
        name = name.substr(1, name.size() - 2);

    p.name = name;
    long pageKB = sysconf(_SC_PAGESIZE) / 1024;
    p.memoryKB = rss * pageKB;
    return p;
}

// enumerates all running processes by scanning /proc for numeric directory entries.
// input:  none
// output: std::vector<ProcessInfo>, one entry per readable process; empty on /proc open failure
std::vector<ProcessInfo> getProcessList() {
    std::vector<ProcessInfo> processList;
    DIR* dir = opendir("/proc");
    if (!dir) {
        std::cerr << "[ERROR] cannot open /proc directory\n";
        return processList;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            ProcessInfo info = readProcessInfo(pid);
            if (!info.name.empty()) {
                processList.push_back(info);
            }
        }
    }
    closedir(dir);
    return processList;
}
