#include "net_utils.h"
#include <iostream>

int main() {
    std::string endpoint = "https://httpbin.org/post";  // test endpoint
    std::string payload  = R"({"device":"laptop","window":"VSCode","timestamp":"2025-10-25"})";

    std::cout << "Sending POST to " << endpoint << "...\n";
    bool ok = sendSample(endpoint, payload);
    std::cout << (ok ? "Success\n" : "Failed\n");
    return 0;
}
