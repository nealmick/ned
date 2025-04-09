#include "ai_open_router.h"
#include "../lib/json.hpp"
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;

static size_t WriteData(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

std::string OpenRouter::request(const std::string &prompt, const std::string &api_key)
{
    CURL *curl = curl_easy_init();
    std::string response;
    long http_code = 0;

    if (curl) {
        // Build JSON payload properly with escaping
        json payload = {
            {"model", "meta-llama/llama-4-scout"},
            {"messages", {{{"role", "system"}, {"content", "Respond ONLY with code completion. No explanations."}}, {{"role", "user"}, {"content", prompt}}}},
            {"temperature", 0.2}, // More focused responses (0-1 scale)
            {"max_tokens", 150}   // Shorter responses for speed
        };
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor"); // Required header

        std::string json_str = payload.dump();

        // Debug output
        std::cout << "Sending payload:" << std::endl;
        std::cout << json_str << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        // Get HTTP status code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Cleanup
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        // Handle errors
        if (res != CURLE_OK) {
            return "cURL error: " + std::string(curl_easy_strerror(res));
        }

        if (http_code != 200) {
            return "HTTP error " + std::to_string(http_code) + ": " + response;
        }

        // Parse JSON response
        try {
            json result = json::parse(response);
            return result["choices"][0]["message"]["content"].get<std::string>();
        } catch (const json::exception &e) {
            return "JSON parse error: " + std::string(e.what()) + "\nResponse: " + response;
        }
    }

    return "Failed to initialize cURL";
}