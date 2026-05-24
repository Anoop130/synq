#pragma once

#include <string>

std::string getConfigDir();
int readInterval(const char* filepath);
std::string getDeviceId();
void saveDeviceId(const std::string& deviceId);
std::string getDeviceName();
std::string getEndpoint();