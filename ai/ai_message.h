#pragma once
#include <string>
#include <chrono>

struct Message {
    std::string text;
    bool isAgent;
    bool isStreaming = false;
    bool hide_message = false;
    std::chrono::system_clock::time_point timestamp;
}; 