// ai_agent.h
#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include "ai_open_router.h"
#include "../util/settings_file_manager.h"
#include "textselect.hpp"
#include "agent_request.h"
#include "ai_agent_history.h"
#include "ai_agent_text_input.h"
#include "ai_message.h"

class AIAgent {
public:
    AIAgent();
    ~AIAgent();
    void render(float agentPaneWidth, ImFont* largeFont = nullptr);
    void sendMessage(const char* msg, bool hide_message = false);
    void printAllMessages();

    // Access to history manager
    AIAgentHistory& getHistoryManager() { return historyManager; }

    // Conversation history management (for internal use)
    void loadConversationFromHistory(const std::vector<std::string>& formattedMessages, const std::string& timestamp);

    // Make messages public so it can be accessed from agent_request.cpp
    std::vector<Message> messages;
    std::mutex messagesMutex;
    bool needsFollowUpMessage = false; // Flag to trigger follow-up message

private:
    char inputBuffer[256] = {0};
    unsigned int frameCounter;
    void renderMessageHistory(const ImVec2& size, ImFont* largeFont = nullptr);
    bool scrollToBottom = false;
    
    // Agent request handler
    AgentRequest agentRequest;
    
    // Message display and UI
    std::vector<std::string> messageDisplayLines;
    std::atomic<bool> messageDisplayLinesDirty{true};
    TextSelect textSelect;
    void rebuildMessageDisplayLines();
    void wrapTextToWidth(const std::string& text, float maxWidth);
    bool userScrolledUp = false; // Tracks if user has manually scrolled up in message history
    bool justAutoScrolled = false; // Prevents userScrolledUp from being set right after auto-scroll
    bool forceScrollToBottomNextFrame = false;
    float lastKnownWidth = 0.0f; // Track last known width for detecting changes
    
    // Helper method for stopping streaming
    void stopStreaming();
    
    // History management
    AIAgentHistory historyManager;
    
    // Track currently loaded conversation for proper updates
    std::string currentConversationTimestamp;
    
    // Text input component
    AIAgentTextInput textInput;
};