#include "net_utils.h"
#include <curl/curl.h>
#include <iostream>

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
