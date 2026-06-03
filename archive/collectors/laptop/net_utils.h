#pragma once
#include <string>

bool sendSample(const std::string& endpoint, const std::string& jsonPayload);
std::string registerDevice(const std::string& endpoint, const std::string& deviceId, const std::string& deviceName, const std::string& deviceType);