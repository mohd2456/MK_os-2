#ifndef MK_HTTP_CPP
#define MK_HTTP_CPP

#include <iostream>
#include <string>
#include <cstddef>
#include <curl/curl.h>

class MKHTTP {
public:
    // Callback function to handle incoming data streams from libcurl
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Performance-optimized HTTP GET request wrapper
    std::string get(const std::string& url) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            // Performs the synchronous transfer
            res = curl_easy_perform(curl);
            
            if(res != CURLE_OK) {
                std::cerr << "[HTTP ERROR] Fetch failed: " << curl_easy_strerror(res) << "\n";
            }
            
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }
};

#endif // MK_HTTP_CPP