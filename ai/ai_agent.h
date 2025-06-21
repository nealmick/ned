// ai_agent.h
#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "ai_open_router.h"
#include "../util/settings_file_manager.h"
#include "textselect.hpp"

class AIAgent {
public:
    struct Message {
        std::string text;
        bool isAgent;
        bool isStreaming = false;
    };

    AIAgent();
    ~AIAgent();
    void render(float agentPaneWidth);
    void sendMessage(const char* msg);
    void AgentInput(const ImVec2& textBoxSize, float textBoxWidth, float horizontalPadding);
    void printAllMessages();

private:
    std::vector<Message> messages;
    char inputBuffer[256] = {0};
    unsigned int frameCounter;
    void renderMessageHistory(const ImVec2& size);
    bool scrollToBottom = false;
    bool shouldRestoreFocus = false;
    
    // Streaming support
    std::atomic<bool> isStreaming{false};
    std::atomic<bool> shouldCancelStreaming{false};
    std::thread streamingThread;
    std::mutex messagesMutex;
    void startStreamingRequest(const std::string& prompt, const std::string& api_key);
    void stopStreaming();
    std::string utf8_buffer; // Buffer for incomplete UTF-8 bytes during streaming
    std::vector<std::string> messageDisplayLines;
    std::atomic<bool> messageDisplayLinesDirty{true};
    TextSelect textSelect;
    void rebuildMessageDisplayLines();
    void wrapTextToWidth(const std::string& text, float maxWidth);
    bool userScrolledUp = false; // Tracks if user has manually scrolled up in message history
    bool justAutoScrolled = false; // Prevents userScrolledUp from being set right after auto-scroll
    bool forceScrollToBottomNextFrame = false;
    float lastKnownWidth = 0.0f; // Track last known width for detecting changes
};