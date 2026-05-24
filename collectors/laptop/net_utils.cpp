#include "net_utils.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// callback to accumulate HTTP response body into a std::string.
// input:  contents (void ptr) raw bytes from libcurl, size (size_t) always 1,
//         nmemb (size_t) byte count for this chunk, userp (std::string ptr) accumulator
// output: size_t, number of bytes consumed; returning less signals an error to libcurl
static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// sends a JSON payload to the collect endpoint via HTTP POST.
// input:  endpoint (const std::string ref) full URL, unbounded length
//         jsonPayload (const std::string ref) valid JSON string, unbounded length
// output: bool, true if the HTTP request completed without a transport error
bool sendSample(const std::string& endpoint, const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ERROR] libcurl init failed in sendSample\n";
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    bool success = (res == CURLE_OK);

    if (!success)
        std::cerr << "[ERROR] curl send: " << curl_easy_strerror(res) << "\n";

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return success;
}

// registers a device with the server via HTTP POST to /register.
// parses the device_id field out of the JSON response body.
// input:  baseEndpoint (const std::string ref) server base URL, unbounded length
//         deviceId   (const std::string ref) UUID v4, max 36 chars
//         deviceName (const std::string ref) hostname label, max 255 chars
//         deviceType (const std::string ref) platform string e.g. linux, max 50 chars
// output: std::string, the device_id echoed by the server, or empty string on failure
std::string registerDevice(const std::string& baseEndpoint, const std::string& deviceId,
                           const std::string& deviceName, const std::string& deviceType) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ERROR] libcurl init failed in registerDevice\n";
        return "";
    }

    std::string endpoint    = baseEndpoint + "/register";
    std::string jsonPayload = "{\"device_id\":\"" + deviceId + "\","
                              "\"device_name\":\"" + deviceName + "\","
                              "\"device_type\":\"" + deviceType + "\"}";
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl registration: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    // extract device_id from JSON response
    size_t idPos = response.find("\"device_id\":\"");
    if (idPos != std::string::npos) {
        idPos += 13; // length of "device_id":"
        size_t endPos = response.find("\"", idPos);
        if (endPos != std::string::npos) {
            return response.substr(idPos, endPos - idPos);
        }
    }

    return "";
}
