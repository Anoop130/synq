#include "net_utils.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// Callback to capture response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool sendSample(const std::string& endpoint, const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[curl] Failed to initialize\n";
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // 5s timeout

    CURLcode res = curl_easy_perform(curl);
    bool success = (res == CURLE_OK);

    if (!success)
        std::cerr << "[curl] Error: " << curl_easy_strerror(res) << "\n";

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return success;
}

std::string registerDevice(const std::string& baseEndpoint, const std::string& deviceName, const std::string& deviceType) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[curl] Failed to initialize for registration\n";
        return "";
    }

    std::string endpoint = baseEndpoint + "/register";
    std::string jsonPayload = "{\"device_name\":\"" + deviceName + "\",\"device_type\":\"" + deviceType + "\"}";
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[curl] Registration failed: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    // Extract device_id from JSON response
    size_t idPos = response.find("\"device_id\":\"");
    if (idPos != std::string::npos) {
        idPos += 13; // Length of "device_id":""
        size_t endPos = response.find("\"", idPos);
        if (endPos != std::string::npos) {
            return response.substr(idPos, endPos - idPos);
        }
    }

    return "";
}
