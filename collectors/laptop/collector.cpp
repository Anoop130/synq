#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>

int main() {
    std::cout << "Synq Collector Build Test\n";
    CURL *curl = curl_easy_init();
    if (curl) {
        std::cout << "libcurl OK\n";
        curl_easy_cleanup(curl);
    }
    sqlite3 *db;
    if (sqlite3_open(":memory:", &db) == SQLITE_OK) {
        std::cout << "SQLite OK\n";
        sqlite3_close(db);
    }
    return 0;
}
