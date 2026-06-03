#include "net_utils.h"
#include <iostream>

int main() {
    std::string endpoint = "https://httpbin.org/post";
    std::string payload  = R"({"device":"laptop","window":"VSCode","timestamp":"2025-10-25"})";

    std::cout << "[INFO] sending POST to " << endpoint << "\n";
    bool ok = sendSample(endpoint, payload);
    std::cout << (ok ? "[INFO] success\n" : "[ERROR] failed\n");
    return 0;
}
