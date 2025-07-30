// ai_agent_history.cpp
#include "ai_agent_history.h"
#include "../files/files.h"
#include "../lib/json.hpp"
#include "../util/settings.h"
#include "ai_agent.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

AIAgentHistory::AIAgentHistory()
	: display_agent_history(false), showHistoryWindow(false), selectedIndex(0),
	  hasPendingSelection(false), isInitialSelection(true),
	  currentConversationTimestamp(nullptr)
{
}

AIAgentHistory::~AIAgentHistory() {}

void AIAgentHistory::renderHistory()
{
	if (display_agent_history)
	{
		std::cout << "Hello World from AIAgentHistory::renderHistory()" << std::endl;
		// TODO: Implement actual history rendering
	}

	// Handle ESC key to close window
	if (showHistoryWindow && ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		toggleWindow();
		return;
	}

	if (!showHistoryWindow)
		return;

	// Render header (window setup and title)
	renderHeader();

	// Render the conversation list
	renderConversationList();

	ImGui::Separator();

	// Add Clear button at the bottom
	ImGui::Spacing();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100); // Position button on the right

	// Style the Clear button to match Send/History buttons
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 4)); // Even smaller padding
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

	if (ImGui::Button("Clear", ImVec2(80, 0)))
	{
		// Clear all conversation history
		if (gFileExplorer.selectedFolder.empty())
		{
			return;
		}

		fs::path historyPath =
			fs::path(gFileExplorer.selectedFolder) / ".ned-agent-history.json";

		try
		{
			// Create empty JSON structure
			json emptyHistory;
			emptyHistory["conversations"] = json::array();

			// Save empty history to file
			std::ofstream file(historyPath);
			if (file.is_open())
			{
				file << emptyHistory.dump(4);
				std::cout << "Conversation history cleared" << std::endl;

				// Clear the local lists
				{
					std::lock_guard<std::mutex> lock(conversationListMutex);
					conversationList.clear();
					filteredList.clear();
				}

				// Show notification
				gSettings.renderNotification("History cleared", 2.0f);

				// Close the window
				toggleWindow();
			} else
			{
				std::cerr << "Failed to clear conversation history" << std::endl;
			}
		} catch (const std::exception &e)
		{
			std::cerr << "Error clearing conversation history: " << e.what() << std::endl;
		}
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);

	ImGui::Spacing();
	ImGui::Text("Press ESC to close");

	// Handle click outside window to close (before ImGui::End())
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		if (mousePos.x < windowPos.x || mousePos.x > (windowPos.x + windowSize.x) ||
			mousePos.y < windowPos.y || mousePos.y > (windowPos.y + windowSize.y))
		{
			toggleWindow();
			ImGui::End();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(3);
			return;
		}
	}

	ImGui::End();

	// Pop the window style colors and vars pushed in renderHeader()
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}

void AIAgentHistory::setDisplayHistory(bool display) { display_agent_history = display; }

bool AIAgentHistory::isDisplayingHistory() const { return display_agent_history; }

void AIAgentHistory::toggleWindow()
{
	showHistoryWindow = !showHistoryWindow;

	if (showHistoryWindow)
	{
		selectedIndex = 0;
		isInitialSelection = true;
		refreshConversationList();
	}
}

bool AIAgentHistory::isWindowOpen() const { return showHistoryWindow; }

void AIAgentHistory::refreshConversationList()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return;
	}

	fs::path historyPath =
		fs::path(gFileExplorer.selectedFolder) / ".ned-agent-history.json";
	std::ifstream file(historyPath);

	if (!file.is_open())
	{
		return;
	}

	try
	{
		json root;
		file >> root;

		std::vector<ConversationEntry> newConversationList;

		if (root.contains("conversations") && root["conversations"].is_array())
		{
			for (const auto &conversation : root["conversations"])
			{
				ConversationEntry entry;
				entry.timestamp = conversation.value("timestamp", "");

				if (conversation.contains("messages") &&
					conversation["messages"].is_array() &&
					!conversation["messages"].empty())
				{
					const auto &firstMsg = conversation["messages"][0];
					entry.firstMessage = "";
					entry.messageCount = conversation["messages"].size();
				} else
				{
					entry.firstMessage = "Empty conversation";
					entry.messageCount = 0;
				}

				newConversationList.push_back(entry);
			}
		}

		{
			std::lock_guard<std::mutex> lock(conversationListMutex);
			conversationList = std::move(newConversationList);
			filteredList = conversationList; // For now, show all conversations

			// Reverse the list so most recent conversations appear first
			std::reverse(filteredList.begin(), filteredList.end());
		}

	} catch (const std::exception &e)
	{
		std::cerr << "Error loading conversation history: " << e.what() << std::endl;
	}
}

void AIAgentHistory::renderHeader()
{
	// Window setup (size, position, flags) - similar to file finder
	ImGui::SetNextWindowSize(ImVec2(600, 350), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
								   ImGui::GetIO().DisplaySize.y * 0.35f),
							ImGuiCond_Always,
							ImVec2(0.5f, 0.5f));
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
								   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
								   ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_NoScrollWithMouse;

	// Push window style (3 style vars, 3 style colors) - same as file finder
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

	// Background
	ImGui::PushStyleColor(
		ImGuiCol_WindowBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
			   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(
		ImGuiCol_FrameBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
			   1.0f));

	ImGui::Begin("ConversationHistory", nullptr, windowFlags);

	// Make title bigger but not too big
	ImGui::SetWindowFontScale(1.2f); // Reduced from 1.5f to 1.2f
	ImGui::TextUnformatted("Conversation History");
	ImGui::SetWindowFontScale(1.0f); // Reset font scale

	ImGui::Separator(); // Add horizontal line
}

void AIAgentHistory::renderConversationList()
{
	// Begin a child window for the conversation list
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

	// Add scrollbar styling
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 13.0f);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
						  ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.7f, 0.7f, 0.7f, 0.9f));

	ImGui::BeginChild("ConversationResults",
					  ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
					  false);

	// Push styling for list items
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

	// Hover colors (no selection colors)
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));

	std::vector<ConversationEntry> localFilteredList;
	{
		std::lock_guard<std::mutex> lock(conversationListMutex);
		localFilteredList = filteredList;
	}

	for (int i = 0; i < static_cast<int>(localFilteredList.size()); ++i)
	{
		const ConversationEntry &entry = localFilteredList[i];

		ImGui::PushID(i);

		// Create a selectable item that shows hover effect
		if (ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns))
		{
			// Handle click on conversation item
			std::cout << "Loading conversation: " << entry.timestamp << std::endl;
			loadConversationFromHistory(entry.timestamp);
		}

		ImGui::SameLine();

		// Display conversation info
		ImGui::Text("%s (%d messages)", entry.timestamp.c_str(), entry.messageCount);
		ImGui::SameLine();

		// Truncate first message if too long
		std::string displayText = entry.firstMessage;
		if (displayText.length() > 50)
		{
			displayText = displayText.substr(0, 47) + "...";
		}
		ImGui::TextUnformatted(displayText.c_str());

		ImGui::PopID();
	}

	// Pop the style colors and variables for list items
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	ImGui::EndChild();

	// Pop scrollbar styling
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(2);
}

void AIAgentHistory::handleSelectionChange()
{
	if (!filteredList.empty() && selectedIndex >= 0 &&
		selectedIndex < static_cast<int>(filteredList.size()))
	{

		const ConversationEntry &selectedEntry = filteredList[selectedIndex];

		if (!isInitialSelection)
		{
			// Update timestamp and store pending conversation
			lastSelectionTime = std::chrono::steady_clock::now();
			pendingConversation = selectedEntry.timestamp;
			hasPendingSelection = true;
		}
	}
}

void AIAgentHistory::checkPendingSelection()
{
	if (!hasPendingSelection)
		return;

	auto now = std::chrono::steady_clock::now();
	auto elapsed =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSelectionTime);

	if (elapsed.count() >= 50)
	{
		hasPendingSelection = false;
		// TODO: Load the selected conversation
		std::cout << "Selected conversation: " << pendingConversation << std::endl;
	}
}

void AIAgentHistory::setLoadConversationCallback(LoadConversationCallback callback)
{
	loadCallback = callback;
}

void AIAgentHistory::loadConversationFromHistory(const std::string &timestamp)
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return;
	}

	fs::path historyPath =
		fs::path(gFileExplorer.selectedFolder) / ".ned-agent-history.json";
	std::ifstream file(historyPath);

	if (!file.is_open())
	{
		return;
	}

	try
	{
		json root;
		file >> root;

		if (root.contains("conversations") && root["conversations"].is_array())
		{
			for (const auto &conversation : root["conversations"])
			{
				if (conversation.value("timestamp", "") == timestamp)
				{
					// Found the conversation, extract messages
					std::vector<std::string> messages;

					if (conversation.contains("messages") &&
						conversation["messages"].is_array())
					{
						for (const auto &msg : conversation["messages"])
						{
							std::string messageText = msg.value("text", "");
							std::string role = msg.value("role", "user");

							// Format message for display based on actual role
							std::string formattedMessage;
							if (role == "assistant")
							{
								formattedMessage = "##### Agent: " + messageText;
							} else if (role == "tool")
							{
								formattedMessage = "##### Tool Result: " + messageText;
							} else
							{
								formattedMessage = "##### User: " + messageText;
							}
							messages.push_back(formattedMessage);
						}
					}

					// Call the callback to load the conversation
					if (loadCallback)
					{
						loadCallback(messages, timestamp);
					}

					// Close the history window
					toggleWindow();
					return;
				}
			}
		}

	} catch (const std::exception &e)
	{
		std::cerr << "Error loading conversation from history: " << e.what() << std::endl;
	}
}

void AIAgentHistory::setMessagesCallback(MessagesCallback callback)
{
	messagesCallback = callback;
}

void AIAgentHistory::setSetMessagesCallback(SetMessagesCallback callback)
{
	setMessagesCallbackVar = callback;
}

void AIAgentHistory::setUpdateDisplayCallback(UpdateDisplayCallback callback)
{
	updateDisplayCallback = callback;
}

void AIAgentHistory::setCurrentConversationTimestamp(std::string &timestamp)
{
	currentConversationTimestamp = &timestamp;
}

// Conversation history management methods
void AIAgentHistory::saveConversationHistory()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return; // No project folder selected
	}
	fs::path historyPath =
		fs::path(gFileExplorer.selectedFolder) / ".ned-agent-history.json";
	try
	{
		json root;
		if (!messagesCallback)
		{
			std::cerr << "Messages callback not set" << std::endl;
			return;
		}
		std::vector<Message> messages = messagesCallback();
		if (!messages.empty())
		{
			json existingHistory;
			std::ifstream existingFile(historyPath);
			if (existingFile.is_open())
			{
				try
				{
					existingFile >> existingHistory;
				} catch (const json::parse_error &e)
				{
					std::cerr << "Error parsing existing agent history: " << e.what()
							  << std::endl;
					existingHistory = json::object();
				}
			}
			if (!existingHistory.contains("conversations"))
			{
				existingHistory["conversations"] = json::array();
			}
			bool shouldCreateNewConversation = true;
			int conversationIndexToUpdate = -1;
			if (currentConversationTimestamp && !currentConversationTimestamp->empty())
			{
				for (size_t i = 0; i < existingHistory["conversations"].size(); ++i)
				{
					if (existingHistory["conversations"][i].value("timestamp", "") ==
						*currentConversationTimestamp)
					{
						shouldCreateNewConversation = false;
						conversationIndexToUpdate = i;
						break;
					}
				}
			}
			std::string timestamp;
			if (shouldCreateNewConversation)
			{
				auto now = std::chrono::system_clock::now();
				auto time_t = std::chrono::system_clock::to_time_t(now);
				std::tm tm = *std::localtime(&time_t);
				std::ostringstream oss;
				oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
				timestamp = oss.str();
				if (currentConversationTimestamp)
				{
					*currentConversationTimestamp = timestamp;
				}
			} else
			{
				timestamp = *currentConversationTimestamp;
			}
			json conversation;
			conversation["timestamp"] = timestamp;
			conversation["firstMessage"] = messages[0].text.substr(0, 100);
			conversation["messageCount"] = messages.size();
			json messagesJson = json::array();
			for (const auto &msg : messages)
			{
				json msgJson;
				msgJson["text"] = msg.text;
				msgJson["role"] = msg.role;
				msgJson["isStreaming"] = msg.isStreaming;
				msgJson["hide_message"] = msg.hide_message;
				msgJson["tool_call_id"] = msg.tool_call_id;
				msgJson["tool_calls"] = msg.tool_calls;
				auto time_t = std::chrono::system_clock::to_time_t(msg.timestamp);
				std::tm tm = *std::localtime(&time_t);
				std::ostringstream oss;
				oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
				msgJson["timestamp"] = oss.str();
				messagesJson.push_back(msgJson);
			}
			conversation["messages"] = messagesJson;
			if (shouldCreateNewConversation)
			{
				existingHistory["conversations"].push_back(conversation);
			} else
			{
				existingHistory["conversations"][conversationIndexToUpdate] =
					conversation;
			}
			std::ofstream file(historyPath);
			if (file.is_open())
			{
				file << existingHistory.dump(2);
				file.close();
			}
		}
	} catch (const std::exception &e)
	{
		std::cerr << "Error saving agent conversation history: " << e.what() << std::endl;
	}
}

void AIAgentHistory::loadConversationHistory()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return; // No project folder selected
	}
	fs::path historyPath =
		fs::path(gFileExplorer.selectedFolder) / ".ned-agent-history.json";
	std::ifstream file(historyPath);
	if (!file.is_open())
	{
		return; // No history file exists
	}
	try
	{
		json root;
		file >> root;
		if (root.contains("conversations") && root["conversations"].is_array() &&
			!root["conversations"].empty())
		{
			const auto &conversations = root["conversations"];
			const auto &latestConversation = conversations.back();
			if (latestConversation.contains("messages") &&
				latestConversation["messages"].is_array())
			{
				std::vector<Message> messages;
				for (const auto &msgJson : latestConversation["messages"])
				{
					Message msg;
					msg.text = msgJson.value("text", "");
					msg.role = msgJson.value("role", "user");
					msg.isStreaming = msgJson.value("isStreaming", false);
					msg.hide_message = msgJson.value("hide_message", false);
					msg.tool_call_id = msgJson.value("tool_call_id", "");
					msg.tool_calls = msgJson.value("tool_calls", json());
					std::string timestampStr = msgJson.value("timestamp", "");
					if (!timestampStr.empty())
					{
						std::tm tm = {};
						std::istringstream ss(timestampStr);
						ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
						if (!ss.fail())
						{
							msg.timestamp =
								std::chrono::system_clock::from_time_t(std::mktime(&tm));
						} else
						{
							msg.timestamp = std::chrono::system_clock::now();
						}
					} else
					{
						msg.timestamp = std::chrono::system_clock::now();
					}
					messages.push_back(msg);
				}
				if (setMessagesCallbackVar)
				{
					setMessagesCallbackVar(messages);
				}
				if (updateDisplayCallback)
				{
					updateDisplayCallback();
				}
				if (currentConversationTimestamp)
				{
					*currentConversationTimestamp =
						latestConversation.value("timestamp", "");
				}
				std::cout << "Loaded " << messages.size()
						  << " messages from agent conversation history" << std::endl;
			}
		}
	} catch (const json::parse_error &e)
	{
		std::cerr << "Error parsing agent conversation history: " << e.what()
				  << std::endl;
		try
		{
			fs::remove(historyPath);
			std::cerr << "Removed corrupted agent conversation history file" << std::endl;
		} catch (...)
		{
		}
	} catch (const std::exception &e)
	{
		std::cerr << "Error loading agent conversation history: " << e.what()
				  << std::endl;
	}
}

void AIAgentHistory::clearConversationHistory()
{
	// Clear messages through callback
	if (setMessagesCallbackVar)
	{
		setMessagesCallbackVar(std::vector<Message>());
	}

	// Clear the timestamp to indicate this is a new conversation
	if (currentConversationTimestamp)
	{
		currentConversationTimestamp->clear();
	}

	// Update display through callback
	if (updateDisplayCallback)
	{
		updateDisplayCallback();
	}
}

void AIAgentHistory::loadConversationFromHistory(
	const std::vector<std::string> &formattedMessages, const std::string &timestamp)
{
	if (currentConversationTimestamp)
	{
		*currentConversationTimestamp = timestamp;
	}
	std::vector<Message> messages;
	for (const auto &formattedMsg : formattedMessages)
	{
		Message msg;

		// Parse the formatted message to determine role and extract text
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
	if (setMessagesCallbackVar)
	{
		setMessagesCallbackVar(messages);
	}
	if (updateDisplayCallback)
	{
		updateDisplayCallback();
	}
	std::cout << "Loaded " << messages.size() << " messages from conversation history"
			  << std::endl;
}