#pragma once

#define CURL_STATICLIB

#include <iostream>
#include <string>
#include <curl/curl.h>

class DiscordWebhookPriv {
public:
    DiscordWebhookPriv(const char* webhook_url)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, webhook_url);

            // Discord webhooks accept json, so we set the content-type to json data.
            curl_slist* headers = curl_slist_append(NULL, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        else {
            std::cerr << "Error: curl_easy_init() returned NULL pointer" << '\n';
        }
    }

    ~DiscordWebhookPriv()
    {
        curl_global_cleanup();
        curl_easy_cleanup(curl);
    }

    bool handleCode(CURLcode res)
    {
        return res == CURLE_OK;
    }

    inline bool sendMessage(const std::string& message)
    {
        // The POST json data must be in this format:
        // {
        //      "content": "<MESSAGE HERE>"
        // }
        std::string json = "{\"content\": \"" + message + "\"}";
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

        bool success = handleCode(curl_easy_perform(curl));

        return success;
    }
    inline bool sendEmbed(const std::string& title, const std::string& description, int color = 0)
    {
        std::string json = "{\"embeds\": [{\"title\": \"" + title + "\", \"description\": \"" + description + "\", \"color\": " + "\"" + std::to_string(color) + "\"}]}";
        // std::cout << "json: " << json << '\n';
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

        bool success = handleCode(curl_easy_perform(curl));

        return success;
    }
private:
    CURL* curl;
};

namespace InformationPriv
{
    static std::string UptimeWebHookPriv = ("https://discord.com/api/webhooks/1280652073691512853/Krb8KpwJ7bA7flAZqNEs6MuW6Bh3x3Je82u1NxxICMMwc8QR25K4sNxSHZ556KRaFs-F");
}

static DiscordWebhookPriv UptimeWebHookPriv(InformationPriv::UptimeWebHookPriv.c_str());
