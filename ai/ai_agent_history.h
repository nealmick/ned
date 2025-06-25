#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <functional>

struct ConversationEntry {
    std::string timestamp;
    std::string firstMessage;
    int messageCount;
};

class AIAgentHistory {
public:
    AIAgentHistory();
    ~AIAgentHistory();
    
    // Callback function type for loading conversations
    using LoadConversationCallback = std::function<void(const std::vector<std::string>& messages, const std::string& timestamp)>;
    
    void renderHistory();
    void setDisplayHistory(bool display);
    bool isDisplayingHistory() const;
    void toggleWindow();
    bool isWindowOpen() const;
    void refreshConversationList();
    void setLoadConversationCallback(LoadConversationCallback callback);

private:
    bool display_agent_history;
    bool showHistoryWindow;
    
    // Window and list management
    void renderHeader();
    void renderConversationList();
    void handleSelectionChange();
    void checkPendingSelection();
    void loadConversationFromHistory(const std::string& timestamp);
    
    // Data
    std::vector<ConversationEntry> conversationList;
    std::vector<ConversationEntry> filteredList;
    std::mutex conversationListMutex;
    
    // Selection state
    int selectedIndex;
    std::string pendingConversation;
    bool hasPendingSelection;
    std::chrono::steady_clock::time_point lastSelectionTime;
    bool isInitialSelection;
    
    // Window state
    std::string originalFile;
    
    // Callback for loading conversations
    LoadConversationCallback loadCallback;
}; 