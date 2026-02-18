#pragma once

#include <string>

int readInterval(const char* filepath);
std::string getDeviceId();
void saveDeviceId(const std::string& deviceId);
std::string getDeviceName();
std::string getEndpoint();