#include <curl/curl.h>
#include <string>

namespace api
{
    inline size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    inline void getResponseAsync(const std::string& url) {
        std::thread([url = url]() {
            curl_global_init(CURL_GLOBAL_ALL);
            CURL* curl = curl_easy_init();
            if (!curl) {
                curl_global_cleanup();
                return;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

            std::string response_body;
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

            curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            curl_global_cleanup();
            }).detach();
    }
}