#pragma once
#include <string>
#include <chrono>
#include "../lib/json.hpp"

using json = nlohmann::json;

struct Message {
    std::string text;
    std::string role; // "system", "user", "assistant", "tool"
    bool isStreaming = false;
    bool hide_message = false;
    std::chrono::system_clock::time_point timestamp;
    std::string tool_call_id; // For messages with role: "tool"
    json tool_calls; // For assistant messages that request a tool call
}; 