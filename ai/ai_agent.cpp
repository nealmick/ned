#include "ai_agent.h"
#include "../files/files.h" // for gFileExplorer
#include "../lib/json.hpp"
#include "agent_request.h"
#include "editor/editor.h" // for editor_state
#include "mcp/mcp_manager.h"
#include "textselect.hpp"
#include "util/settings.h"
#include "util/settings_file_manager.h"
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#ifdef _WIN32
// Fix for Windows UTF-8 library assert macro conflict
#include <cassert>
#ifdef assert
#undef assert
#endif
#include <utf8.h>
#ifdef _WIN32
#define assert(expr) ((void)0)
#endif
#else
#include <utf8.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

// External global MCP manager instance
extern MCP::Manager gMCPManager;

// ImGuiTextSelect integration for selectable/copyable message history
// Requires textselect.cpp/textselect.hpp and utfcpp (utf8.h) in the include
// path See: https://github.com/WhaleConnect/mGuiTextSelect

AIAgent::AIAgent()
	: frameCounter(0), textSelect([](size_t) { return ""; },
								  [] { return 0; },
								  true), // dummy accessors, word wrap enabled
	  forceScrollToBottomNextFrame(false), lastKnownWidth(0.0f),
	  lastToolCallTime(std::chrono::system_clock::now())
{
	// Initialize input buffer
	strncpy(inputBuffer, "", sizeof(inputBuffer));

	// Set up history manager callback
	historyManager.setLoadConversationCallback(
		[this](const std::vector<std::string> &messages, const std::string &timestamp) {
			loadConversationFromHistory(messages, timestamp);
		});

	// Set up text input component
	textInput.setInputBuffer(inputBuffer, sizeof(inputBuffer));
	textInput.setSendMessageCallback(
		[this](const char *msg, bool hide_message) { sendMessage(msg, hide_message); });
	textInput.setIsProcessingCallback([this]() { return agentRequest.isProcessing(); });
	textInput.setNotificationCallback([](const char *msg, float duration) {
		gSettings.renderNotification(msg, duration);
	});
	textInput.setClearConversationCallback(
		[this]() { historyManager.clearConversationHistory(); });
	textInput.setToggleHistoryCallback([this]() { historyManager.toggleWindow(); });
	textInput.setBlockInputCallback([](bool block) { editor_state.block_input = block; });
	textInput.setStopRequestCallback([this]() { stopStreaming(); });

	// Set up history manager callbacks for accessing messages and updating display
	historyManager.setMessagesCallback([this]() -> std::vector<Message> {
		std::lock_guard<std::mutex> lock(messagesMutex);
		return messages;
	});
	historyManager.setSetMessagesCallback([this](const std::vector<Message> &newMessages) {
		std::lock_guard<std::mutex> lock(messagesMutex);
		messages = newMessages;
	});
	historyManager.setUpdateDisplayCallback([this]() {
		messageDisplayLinesDirty = true;
		scrollToBottom = true;
	});
	historyManager.setCurrentConversationTimestamp(currentConversationTimestamp);
}

AIAgent::~AIAgent() { stopStreaming(); }

void AIAgent::stopStreaming() { agentRequest.stopRequest(); }

// Helper method to check if a tool call should be allowed (prevents loops)
bool AIAgent::shouldAllowToolCall(const std::string &toolName)
{
	auto now = std::chrono::system_clock::now();
	auto timeSinceLastCall =
		std::chrono::duration_cast<std::chrono::seconds>(now - lastToolCallTime).count();

	// Reset failure counts if more than 30 seconds have passed
	if (timeSinceLastCall > 30)
	{
		failedToolCalls.clear();
	}

	// Check if this tool has failed too many times
	auto it = failedToolCalls.find(toolName);
	if (it != failedToolCalls.end() && it->second >= MAX_FAILED_CALLS)
	{
		return false;
	}

	lastToolCallTime = now;
	return true;
}

// Helper method to record a failed tool call
void AIAgent::recordFailedToolCall(const std::string &toolName)
{
	failedToolCalls[toolName]++;
	std::cout << "DEBUG: Recorded failed tool call for " << toolName
			  << " (count: " << failedToolCalls[toolName] << ")" << std::endl;
}

void AIAgent::render(float agentPaneWidth, ImFont *largeFont)
{

	float inputWidth = ImGui::GetContentRegionAvail().x;
	float windowHeight = ImGui::GetWindowHeight();
	float horizontalPadding = 16.0f;
	float textBoxWidth = inputWidth; // Do NOT subtract padding here
	if (textBoxWidth < 50.0f)
		textBoxWidth = 50.0f;

	ImGui::BeginGroup();
	ImGui::Dummy(ImVec2(horizontalPadding - 10, 0)); // Left padding
	ImGui::SameLine();

	ImGui::BeginGroup(); // Vertical stack for message history, button, and input
	// Calculate history height - reduce it if we need to show the key input
	float historyHeight = windowHeight * 0.7f;
	if (hasApiKeyError)
	{
		// Reduce history height to make room for the key input area
		// The key input area takes roughly 120-140 pixels (including separators and spacing)
		historyHeight -= 190.0f;
		if (historyHeight < 100.0f) // Ensure minimum height
			historyHeight = 100.0f;
	}

	// Account for scroll bar width and padding to prevent overflow
	float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
	float historyWidth = textBoxWidth - 2 * horizontalPadding - scrollbarWidth +
						 45.0f; // Subtract scrollbar width and extra padding
	if (historyWidth < 50.0f)
		historyWidth = 50.0f; // Minimum width
	ImVec2 historySize = ImVec2(historyWidth, historyHeight);
	renderMessageHistory(historySize, largeFont);
	ImGui::Spacing();

	// Render OpenRouter key input if there are API key errors
	renderOpenRouterKeyInput(textBoxWidth, horizontalPadding);

	float lineHeight = ImGui::GetTextLineHeightWithSpacing();
	float fontSize = ImGui::GetFontSize();
	int numLines =
		(fontSize > 30.0f) ? 2 : 3; // 2 lines for large fonts, 3 for normal fonts
	ImVec2 textBoxSize =
		ImVec2(textBoxWidth - 2 * horizontalPadding, lineHeight * numLines + 16.0f);
	textInput.render(textBoxSize, textBoxWidth - 2 * horizontalPadding, horizontalPadding);
	ImGui::EndGroup(); // End vertical group

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(horizontalPadding, 0)); // Right padding
	ImGui::EndGroup();

	// Render history if enabled
	historyManager.renderHistory();
}

void AIAgent::rebuildMessageDisplayLines()
{
	std::vector<Message> messagesCopy;
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		messagesCopy = messages;
	}
	messageDisplayLines.clear();

	// Calculate available width for text (accounting for padding, scrollbar,
	// and some buffer)
	float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
	float childPadding =
		ImGui::GetStyle().ChildRounding * 2.0f; // Account for child window padding
	float availableWidth = lastKnownWidth - scrollbarWidth - childPadding -
						   8.0f; // scrollbar + child padding + buffer
	if (availableWidth < 50.0f)
		availableWidth = 50.0f; // Minimum width

	for (size_t i = 0; i < messagesCopy.size(); ++i)
	{
		const auto &msg = messagesCopy[i];

		// std::cout << "Processing message " << i << ": role=" << msg.role <<
		// ", text length=" << msg.text.length() << std::endl;

		// Skip hidden messages
		if (msg.hide_message)
		{
			// std::cout << "Skipping hidden message" << std::endl;
			continue;
		}

		// Add separator line before each message (except the first one)
		if (i > 0)
		{
			// Calculate separator width based on available width and font metrics
			float separatorWidth = availableWidth;
			ImVec2 dashSize = ImGui::CalcTextSize("-"); // Get dash character width
			float charAdvance = dashSize.x;
			int numDashes = static_cast<int>(separatorWidth / charAdvance);
			if (numDashes < 3)
				numDashes = 3; // Minimum 3 dashes

			std::string separator(numDashes, '-');
			messageDisplayLines.push_back(separator);
		}

		std::string displayText = msg.text;

		// Use role-based display only
		if (msg.role == "assistant")
		{
			displayText = "✨ Agent: " + displayText;

			// If this assistant message contains tool calls, add details about them
			if (!msg.tool_calls.is_null() && msg.tool_calls.is_array() &&
				!msg.tool_calls.empty())
			{
				displayText += "\n[Tool calls made:";
				for (size_t i = 0; i < msg.tool_calls.size(); ++i)
				{
					const auto &toolCall = msg.tool_calls[i];
					if (toolCall.contains("function") &&
						toolCall["function"].contains("name") &&
						!toolCall["function"]["name"].is_null())
					{
						std::string toolName =
							toolCall["function"]["name"].get<std::string>();
						displayText += " " + toolName;
						if (i < msg.tool_calls.size() - 1)
							displayText += ",";
					}
				}
				displayText += "]";
			}
		} else if (msg.role == "user")
		{
			displayText = "🧑 User: " + displayText;
		} else if (msg.role == "tool")
		{
			displayText = "🔧 Tool Result: " + displayText;
		} else if (msg.role == "system")
		{
			displayText = "⚙️ System: " + displayText;
		}

		// Handle both existing newlines and word wrapping
		size_t start = 0;
		while (start < displayText.size())
		{
			// First, find the next existing newline
			size_t nextNewline = displayText.find('\n', start);
			if (nextNewline == std::string::npos)
			{
				// No more newlines, wrap the remaining text
				std::string remainingText = displayText.substr(start);
				wrapTextToWidth(remainingText, availableWidth);
				break;
			} else
			{
				// Found a newline, wrap the text up to it
				std::string lineText = displayText.substr(start, nextNewline - start);
				wrapTextToWidth(lineText, availableWidth);
				start = nextNewline + 1;
			}
		}
	}
}

// Helper function to wrap text to a specific width
void AIAgent::wrapTextToWidth(const std::string &text, float maxWidth)
{
	if (text.empty())
	{
		messageDisplayLines.push_back("");
		return;
	}

	// Add a small buffer to prevent character cutoff
	float safeWidth = maxWidth - 4.0f; // 4px buffer

	std::string currentLine;
	std::string currentWord;

	for (size_t i = 0; i < text.size(); ++i)
	{
		char c = text[i];

		if (c == ' ' || c == '\t')
		{
			// End of word, try to add it to current line
			if (!currentWord.empty())
			{
				std::string testLine =
					currentLine + (currentLine.empty() ? "" : " ") + currentWord;
				float lineWidth = ImGui::CalcTextSize(testLine.c_str()).x;

				if (lineWidth - 10.0f <= safeWidth)
				{
					currentLine = testLine;
				} else
				{
					// Word doesn't fit, start new line
					if (!currentLine.empty())
					{
						messageDisplayLines.push_back(currentLine);
					}
					currentLine = currentWord;
				}
				currentWord.clear();
			}
			// Don't add spaces to currentLine here - they'll be added when we
			// add the next word
		} else
		{
			// Add character to current word
			currentWord += c;
		}
	}

	// Handle the last word
	if (!currentWord.empty())
	{
		std::string testLine =
			currentLine + (currentLine.empty() ? "" : " ") + currentWord;
		float lineWidth = ImGui::CalcTextSize(testLine.c_str()).x;

		if (lineWidth <= safeWidth)
		{
			currentLine = testLine;
		} else
		{
			// Last word doesn't fit, start new line
			if (!currentLine.empty())
			{
				messageDisplayLines.push_back(currentLine);
			}
			currentLine = currentWord;
		}
	}

	// Add the final line if not empty
	if (!currentLine.empty())
	{
		messageDisplayLines.push_back(currentLine);
	}
}

void AIAgent::renderMessageHistory(const ImVec2 &size, ImFont *largeFont)
{
	// Check if width has changed and trigger rebuild if needed
	float currentWidth = size.x;
	if (std::abs(currentWidth - lastKnownWidth) > 1.0f)
	{ // Small threshold to avoid unnecessary rebuilds
		messageDisplayLinesDirty.store(true);
		lastKnownWidth = currentWidth;
	}

	// Check if font size has changed and trigger rebuild if needed
	if (gSettings.hasFontSizeChanged())
	{
		messageDisplayLinesDirty.store(true);
	}

	if (messageDisplayLinesDirty.load())
	{
		rebuildMessageDisplayLines();
		textSelect = TextSelect(
			[this](size_t idx) -> std::string_view {
				if (idx < messageDisplayLines.size())
					return messageDisplayLines[idx];
				return "";
			},
			[this]() -> size_t { return messageDisplayLines.size(); },
			true // word wrap
		);
		messageDisplayLinesDirty.store(false);
	}

	// Get the position for drawing the border
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 borderMin = pos;
	ImVec2 borderMax = ImVec2(pos.x + size.x, pos.y + size.y);
	// Draw border manually

	/*

	ImGui::GetWindowDrawList()->AddRect(
		borderMin,
		borderMax,
		ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)),
		4.0f, // rounding
		0,    // flags
		2.0f  // thickness
	);
	*/

	// Use vertical scrollbar only when needed
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove;

	// Apply custom styling for the scrollable area
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,
						13.0f); // Make scrollbar wider for better usability
	ImGui::PushStyleColor(ImGuiCol_ChildBg,
						  ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,
						  ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Hide scrollbar track
	ImGui::PushStyleColor(
		ImGuiCol_ScrollbarGrab,
		ImVec4(0.4f, 0.4f, 0.4f, 0.5f)); // Completely transparent by default
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
						  ImVec4(0.55f, 0.55f, 0.55f, 0.8f)); // Show on hover
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
						  ImVec4(0.7f, 0.7f, 0.7f, 0.9f)); // Show when active

	ImGui::BeginChild("text", size, false, flags);

	// Check if chat is empty and show centered "Agent" title
	if (messageDisplayLines.empty())
	{
		// Calculate center position
		ImVec2 windowSize = ImGui::GetWindowSize();

		// Static variables for dropdown (shared between both font sections)
		static int selectedItem = 0;
		static std::string currentAgentModel = "";
		static std::vector<std::string> dropdownItems;
		static std::vector<std::string> displayItems; // For display purposes
		static bool dropdownInitialized = false;

		// Check if profile changed or first time initializing
		std::string newAgentModel = gSettings.getAgentModel();
		if (!dropdownInitialized || gSettings.profileJustSwitched ||
			currentAgentModel != newAgentModel)
		{
			currentAgentModel = newAgentModel;
			dropdownItems.clear();
			displayItems.clear();

			// Add current agent model as first option
			dropdownItems.push_back(currentAgentModel);

			// Create display version for current model (show left part before slash)
			std::string displayModel = currentAgentModel;
			size_t slashPos = currentAgentModel.find('/');
			if (slashPos != std::string::npos)
			{
				displayModel =
					currentAgentModel.substr(slashPos + 1,
											 currentAgentModel.length() - slashPos -
												 1); // Show part after slash
			} else
			{
				displayModel = currentAgentModel;
			}
			displayItems.push_back(displayModel);

			// Add models grouped by provider
			// Claude models
			dropdownItems.push_back("anthropic/claude-sonnet-4");
			dropdownItems.push_back("anthropic/claude-3-5-haiku-20241022");
			dropdownItems.push_back("anthropic/claude-3.7-sonnet");
			// Google models
			dropdownItems.push_back("google/gemini-2.5-flash");
			dropdownItems.push_back("google/gemini-2.5-pro");
			// OpenAI models
			//			dropdownItems.push_back("openai/gpt-5");
			dropdownItems.push_back("openai/gpt-5-mini");
			dropdownItems.push_back("openai/gpt-5-nano");
			// xAI models
			dropdownItems.push_back("x-ai/grok-3");
			dropdownItems.push_back("x-ai/grok-4");
			// DeepSeek & Qwen models
			dropdownItems.push_back("deepseek/deepseek-chat-v3-0324");
			dropdownItems.push_back("qwen/qwen3-coder");
			// Other models
			dropdownItems.push_back("meta-llama/llama-3.3-70b-instruct");

			// Create display versions for placeholders (show right part after slash)
			displayItems.push_back("claude-sonnet-4");
			displayItems.push_back("claude-3.5-haiku");
			displayItems.push_back("claude-3.7-sonnet");

			displayItems.push_back("gemini-2.5-flash");
			displayItems.push_back("gemini-2.5-pro");

			// displayItems.push_back("gpt-5");
			displayItems.push_back("gpt-5-mini");
			displayItems.push_back("gpt-5-nano");

			displayItems.push_back("grok-3");
			displayItems.push_back("grok-4");

			displayItems.push_back("deepseek-chat-v3-0324");
			displayItems.push_back("qwen3-coder");

			displayItems.push_back("llama-3.3-70b-instruct");

			// Reset selection to first item (current model)
			selectedItem = 0;
			dropdownInitialized = true;

			// Debug: Print all available models
			std::cout << "=== AVAILABLE MODELS ===" << std::endl;
			for (size_t i = 0; i < displayItems.size(); ++i)
			{
				std::cout << i << ": " << displayItems[i] << " (" << dropdownItems[i]
						  << ")" << std::endl;
			}
			std::cout << "=== END MODELS ===" << std::endl;
		}

		// Use the large font for the title
		if (largeFont)
		{
			if (largeFont)
				ImGui::PushFont(largeFont);
			ImVec2 textSize =
				largeFont->CalcTextSizeA(largeFont->LegacySize, FLT_MAX, 0.0f, "Agent");
			ImVec2 centerPos = ImVec2((windowSize.x - textSize.x) * 0.5f,
									  (windowSize.y - textSize.y) * 0.5f - 30.0f);
			ImGui::SetCursorPos(centerPos);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "Agent");
			if (largeFont)
				ImGui::PopFont();

			// Calculate the maximum width needed for all dropdown items
			float maxItemWidth = 0.0f;
			for (const auto &item : displayItems)
			{
				ImVec2 itemSize = largeFont->CalcTextSizeA(
					largeFont->LegacySize, FLT_MAX, 0.0f, item.c_str());
				maxItemWidth = std::max(maxItemWidth, itemSize.x);
			}

			// Add padding for the dropdown button (arrow + some spacing)
			float dropdownPadding = 40.0f; // Space for dropdown arrow and padding
			float dropdownWidth = maxItemWidth + dropdownPadding;

			// Ensure minimum width and maximum width constraints
			dropdownWidth = std::max(dropdownWidth, 180.0f); // Minimum width
			dropdownWidth = std::min(dropdownWidth,
									 windowSize.x * 0.8f); // Maximum 80% of window width

			ImVec2 dropdownSize(dropdownWidth, 0.0f);
			ImVec2 dropdownPos = ImVec2((windowSize.x - dropdownSize.x) * 0.5f,
										centerPos.y + textSize.y + 20.0f);
			ImGui::SetCursorPos(dropdownPos);
			std::vector<const char *> items;
			for (const auto &item : displayItems)
				items.push_back(item.c_str());
			ImGui::SetNextItemWidth(dropdownSize.x);
			int previousSelectedItem = selectedItem;
			if (ImGui::Combo("##AgentDropdown", &selectedItem, items.data(), items.size()))
			{
				if (selectedItem >= 0 && selectedItem < (int)dropdownItems.size())
				{
					std::string newModel =
						dropdownItems[selectedItem]; // Use full path from
													 // dropdownItems
					gSettings.getSettings()["agent_model"] = newModel;
					gSettings.saveSettings();
					std::cout << "Agent model changed to: " << newModel << std::endl;
				}
			}
		} else
		{
			ImFont *currentFont = ImGui::GetFont();
			if (currentFont)
				ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(2.0f);
			ImVec2 textSize = ImGui::CalcTextSize("Agent");
			ImVec2 centerPos = ImVec2((windowSize.x - textSize.x) * 0.5f,
									  (windowSize.y - textSize.y) * 0.5f - 30.0f);
			ImGui::SetCursorPos(centerPos);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "Agent");

			// Calculate the maximum width needed for all dropdown items
			float maxItemWidth = 0.0f;
			for (const auto &item : displayItems)
			{
				ImVec2 itemSize = ImGui::CalcTextSize(item.c_str());
				maxItemWidth = std::max(maxItemWidth, itemSize.x);
			}

			// Add padding for the dropdown button (arrow + some spacing)
			float dropdownPadding = 40.0f; // Space for dropdown arrow and padding
			float dropdownWidth = maxItemWidth + dropdownPadding;

			// Ensure minimum width and maximum width constraints
			dropdownWidth = std::max(dropdownWidth, 120.0f); // Minimum width
			dropdownWidth = std::min(dropdownWidth,
									 windowSize.x * 0.8f); // Maximum 80% of window width

			ImVec2 dropdownSize(dropdownWidth, 0.0f);
			ImVec2 dropdownPos = ImVec2((windowSize.x - dropdownSize.x) * 0.5f,
										centerPos.y + textSize.y + 20.0f);
			ImGui::SetCursorPos(dropdownPos);
			std::vector<const char *> items;
			for (const auto &item : displayItems)
				items.push_back(item.c_str());
			ImGui::SetNextItemWidth(dropdownSize.x);
			int previousSelectedItem = selectedItem;
			if (ImGui::Combo("##AgentDropdown", &selectedItem, items.data(), items.size()))
			{
				if (selectedItem >= 0 && selectedItem < (int)dropdownItems.size())
				{
					std::string newModel =
						dropdownItems[selectedItem]; // Use full path from
													 // dropdownItems
					gSettings.getSettings()["agent_model"] = newModel;
					gSettings.saveSettings();
					std::cout << "Agent model changed to: " << newModel << std::endl;
				}
			}
			ImGui::SetWindowFontScale(1.0f);
			if (currentFont)
				ImGui::PopFont();
		}
	} else
	{
		// Render messages as usual
		for (size_t i = 0; i < messageDisplayLines.size(); ++i)
		{
			ImGui::TextWrapped("%s", messageDisplayLines[i].c_str());
		}
	}

	// Let TextSelect handle selection and copy
	textSelect.update();

	// Scroll to bottom if the flag is set
	if (scrollToBottom)
	{
		ImGui::SetScrollHereY(1.0f);
		scrollToBottom = false;
	}

	ImGui::EndChild();

	// Restore styles
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(3);
}

void AIAgent::printAllMessages()
{
	// Function kept for compatibility but no longer prints anything
}

void AIAgent::sendMessage(const char *msg, bool hide_message)
{
	userScrolledUp = false;
	scrollToBottom = true;
	forceScrollToBottomNextFrame = true;
	std::string api_key = gSettingsFileManager.getOpenRouterKey();
	if (api_key.empty())
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		Message errorMsg;
		errorMsg.text = "Error: No OpenRouter API key configured. Please set "
						"your API key in Settings.";
		errorMsg.role = "assistant";
		errorMsg.isStreaming = false;
		errorMsg.hide_message = false;
		errorMsg.timestamp = std::chrono::system_clock::now();
		messages.push_back(errorMsg);
		hasApiKeyError = true;
		std::cout << "hasApiKeyError: " << hasApiKeyError << std::endl;
		messageDisplayLinesDirty = true;
		scrollToBottom = true;
		return;
	}
	stopStreaming();
	if (agentRequest.isProcessing())
	{
		return;
	}
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		Message userMsg;
		userMsg.text = msg ? msg : "";
		userMsg.role = "user";
		userMsg.isStreaming = false;
		userMsg.hide_message = hide_message;
		userMsg.timestamp = std::chrono::system_clock::now();
		messages.push_back(userMsg);
		messageDisplayLinesDirty = true;
	}
	historyManager.saveConversationHistory();
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		Message agentMsg;
		agentMsg.text = "";
		agentMsg.role = "assistant";
		agentMsg.isStreaming = true;
		agentMsg.hide_message = false;
		agentMsg.timestamp = std::chrono::system_clock::now();
		messages.push_back(agentMsg);
		messageDisplayLinesDirty = true;
	}
	scrollToBottom = true;
	json payload;
	payload["model"] = gSettings.getAgentModel();
	payload["temperature"] = 0.7;
	payload["max_tokens"] = 2000;
	json messagesJson = json::array();
	json systemMessage;
	systemMessage["role"] = "system";

	// Build system prompt with current context
	std::string systemPrompt =
		"You are a helpful AI assistant with access to file system and "
		"terminal tools. Use these "
		"tools when they would help accomplish the user's request.\n\n";

	// Add current project directory if available
	if (!gFileExplorer.selectedFolder.empty())
	{
		systemPrompt +=
			"Current project directory: " + gFileExplorer.selectedFolder + "\n";
	}

	// Add current open file if available
	if (!gFileExplorer.currentOpenFile.empty())
	{
		systemPrompt += "Current open file: " + gFileExplorer.currentOpenFile + "\n";
	}

	systemPrompt += "\n";

	systemMessage["content"] = systemPrompt;
	messagesJson.push_back(systemMessage);
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		for (const auto &msg : messages)
		{
			if (!msg.hide_message)
			{
				json messageObj;
				messageObj["role"] = msg.role;
				if (msg.role == "tool")
				{
					messageObj["content"] = msg.text;
					if (!msg.tool_call_id.empty())
					{
						messageObj["tool_call_id"] = msg.tool_call_id;
					}
				} else if (!msg.tool_calls.is_null())
				{
					if (msg.text.empty())
					{
						messageObj["content"] = "";
					} else
					{
						messageObj["content"] = msg.text;
					}
					messageObj["tool_calls"] = msg.tool_calls;
				} else
				{
					// For regular messages without tool calls, handle empty
					// content properly
					if (msg.text.empty())
					{
						messageObj["content"] = "";
					} else
					{
						messageObj["content"] = msg.text;
					}
				}
				messagesJson.push_back(messageObj);
			}
		}
	}
	payload["messages"] = messagesJson;
	json toolsJson = json::array();
	std::vector<MCP::ToolDefinition> tools = gMCPManager.getToolDefinitions();
	for (const auto &tool : tools)
	{
		json toolObj;
		toolObj["type"] = "function";
		json functionObj;
		functionObj["name"] = tool.name;
		functionObj["description"] = tool.description;
		json properties = json::object();
		json required = json::array();
		for (const auto &param : tool.parameters)
		{
			json paramObj;
			paramObj["type"] = param.type;
			paramObj["description"] = param.description;
			properties[param.name] = paramObj;
			if (param.required)
			{
				required.push_back(param.name);
			}
		}
		json parametersObj;
		parametersObj["type"] = "object";
		parametersObj["properties"] = properties;
		parametersObj["required"] = required;
		functionObj["parameters"] = parametersObj;
		toolObj["function"] = functionObj;
		toolsJson.push_back(toolObj);
	}
	payload["tools"] = toolsJson;
	payload["tool_choice"] = "auto";
	// std::cout << "DEBUG: Sending modern API payload:" << std::endl;
	// std::cout << payload.dump(2) << std::endl;
	std::string payloadStr = payload.dump();
	agentRequest.sendMessage(
		payloadStr,
		api_key,
		[this](const std::string &token) {
			std::lock_guard<std::mutex> lock(messagesMutex);
			if (!messages.empty() && messages.back().isStreaming)
			{
				messages.back().text += token;
				messageDisplayLinesDirty = true;
			}
			scrollToBottom = true;
		},
		[this](const std::string &finalResult, bool hadToolCall) {
			std::cout << "=== COMPLETION CALLBACK: STARTED ===" << std::endl;
			std::cout << "Final result length: " << finalResult.length() << " bytes"
					  << std::endl;
			std::cout << "Had tool call: " << (hadToolCall ? "YES" : "NO") << std::endl;
			std::cout << "Final result preview: " << finalResult.substr(0, 100) << "..."
					  << std::endl;

			// Check if this is an error message and handle it appropriately
			if (finalResult.find("Error: ") == 0)
			{
				std::cout << "=== ERROR HANDLING: STARTED ===" << std::endl;
				std::cout << "Processing error message: " << finalResult << std::endl;
				std::cout << "Messages count: " << messages.size() << std::endl;

				// Update the assistant message with the error
				{
					std::lock_guard<std::mutex> lock(messagesMutex);
					if (!messages.empty() && messages.back().isStreaming)
					{
						std::cout << "Found streaming message, updating with error..."
								  << std::endl;
						messages.back().text = finalResult;
						messages.back().isStreaming = false;
						messages.back().role = "assistant";
						messageDisplayLinesDirty = true;
						std::cout
							<< "Updated assistant message with error: " << finalResult
							<< std::endl;
						std::cout << "Message updated successfully!" << std::endl;
					} else
					{
						std::cout
							<< "WARNING: No streaming message found to update with error!"
							<< std::endl;
						if (!messages.empty())
						{
							std::cout << "Last message role: " << messages.back().role
									  << std::endl;
							std::cout << "Last message streaming: "
									  << (messages.back().isStreaming ? "YES" : "NO")
									  << std::endl;
						}

						// Create a new assistant message with the error
						std::cout << "Creating new assistant message with error..."
								  << std::endl;
						Message errorMsg;
						errorMsg.text = finalResult;
						errorMsg.role = "assistant";
						errorMsg.isStreaming = false;
						errorMsg.hide_message = false;
						errorMsg.timestamp = std::chrono::system_clock::now();
						messages.push_back(errorMsg);

						// Check if this is an API key error and set the flag
						if (finalResult.find("Invalid or expired API key") !=
								std::string::npos ||
							finalResult.find("No OpenRouter API key configured") !=
								std::string::npos)
						{
							hasApiKeyError = true;
							std::cout << "Set hasApiKeyError to true for API key error"
									  << std::endl;
						}

						messageDisplayLinesDirty = true;
						std::cout << "Created new error message successfully!"
								  << std::endl;
					}
				}

				scrollToBottom = true;
				historyManager.saveConversationHistory();
				std::cout << "=== COMPLETION CALLBACK: FINISHED (ERROR) ===" << std::endl;
				return;
			} else
			{
				std::cout << "Not an error message, continuing with normal processing..."
						  << std::endl;
			}

			{
				std::lock_guard<std::mutex> lock(messagesMutex);
				std::cout << "Messages count: " << messages.size() << std::endl;
				if (!messages.empty())
				{
					std::cout << "Last message role: " << messages.back().role
							  << std::endl;
					std::cout << "Last message streaming: "
							  << (messages.back().isStreaming ? "YES" : "NO")
							  << std::endl;
				}

				if (!messages.empty() && messages.back().isStreaming)
				{
					if (hadToolCall)
					{
						std::cout << "Processing tool call completion..." << std::endl;
						// For tool calls, the assistant message should already
						// be updated with tool_calls and the tool results
						// should be added as separate tool messages The
						// assistant message should remain as "assistant" role
						if (messages.back().role == "assistant")
						{
							messages.back().isStreaming = false;
							// The tool_calls should already be set by
							// agent_request.cpp
							std::cout << "Updated assistant message with tool calls"
									  << std::endl;
						}
						messageDisplayLinesDirty = true;

						// Set flag to trigger automatic follow-up after tool calls
						needsFollowUpMessage = true;
						std::cout << "Set needsFollowUpMessage flag" << std::endl;
					} else
					{
						std::cout << "Processing regular assistant message "
									 "completion..."
								  << std::endl;
						// Regular assistant message
						messages.back().text = finalResult;
						messages.back().isStreaming = false;
						messages.back().role = "assistant";
						messageDisplayLinesDirty = true;
						std::cout << "Updated assistant message with final result"
								  << std::endl;
					}
				} else
				{
					std::cout << "WARNING: No streaming message found to update"
							  << std::endl;
				}
				scrollToBottom = true;
			}
			historyManager.saveConversationHistory();
			std::cout << "Saved conversation history" << std::endl;

			// If we had tool calls, trigger the follow-up response
			if (hadToolCall)
			{
				std::cout << "Triggering follow-up response for tool calls..."
						  << std::endl;
				// Use a small delay to ensure the current request is fully processed
				std::thread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					if (needsFollowUpMessage && !agentRequest.isProcessing())
					{
						std::cout << "Executing follow-up message..." << std::endl;
						needsFollowUpMessage = false;

						try
						{
							// Add a hidden system message to prompt the AI to continue
							std::string followUpPrompt =
								"Please continue with the conversation based on "
								"the tool results. You "
								"can make additional tool calls if needed to "
								"complete the user's "
								"request.";

							// Add the follow-up prompt as a hidden system message
							// to the conversation
							{
								std::lock_guard<std::mutex> lock(messagesMutex);
								Message followUpMsg;
								followUpMsg.text = followUpPrompt;
								followUpMsg.role = "system";
								followUpMsg.isStreaming = false;
								followUpMsg.hide_message = true;
								followUpMsg.timestamp = std::chrono::system_clock::now();
								messages.push_back(followUpMsg);
								messageDisplayLinesDirty = true;
								std::cout << "Added hidden system message for follow-up"
										  << std::endl;
							}

							// Trigger the AI response directly
							triggerAIResponse();
							std::cout << "Triggered AI response" << std::endl;
						} catch (const std::exception &e)
						{
							std::cout
								<< "ERROR in follow-up message processing: " << e.what()
								<< std::endl;
							needsFollowUpMessage = false;
						}
					} else
					{
						if (!needsFollowUpMessage)
						{
							std::cout << "Follow-up message flag was cleared, skipping"
									  << std::endl;
						} else
						{
							std::cout << "Agent is still processing, skipping follow-up"
									  << std::endl;
						}
					}
				}).detach();
			} else
			{
				std::cout << "No tool calls, no follow-up needed" << std::endl;
			}

			std::cout << "=== COMPLETION CALLBACK: FINISHED ===" << std::endl;
		});
}

void AIAgent::loadConversationFromHistory(
	const std::vector<std::string> &formattedMessages, const std::string &timestamp)
{
	std::lock_guard<std::mutex> lock(messagesMutex);

	// Store the timestamp of the loaded conversation
	currentConversationTimestamp = timestamp;

	// Clear current messages
	messages.clear();

	// Reset conversation state variables to prevent issues when switching
	// conversations
	needsFollowUpMessage = false;
	failedToolCalls.clear();
	lastToolCallTime = std::chrono::system_clock::now();

	// Parse formatted messages and add them to the current conversation
	for (const auto &formattedMsg : formattedMessages)
	{
		Message msg;

		// Check if it's an agent, user, or tool message and set the role correctly
		if (formattedMsg.find("##### Agent: ") == 0)
		{
			msg.text = formattedMsg.substr(12); // Remove "##### Agent: " prefix
			msg.role = "assistant";
		} else if (formattedMsg.find("##### User: ") == 0)
		{
			msg.text = formattedMsg.substr(11); // Remove "##### User: " prefix
			msg.role = "user";
		} else if (formattedMsg.find("##### Tool Result: ") == 0)
		{
			msg.text = formattedMsg.substr(18); // Remove "##### Tool Result: " prefix
			msg.role = "tool";
		} else
		{
			// Fallback: treat as user message
			msg.text = formattedMsg;
			msg.role = "user";
		}

		msg.isStreaming = false;
		msg.hide_message = false;
		msg.timestamp = std::chrono::system_clock::now();

		messages.push_back(msg);
	}

	// Update display
	messageDisplayLinesDirty = true;
	scrollToBottom = true;

	std::cout << "Loaded " << messages.size() << " messages from conversation history"
			  << std::endl;
}

void AIAgent::triggerAIResponse()
{
	std::cout << "=== TRIGGER AI RESPONSE: STARTED ===" << std::endl;

	std::string api_key = gSettingsFileManager.getOpenRouterKey();
	if (api_key.empty())
	{
		std::cout << "ERROR: No API key available for follow-up response" << std::endl;
		std::lock_guard<std::mutex> lock(messagesMutex);
		Message errorMsg;
		errorMsg.text = "Error: No OpenRouter API key configured. Please set "
						"your API key in Settings.";
		errorMsg.role = "assistant";
		errorMsg.isStreaming = false;
		errorMsg.hide_message = false;
		errorMsg.timestamp = std::chrono::system_clock::now();
		messages.push_back(errorMsg);
		hasApiKeyError = true;
		std::cout << "hasApiKeyError: " << hasApiKeyError << std::endl;
		messageDisplayLinesDirty = true;
		scrollToBottom = true;
		return;
	}

	std::cout << "API key available, length: " << api_key.length() << std::endl;

	stopStreaming();
	if (agentRequest.isProcessing())
	{
		std::cout << "WARNING: Agent request already processing, aborting follow-up"
				  << std::endl;
		return;
	}

	std::cout << "Adding assistant message for streaming..." << std::endl;
	// Add assistant message for streaming
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		Message agentMsg;
		agentMsg.text = "";
		agentMsg.role = "assistant";
		agentMsg.isStreaming = true;
		agentMsg.hide_message = false;
		agentMsg.timestamp = std::chrono::system_clock::now();
		messages.push_back(agentMsg);
		messageDisplayLinesDirty = true;
		std::cout << "Added streaming assistant message, total messages: "
				  << messages.size() << std::endl;
	}

	scrollToBottom = true;

	std::cout << "Building payload for follow-up response..." << std::endl;
	// Build the payload (same logic as sendMessage but without adding a user
	// message)
	json payload;
	payload["model"] = gSettings.getAgentModel();
	payload["temperature"] = 0.7;
	payload["max_tokens"] = 2000;
	json messagesJson = json::array();
	json systemMessage;
	systemMessage["role"] = "system";

	// Build system prompt with current context
	std::string systemPrompt =
		"You are a helpful AI assistant with access to file system and "
		"terminal tools. Use these "
		"tools when they would help accomplish the user's request.\n\n";

	// Add current project directory if available
	if (!gFileExplorer.selectedFolder.empty())
	{
		systemPrompt +=
			"Current project directory: " + gFileExplorer.selectedFolder + "\n";
	}

	// Add current open file if available
	if (!gFileExplorer.currentOpenFile.empty())
	{
		systemPrompt += "Current open file: " + gFileExplorer.currentOpenFile + "\n";
	}

	systemPrompt += "\n";

	systemMessage["content"] = systemPrompt;
	messagesJson.push_back(systemMessage);

	// Add all messages including hidden ones for context
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		std::cout << "Adding " << messages.size() << " messages to payload" << std::endl;

		// Process messages in pairs to ensure tool results immediately follow
		// tool calls
		for (size_t i = 0; i < messages.size(); ++i)
		{
			const auto &msg = messages[i];
			json messageObj;
			messageObj["role"] = msg.role;

			// If this is the last message and an assistant, trim trailing
			// whitespace from content
			bool isLastMessage = (i == messages.size() - 1);
			if (msg.role == "assistant" && isLastMessage)
			{
				std::string trimmed = msg.text;
				// Remove trailing whitespace safely
				size_t lastNonWhitespace = trimmed.find_last_not_of(" \t\n\r");
				if (lastNonWhitespace != std::string::npos)
				{
					trimmed.erase(lastNonWhitespace + 1);
				} else
				{
					// String is all whitespace, make it empty
					trimmed.clear();
				}
				// If the content is empty after trimming, skip this message
				// entirely to avoid the "final assistant content cannot end
				// with trailing whitespace" error
				if (trimmed.empty() && msg.tool_calls.is_null())
				{
					std::cout << "Skipping last assistant message with empty "
								 "content to avoid API error"
							  << std::endl;
					continue;
				} else if (trimmed.empty())
				{
					// If it has tool calls, we need to keep it but use empty
					// string instead of null
					messageObj["content"] = "";
				} else
				{
					messageObj["content"] = trimmed;
				}
				if (!msg.tool_calls.is_null() && msg.tool_calls.is_array() &&
					!msg.tool_calls.empty())
				{
					messageObj["tool_calls"] = msg.tool_calls;
				}
			} else if (msg.role == "tool")
			{
				messageObj["content"] = msg.text;
				if (!msg.tool_call_id.empty())
				{
					messageObj["tool_call_id"] = msg.tool_call_id;
				}
			} else if (!msg.tool_calls.is_null() && msg.tool_calls.is_array() &&
					   !msg.tool_calls.empty())
			{
				// This is an assistant message with tool calls
				if (msg.text.empty())
				{
					messageObj["content"] = "";
				} else
				{
					messageObj["content"] = msg.text;
				}
				messageObj["tool_calls"] = msg.tool_calls;
			} else
			{
				messageObj["content"] = msg.text;
			}
			messagesJson.push_back(messageObj);

			// If this message has tool calls, ensure the next message is the
			// corresponding tool result
			if (!msg.tool_calls.is_null() && msg.tool_calls.is_array() &&
				!msg.tool_calls.empty())
			{
				// Check if the next message is a tool result for these tool calls
				if (i + 1 < messages.size())
				{
					const auto &nextMsg = messages[i + 1];
					if (nextMsg.role == "tool" && !nextMsg.tool_call_id.empty())
					{
						// Verify this tool result corresponds to one of the
						// tool calls
						bool foundMatchingToolCall = false;
						for (const auto &toolCall : msg.tool_calls)
						{
							if (toolCall.contains("id") && !toolCall["id"].is_null() &&
								toolCall["id"].get<std::string>() == nextMsg.tool_call_id)
							{
								foundMatchingToolCall = true;
								break;
							}
						}

						if (!foundMatchingToolCall)
						{
							std::cout << "WARNING: Tool result message doesn't "
										 "match any tool call ID"
									  << std::endl;
							std::cout << "Tool call IDs: ";
							for (const auto &toolCall : msg.tool_calls)
							{
								if (toolCall.contains("id") && !toolCall["id"].is_null())
								{
									std::cout << toolCall["id"].get<std::string>() << " ";
								}
							}
							std::cout << std::endl;
							std::cout << "Tool result ID: " << nextMsg.tool_call_id
									  << std::endl;
						}
					}
				}
			}
		}
	}

	payload["messages"] = messagesJson;
	json toolsJson = json::array();
	std::vector<MCP::ToolDefinition> tools = gMCPManager.getToolDefinitions();
	std::cout << "Adding " << tools.size() << " tools to payload" << std::endl;
	for (const auto &tool : tools)
	{
		json toolObj;
		toolObj["type"] = "function";
		json functionObj;
		functionObj["name"] = tool.name;
		functionObj["description"] = tool.description;
		json properties = json::object();
		json required = json::array();
		for (const auto &param : tool.parameters)
		{
			json paramObj;
			paramObj["type"] = param.type;
			paramObj["description"] = param.description;
			properties[param.name] = paramObj;
			if (param.required)
			{
				required.push_back(param.name);
			}
		}
		json parametersObj;
		parametersObj["type"] = "object";
		parametersObj["properties"] = properties;
		parametersObj["required"] = required;
		functionObj["parameters"] = parametersObj;
		toolObj["function"] = functionObj;
		toolsJson.push_back(toolObj);
	}
	payload["tools"] = toolsJson;
	payload["tool_choice"] = "auto";

	std::string payloadStr = payload.dump();
	std::cout << "Payload prepared, length: " << payloadStr.length() << " bytes"
			  << std::endl;

	std::cout << "Sending follow-up request..." << std::endl;
	agentRequest.sendMessage(
		payloadStr,
		api_key,
		[this](const std::string &token) {
			std::lock_guard<std::mutex> lock(messagesMutex);
			if (!messages.empty() && messages.back().isStreaming)
			{
				messages.back().text += token;
				messageDisplayLinesDirty = true;
			}
			scrollToBottom = true;
		},
		[this](const std::string &finalResult, bool hadToolCall) {
			std::cout << "=== FOLLOW-UP COMPLETION CALLBACK: STARTED ===" << std::endl;
			std::cout << "Final result length: " << finalResult.length() << " bytes"
					  << std::endl;
			std::cout << "Had tool call: " << (hadToolCall ? "YES" : "NO") << std::endl;

			{
				std::lock_guard<std::mutex> lock(messagesMutex);
				if (!messages.empty() && messages.back().isStreaming)
				{
					if (hadToolCall)
					{
						std::cout << "Follow-up had tool calls, updating message"
								  << std::endl;
						// For tool calls, the assistant message should already
						// be updated with tool_calls
						if (messages.back().role == "assistant")
						{
							messages.back().isStreaming = false;
						}
						messageDisplayLinesDirty = true;

						// Set flag to trigger automatic follow-up after tool calls
						needsFollowUpMessage = true;
						std::cout << "Set needsFollowUpMessage flag for "
									 "follow-up tool calls"
								  << std::endl;
					} else
					{
						std::cout << "Follow-up completed with regular response"
								  << std::endl;
						// Regular assistant message
						messages.back().text = finalResult;
						messages.back().isStreaming = false;
						messages.back().role = "assistant";
						messageDisplayLinesDirty = true;
					}
				}
				scrollToBottom = true;
			}
			historyManager.saveConversationHistory();
			std::cout << "=== FOLLOW-UP COMPLETION CALLBACK: FINISHED ===" << std::endl;

			// If we had tool calls in the follow-up, trigger another follow-up
			// response
			if (hadToolCall)
			{
				std::cout << "Triggering follow-up response for follow-up tool "
							 "calls..."
						  << std::endl;
				// Use a small delay to ensure the current request is fully processed
				std::thread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					if (needsFollowUpMessage)
					{
						std::cout << "Executing follow-up message for "
									 "follow-up tool calls..."
								  << std::endl;
						needsFollowUpMessage = false;
						// Add a hidden system message to prompt the AI to continue
						std::string followUpPrompt =
							"Please continue with the conversation based on "
							"the tool results. You "
							"can make additional tool calls if needed to "
							"complete the user's "
							"request.";

						// Add the follow-up prompt as a hidden system message
						// to the conversation
						{
							std::lock_guard<std::mutex> lock(messagesMutex);
							Message followUpMsg;
							followUpMsg.text = followUpPrompt;
							followUpMsg.role = "system";
							followUpMsg.isStreaming = false;
							followUpMsg.hide_message = true;
							followUpMsg.timestamp = std::chrono::system_clock::now();
							messages.push_back(followUpMsg);
							messageDisplayLinesDirty = true;
							std::cout << "Added hidden system message for "
										 "follow-up tool calls"
									  << std::endl;
						}

						// Trigger the AI response directly
						triggerAIResponse();
						std::cout << "Triggered AI response for follow-up tool calls"
								  << std::endl;
					} else
					{
						std::cout << "Follow-up message flag was cleared, skipping"
								  << std::endl;
					}
				}).detach();
			} else
			{
				std::cout << "No tool calls in follow-up, no further follow-up needed"
						  << std::endl;
			}
		});

	std::cout << "=== TRIGGER AI RESPONSE: FINISHED ===" << std::endl;
}

void AIAgent::renderOpenRouterKeyInput(float textBoxWidth, float horizontalPadding)
{
	// Only render if we have API key errors
	if (!hasApiKeyError)
	{
		return;
	}

	// Static variables for the input
	static char openRouterKeyBuffer[128] = "";
	static bool showOpenRouterKey = false;
	static bool initialized = false;
	static bool keyChanged = false;

	// Initialize the buffer
	if (!initialized)
	{
		openRouterKeyBuffer[0] = '\0';
		initialized = true;
	}

	// Render the key input section
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
	ImGui::TextWrapped("⚠️ OpenRouter Key");
	ImGui::PopStyleColor();
	ImGui::Spacing();

	// Buttons above the input (styled like agent input buttons)
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

	// Safe access to backgroundColor with null checks
	auto &bgColor = gSettings.getSettings()["backgroundColor"];
	float bgR = (bgColor.is_array() && bgColor.size() > 0 && !bgColor[0].is_null())
					? bgColor[0].get<float>()
					: 0.1f;
	float bgG = (bgColor.is_array() && bgColor.size() > 1 && !bgColor[1].is_null())
					? bgColor[1].get<float>()
					: 0.1f;
	float bgB = (bgColor.is_array() && bgColor.size() > 2 && !bgColor[2].is_null())
					? bgColor[2].get<float>()
					: 0.1f;

	ImGui::PushStyleColor(ImGuiCol_Button,
						  ImVec4(bgR * 0.8f, bgG * 0.8f, bgB * 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
						  ImVec4(bgR * 0.95f, bgG * 0.95f, bgB * 0.95f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,
						  ImVec4(bgR * 0.7f, bgG * 0.7f, bgB * 0.7f, 1.0f));

	// Save button
	ImVec2 saveTextSize = ImGui::CalcTextSize("Save");
	ImVec2 saveButtonSize = ImVec2(saveTextSize.x + 16.0f, 0);
	if (ImGui::Button("Save##agent_key", saveButtonSize) && keyChanged)
	{
		gSettingsFileManager.setOpenRouterKey(std::string(openRouterKeyBuffer));
		gAITab.load_key();
		gSettings.renderNotification("OpenRouter key saved!", 2.0f);
		keyChanged = false;

		// Clear the buffer after saving
		openRouterKeyBuffer[0] = '\0';

		// Clear the API key error flag so the input disappears
		hasApiKeyError = false;

		// Mark display as dirty to refresh the error message display
		messageDisplayLinesDirty = true;
	}
	ImGui::SameLine();

	// Show/Hide button
	ImVec2 showTextSize = ImGui::CalcTextSize(showOpenRouterKey ? "Hide" : "Show");
	ImVec2 showButtonSize = ImVec2(showTextSize.x + 16.0f, 0);
	if (ImGui::Button(showOpenRouterKey ? "Hide" : "Show", showButtonSize))
	{
		showOpenRouterKey = !showOpenRouterKey;
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);

	ImGui::Spacing();

	// OpenRouter Key Input - styled like agent input below
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

	// Safe access to backgroundColor with null checks
	auto &bgColor2 = gSettings.getSettings()["backgroundColor"];
	float bgR2 = (bgColor2.is_array() && bgColor2.size() > 0 && !bgColor2[0].is_null())
					 ? bgColor2[0].get<float>()
					 : 0.1f;
	float bgG2 = (bgColor2.is_array() && bgColor2.size() > 1 && !bgColor2[1].is_null())
					 ? bgColor2[1].get<float>()
					 : 0.1f;
	float bgB2 = (bgColor2.is_array() && bgColor2.size() > 2 && !bgColor2[2].is_null())
					 ? bgColor2[1].get<float>()
					 : 0.1f;

	ImGui::PushStyleColor(ImGuiCol_FrameBg,
						  ImVec4(bgR2 * 0.8f, bgG2 * 0.8f, bgB2 * 0.8f, 1.0f));

	// Set width to match agent input (responsive to pane size)
	ImGui::SetNextItemWidth(textBoxWidth - 2 * horizontalPadding);

	ImGuiInputTextFlags flags = showOpenRouterKey ? 0 : ImGuiInputTextFlags_Password;
	bool inputChanged = ImGui::InputTextWithHint("##openrouterkey_agent",
												 "API key",
												 openRouterKeyBuffer,
												 sizeof(openRouterKeyBuffer),
												 flags);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);

	// Check focus state immediately after the input widget
	bool isInputActive = ImGui::IsItemActive();

	if (inputChanged)
	{
		keyChanged = true;
	}

	ImGui::Spacing();

	// Handle input blocking logic for the key input
	static bool wasInputActive = false;
	if (isInputActive != wasInputActive)
	{
		editor_state.block_input = isInputActive;
		wasInputActive = isInputActive;
	}
	if (isInputActive)
	{
		editor_state.block_input = true;
	}
}
