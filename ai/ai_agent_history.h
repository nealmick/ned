#pragma once
#include "ai_message.h"
#include <chrono>
#include <functional>
#include <imgui.h>
#include <mutex>
#include <string>
#include <vector>

struct ConversationEntry
{
	std::string timestamp;
	std::string firstMessage;
	int messageCount;
};

// Forward declaration
class AIAgent;

class AIAgentHistory
{
  public:
	AIAgentHistory();
	~AIAgentHistory();

	// Callback function type for loading conversations
	using LoadConversationCallback =
		std::function<void(const std::vector<std::string> &messages,
						   const std::string &timestamp)>;
	using MessagesCallback = std::function<std::vector<Message>()>;
	using SetMessagesCallback = std::function<void(const std::vector<Message> &)>;
	using UpdateDisplayCallback = std::function<void()>;

	void renderHistory();
	void setDisplayHistory(bool display);
	bool isDisplayingHistory() const;
	void toggleWindow();
	bool isWindowOpen() const;
	void refreshConversationList();
	void setLoadConversationCallback(LoadConversationCallback callback);

	// Conversation history management methods
	void saveConversationHistory();
	void loadConversationHistory();
	void clearConversationHistory();
	void loadConversationFromHistory(const std::vector<std::string> &formattedMessages,
									 const std::string &timestamp);

	// Set callbacks for accessing messages and updating display
	void setMessagesCallback(MessagesCallback callback);
	void setSetMessagesCallback(SetMessagesCallback callback);
	void setUpdateDisplayCallback(UpdateDisplayCallback callback);
	void setCurrentConversationTimestamp(std::string &timestamp);

  private:
	bool display_agent_history;
	bool showHistoryWindow;

	// Window and list management
	void renderHeader();
	void renderConversationList();
	void handleSelectionChange();
	void checkPendingSelection();
	void loadConversationFromHistory(const std::string &timestamp);

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

	// Callbacks
	LoadConversationCallback loadCallback;
	MessagesCallback messagesCallback;
	SetMessagesCallback setMessagesCallbackVar;
	UpdateDisplayCallback updateDisplayCallback;
	std::string *currentConversationTimestamp;
};