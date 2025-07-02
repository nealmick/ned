#include "ai_agent.h"
#include <imgui.h>
#include <iostream>
#include <cstring>
#include <cctype>
#include "editor/editor.h" // for editor_state
#include "util/settings.h"
#include <imgui_internal.h> 
#include <thread>
#include <mutex>
#include "textselect.hpp"
#include <utf8.h>
#include "agent_request.h"
#include "mcp/mcp_manager.h"
#include "../lib/json.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "../files/files.h" // for gFileExplorer

using json = nlohmann::json;
namespace fs = std::filesystem;

// External global MCP manager instance
extern MCP::Manager gMCPManager;

// ImGuiTextSelect integration for selectable/copyable message history
// Requires textselect.cpp/textselect.hpp and utfcpp (utf8.h) in the include path
// See: https://github.com/WhaleConnect/mGuiTextSelect

AIAgent::AIAgent() : frameCounter(0),
    textSelect([](size_t){ return ""; }, []{ return 0; }, true), // dummy accessors, word wrap enabled
    forceScrollToBottomNextFrame(false),
    lastKnownWidth(0.0f),
    lastToolCallTime(std::chrono::system_clock::now()),
    toolCallsProcessed(false)
{
    // Initialize input buffer
    strncpy(inputBuffer, "", sizeof(inputBuffer));
    
    // Set up history manager callback
    historyManager.setLoadConversationCallback([this](const std::vector<std::string>& messages, const std::string& timestamp) {
        loadConversationFromHistory(messages, timestamp);
    });

    // Set up text input component
    textInput.setInputBuffer(inputBuffer, sizeof(inputBuffer));
    textInput.setSendMessageCallback([this](const char* msg, bool hide_message) {
        sendMessage(msg, hide_message);
    });
    textInput.setIsProcessingCallback([this]() {
        return agentRequest.isProcessing();
    });
    textInput.setNotificationCallback([](const char* msg, float duration) {
        gSettings.renderNotification(msg, duration);
    });
    textInput.setClearConversationCallback([this]() {
        historyManager.clearConversationHistory();
    });
    textInput.setToggleHistoryCallback([this]() {
        historyManager.toggleWindow();
    });
    textInput.setBlockInputCallback([](bool block) {
        editor_state.block_input = block;
    });
    textInput.setStopRequestCallback([this]() {
        stopStreaming();
    });

    // Set up history manager callbacks for accessing messages and updating display
    historyManager.setMessagesCallback([this]() -> std::vector<Message> {
        std::lock_guard<std::mutex> lock(messagesMutex);
        return messages;
    });
    historyManager.setSetMessagesCallback([this](const std::vector<Message>& newMessages) {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messages = newMessages;
    });
    historyManager.setUpdateDisplayCallback([this]() {
        messageDisplayLinesDirty = true;
        scrollToBottom = true;
    });
    historyManager.setCurrentConversationTimestamp(currentConversationTimestamp);
}

AIAgent::~AIAgent() {
    stopStreaming();
}

void AIAgent::stopStreaming() {
    agentRequest.stopRequest();
}

// Helper method to check if a tool call should be allowed (prevents loops)
bool AIAgent::shouldAllowToolCall(const std::string& toolName) {
    auto now = std::chrono::system_clock::now();
    auto timeSinceLastCall = std::chrono::duration_cast<std::chrono::seconds>(now - lastToolCallTime).count();
    
    // Reset failure counts if more than 30 seconds have passed
    if (timeSinceLastCall > 30) {
        failedToolCalls.clear();
    }
    
    // Check if this tool has failed too many times
    auto it = failedToolCalls.find(toolName);
    if (it != failedToolCalls.end() && it->second >= MAX_FAILED_CALLS) {
        return false;
    }
    
    lastToolCallTime = now;
    return true;
}

// Helper method to record a failed tool call
void AIAgent::recordFailedToolCall(const std::string& toolName) {
    failedToolCalls[toolName]++;
    std::cout << "DEBUG: Recorded failed tool call for " << toolName << " (count: " << failedToolCalls[toolName] << ")" << std::endl;
}

void AIAgent::render(float agentPaneWidth, ImFont* largeFont) {
    // Check if we need to send a follow-up message from a tool call
    if (needsFollowUpMessage) {
        needsFollowUpMessage = false;
        sendMessage("### SYSTEM MESSAGE ### The tool call has been completed and the results are shown above. Please continue with the user's request. ### END SYSTEM MESSAGE ###", true);
    }
    
    float inputWidth = ImGui::GetContentRegionAvail().x;
    float windowHeight = ImGui::GetWindowHeight();
    float horizontalPadding = 16.0f;
    float textBoxWidth = inputWidth; // Do NOT subtract padding here
    if (textBoxWidth < 50.0f) textBoxWidth = 50.0f;

    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(horizontalPadding-10, 0)); // Left padding
    ImGui::SameLine();

    ImGui::BeginGroup(); // Vertical stack for message history, button, and input

    // Render message history above the send button and input
    float historyHeight = windowHeight * 0.7f;
    // Account for scroll bar width and padding to prevent overflow
    float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
    float historyWidth = textBoxWidth - 2 * horizontalPadding - scrollbarWidth + 45.0f; // Subtract scrollbar width and extra padding
    if (historyWidth < 50.0f) historyWidth = 50.0f; // Minimum width
    ImVec2 historySize = ImVec2(historyWidth, historyHeight);
    renderMessageHistory(historySize, largeFont);
    ImGui::Spacing();

    float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    float fontSize = ImGui::GetFontSize();
    int numLines = (fontSize > 30.0f) ? 2 : 3; // 2 lines for large fonts, 3 for normal fonts
    ImVec2 textBoxSize = ImVec2(textBoxWidth - 2 * horizontalPadding, lineHeight * numLines + 16.0f);
    textInput.render(textBoxSize, textBoxWidth - 2 * horizontalPadding, horizontalPadding);
    ImGui::EndGroup(); // End vertical group

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(horizontalPadding, 0)); // Right padding
    ImGui::EndGroup();
    
    // Render history if enabled
    historyManager.renderHistory();
}

void AIAgent::rebuildMessageDisplayLines() {
    std::vector<Message> messagesCopy;
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messagesCopy = messages;
    }
    messageDisplayLines.clear();
    
    std::cout << "=== DEBUG: rebuildMessageDisplayLines called ===" << std::endl;
    std::cout << "Message count: " << messagesCopy.size() << std::endl;
    
    // Calculate available width for text (accounting for padding, scrollbar, and some buffer)
    float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
    float childPadding = ImGui::GetStyle().ChildRounding * 2.0f; // Account for child window padding
    float availableWidth = lastKnownWidth - scrollbarWidth - childPadding - 8.0f; // scrollbar + child padding + buffer
    if (availableWidth < 50.0f) availableWidth = 50.0f; // Minimum width
    
    for (size_t i = 0; i < messagesCopy.size(); ++i) {
        const auto& msg = messagesCopy[i];
        
        std::cout << "Processing message " << i << ": role=" << msg.role << ", text length=" << msg.text.length() << std::endl;
        
        // Skip hidden messages
        if (msg.hide_message) {
            std::cout << "Skipping hidden message" << std::endl;
            continue;
        }
        
        std::string displayText = msg.text;
        
        // Use role-based display only
        if (msg.role == "assistant") {
            displayText = "##### Agent: " + displayText;
        } else if (msg.role == "user") {
            displayText = "##### User: " + displayText;
        } else if (msg.role == "tool") {
            displayText = "##### Tool Result: " + displayText;
            std::cout << "Processing tool message: " << displayText.substr(0, 100) << "..." << std::endl;
        } else if (msg.role == "system") {
            displayText = "##### System: " + displayText;
        }
        
        // Handle both existing newlines and word wrapping
        size_t start = 0;
        while (start < displayText.size()) {
            // First, find the next existing newline
            size_t nextNewline = displayText.find('\n', start);
            if (nextNewline == std::string::npos) {
                // No more newlines, wrap the remaining text
                std::string remainingText = displayText.substr(start);
                wrapTextToWidth(remainingText, availableWidth);
                break;
            } else {
                // Found a newline, wrap the text up to it
                std::string lineText = displayText.substr(start, nextNewline - start);
                wrapTextToWidth(lineText, availableWidth);
                start = nextNewline + 1;
            }
        }
    }
    
    std::cout << "Final display lines count: " << messageDisplayLines.size() << std::endl;
    std::cout << "=== END DEBUG ===" << std::endl;
}

// Helper function to wrap text to a specific width
void AIAgent::wrapTextToWidth(const std::string& text, float maxWidth) {
    if (text.empty()) {
        messageDisplayLines.push_back("");
        return;
    }
    
    // Add a small buffer to prevent character cutoff
    float safeWidth = maxWidth - 4.0f; // 4px buffer
    
    std::string currentLine;
    std::string currentWord;
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        
        if (c == ' ' || c == '\t') {
            // End of word, try to add it to current line
            if (!currentWord.empty()) {
                std::string testLine = currentLine + (currentLine.empty() ? "" : " ") + currentWord;
                float lineWidth = ImGui::CalcTextSize(testLine.c_str()).x;
                
                if (lineWidth-10.0f <= safeWidth) {
                    currentLine = testLine;
                } else {
                    // Word doesn't fit, start new line
                    if (!currentLine.empty()) {
                        messageDisplayLines.push_back(currentLine);
                    }
                    currentLine = currentWord;
                }
                currentWord.clear();
            }
            // Don't add spaces to currentLine here - they'll be added when we add the next word
        } else {
            // Add character to current word
            currentWord += c;
        }
    }
    
    // Handle the last word
    if (!currentWord.empty()) {
        std::string testLine = currentLine + (currentLine.empty() ? "" : " ") + currentWord;
        float lineWidth = ImGui::CalcTextSize(testLine.c_str()).x;
        
        if (lineWidth <= safeWidth) {
            currentLine = testLine;
        } else {
            // Last word doesn't fit, start new line
            if (!currentLine.empty()) {
                messageDisplayLines.push_back(currentLine);
            }
            currentLine = currentWord;
        }
    }
    
    // Add the final line if not empty
    if (!currentLine.empty()) {
        messageDisplayLines.push_back(currentLine);
    }
}

void AIAgent::renderMessageHistory(const ImVec2& size, ImFont* largeFont) {
    // Check if width has changed and trigger rebuild if needed
    float currentWidth = size.x;
    if (std::abs(currentWidth - lastKnownWidth) > 1.0f) { // Small threshold to avoid unnecessary rebuilds
        messageDisplayLinesDirty.store(true);
        lastKnownWidth = currentWidth;
    }
    
    if (messageDisplayLinesDirty.load()) {
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
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 13.0f); // Make scrollbar wider for better usability
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Hide scrollbar track
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.4f, 0.4f, 0.4f, 0.5f)); // Completely transparent by default
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f)); // Show on hover
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.7f, 0.7f, 0.7f, 0.9f)); // Show when active
    
    ImGui::BeginChild("text", size, false, flags);

    // Check if chat is empty and show centered "Agent" title
    if (messageDisplayLines.empty()) {
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
        if (!dropdownInitialized || gSettings.profileJustSwitched || currentAgentModel != newAgentModel) {
            currentAgentModel = newAgentModel;
            dropdownItems.clear();
            displayItems.clear();
            
            // Add current agent model as first option
            dropdownItems.push_back(currentAgentModel);
            
            // Create display version for current model (show left part before slash)
            std::string displayModel = currentAgentModel;
            size_t slashPos = currentAgentModel.find('/');
            if (slashPos != std::string::npos) {
                displayModel = currentAgentModel.substr(slashPos + 1, currentAgentModel.length() - slashPos - 1); // Show part after slash
            }else{
                displayModel = currentAgentModel;
            }
            displayItems.push_back(displayModel);
            
            // Add placeholder test values
            dropdownItems.push_back("anthropic/claude-sonnet-4");
            dropdownItems.push_back("google/gemini-2.0-flash-001");
            dropdownItems.push_back("google/gemini-2.5-flash-preview-05-20");
            dropdownItems.push_back("deepseek/deepseek-chat-v3-0324");
            dropdownItems.push_back("anthropic/claude-3.7-sonnet");
            dropdownItems.push_back("openai/gpt-4.1");
            dropdownItems.push_back("x-ai/grok-3-beta");
            dropdownItems.push_back("meta-llama/llama-3.3-70b-instruct");
            
            // Create display versions for placeholders (show right part after slash)
            displayItems.push_back("claude-sonnet-4");
            displayItems.push_back("gemini-2.0-flash-001");
            displayItems.push_back("gemini-2.5-flash-preview-05-20");
            displayItems.push_back("deepseek-chat-v3-0324");
            displayItems.push_back("claude-3.7-sonnet");
            displayItems.push_back("gpt-4.1");
            displayItems.push_back("grok-3-beta");
            displayItems.push_back("llama-3.3-70b-instruct");
            
            
            
            // Reset selection to first item (current model)
            selectedItem = 0;
            dropdownInitialized = true;
        }
        
        // Use the large font for the title
        if (largeFont) {
            if (largeFont) ImGui::PushFont(largeFont);
            ImVec2 textSize = largeFont->CalcTextSizeA(largeFont->FontSize, FLT_MAX, 0.0f, "Agent");
            ImVec2 centerPos = ImVec2(
                (windowSize.x - textSize.x) * 0.5f,
                (windowSize.y - textSize.y) * 0.5f - 30.0f
            );
            ImGui::SetCursorPos(centerPos);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "Agent");
            if (largeFont) ImGui::PopFont();
            
            // Calculate the maximum width needed for all dropdown items
            float maxItemWidth = 0.0f;
            for (const auto& item : displayItems) {
                ImVec2 itemSize = largeFont->CalcTextSizeA(largeFont->FontSize, FLT_MAX, 0.0f, item.c_str());
                maxItemWidth = std::max(maxItemWidth, itemSize.x);
            }
            
            // Add padding for the dropdown button (arrow + some spacing)
            float dropdownPadding = 40.0f; // Space for dropdown arrow and padding
            float dropdownWidth = maxItemWidth + dropdownPadding;
            
            // Ensure minimum width and maximum width constraints
            dropdownWidth = std::max(dropdownWidth, 180.0f); // Minimum width
            dropdownWidth = std::min(dropdownWidth, windowSize.x * 0.8f); // Maximum 80% of window width
            
            ImVec2 dropdownSize(dropdownWidth, 0.0f);
            ImVec2 dropdownPos = ImVec2(
                (windowSize.x - dropdownSize.x) * 0.5f,
                centerPos.y + textSize.y + 20.0f
            );
            ImGui::SetCursorPos(dropdownPos);
            std::vector<const char*> items;
            for (const auto& item : displayItems) items.push_back(item.c_str());
            ImGui::SetNextItemWidth(dropdownSize.x);
            int previousSelectedItem = selectedItem;
            if (ImGui::Combo("##AgentDropdown", &selectedItem, items.data(), items.size())) {
                if (selectedItem >= 0 && selectedItem < (int)dropdownItems.size()) {
                    std::string newModel = dropdownItems[selectedItem]; // Use full path from dropdownItems
                    gSettings.getSettings()["agent_model"] = newModel;
                    gSettings.saveSettings();
                    std::cout << "Agent model changed to: " << newModel << std::endl;
                }
            }
        } else {
            ImFont* currentFont = ImGui::GetFont();
            if (currentFont) ImGui::PushFont(currentFont);
            ImGui::SetWindowFontScale(2.0f);
            ImVec2 textSize = ImGui::CalcTextSize("Agent");
            ImVec2 centerPos = ImVec2(
                (windowSize.x - textSize.x) * 0.5f,
                (windowSize.y - textSize.y) * 0.5f - 30.0f
            );
            ImGui::SetCursorPos(centerPos);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "Agent");
            
            // Calculate the maximum width needed for all dropdown items
            float maxItemWidth = 0.0f;
            for (const auto& item : displayItems) {
                ImVec2 itemSize = ImGui::CalcTextSize(item.c_str());
                maxItemWidth = std::max(maxItemWidth, itemSize.x);
            }
            
            // Add padding for the dropdown button (arrow + some spacing)
            float dropdownPadding = 40.0f; // Space for dropdown arrow and padding
            float dropdownWidth = maxItemWidth + dropdownPadding;
            
            // Ensure minimum width and maximum width constraints
            dropdownWidth = std::max(dropdownWidth, 120.0f); // Minimum width
            dropdownWidth = std::min(dropdownWidth, windowSize.x * 0.8f); // Maximum 80% of window width
            
            ImVec2 dropdownSize(dropdownWidth, 0.0f);
            ImVec2 dropdownPos = ImVec2(
                (windowSize.x - dropdownSize.x) * 0.5f,
                centerPos.y + textSize.y + 20.0f
            );
            ImGui::SetCursorPos(dropdownPos);
            std::vector<const char*> items;
            for (const auto& item : displayItems) items.push_back(item.c_str());
            ImGui::SetNextItemWidth(dropdownSize.x);
            int previousSelectedItem = selectedItem;
            if (ImGui::Combo("##AgentDropdown", &selectedItem, items.data(), items.size())) {
                if (selectedItem >= 0 && selectedItem < (int)dropdownItems.size()) {
                    std::string newModel = dropdownItems[selectedItem]; // Use full path from dropdownItems
                    gSettings.getSettings()["agent_model"] = newModel;
                    gSettings.saveSettings();
                    std::cout << "Agent model changed to: " << newModel << std::endl;
                }
            }
            ImGui::SetWindowFontScale(1.0f);
            if (currentFont) ImGui::PopFont();
        }
    } else {
        // Render messages as usual
        for (size_t i = 0; i < messageDisplayLines.size(); ++i) {
            ImGui::TextWrapped("%s", messageDisplayLines[i].c_str());
        }
    }

    // Let TextSelect handle selection and copy
    textSelect.update();

    // Scroll to bottom if the flag is set
    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    if (ImGui::BeginPopupContextWindow()) {
        ImGui::BeginDisabled(!textSelect.hasSelection());
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            textSelect.copy();
        }
        ImGui::EndDisabled();
        if (ImGui::MenuItem("Select all", "Ctrl+A")) {
            textSelect.selectAll();
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();
    
    // Restore styles
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
}

void AIAgent::printAllMessages() {
    // Function kept for compatibility but no longer prints anything
}

void AIAgent::sendMessage(const char* msg, bool hide_message) {
    if (needsFollowUpMessage) {
        needsFollowUpMessage = false;
        sendMessage("Please continue with the user's request based on the tool call results.", false);
        return;
    }
    userScrolledUp = false;
    scrollToBottom = true;
    forceScrollToBottomNextFrame = true;
    std::string api_key = gSettingsFileManager.getOpenRouterKey();
    if (api_key.empty()) {
        std::lock_guard<std::mutex> lock(messagesMutex);
        Message errorMsg;
        errorMsg.text = "Error: No OpenRouter API key configured. Please set your API key in Settings.";
        errorMsg.role = "assistant";
        errorMsg.isStreaming = false;
        errorMsg.hide_message = false;
        errorMsg.timestamp = std::chrono::system_clock::now();
        messages.push_back(errorMsg);
        scrollToBottom = true;
        return;
    }
    stopStreaming();
    if (agentRequest.isProcessing()) {
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
    std::string systemPrompt = "You are a helpful AI assistant with access to file system and terminal tools. Use these tools when they would help accomplish the user's request.\n\n";
    
    // Add current project directory if available
    if (!gFileExplorer.selectedFolder.empty()) {
        systemPrompt += "Current project directory: " + gFileExplorer.selectedFolder + "\n";
    }
    
    // Add current open file if available
    if (!gFileExplorer.currentOpenFile.empty()) {
        systemPrompt += "Current open file: " + gFileExplorer.currentOpenFile + "\n";
    }
    
    systemPrompt += "\n";
    
    systemMessage["content"] = systemPrompt;
    messagesJson.push_back(systemMessage);
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        for (const auto& msg : messages) {
            if (!msg.hide_message) {
                json messageObj;
                messageObj["role"] = msg.role;
                if (msg.role == "tool") {
                    messageObj["content"] = msg.text;
                    if (!msg.tool_call_id.empty()) {
                        messageObj["tool_call_id"] = msg.tool_call_id;
                    }
                } else if (!msg.tool_calls.is_null()) {
                    if (msg.text.empty()) {
                        messageObj["content"] = nullptr;
                    } else {
                        messageObj["content"] = msg.text;
                    }
                    messageObj["tool_calls"] = msg.tool_calls;
                } else {
                    messageObj["content"] = msg.text;
                }
                messagesJson.push_back(messageObj);
            }
        }
    }
    payload["messages"] = messagesJson;
    json toolsJson = json::array();
    std::vector<MCP::ToolDefinition> tools = gMCPManager.getToolDefinitions();
    for (const auto& tool : tools) {
        json toolObj;
        toolObj["type"] = "function";
        json functionObj;
        functionObj["name"] = tool.name;
        functionObj["description"] = tool.description;
        json properties = json::object();
        json required = json::array();
        for (const auto& param : tool.parameters) {
            json paramObj;
            paramObj["type"] = param.type;
            paramObj["description"] = param.description;
            properties[param.name] = paramObj;
            if (param.required) {
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
        payloadStr, api_key,
        [this](const std::string& token) {
            std::lock_guard<std::mutex> lock(messagesMutex);
            if (!messages.empty() && messages.back().isStreaming) {
                messages.back().text += token;
                messageDisplayLinesDirty = true;
            }
            scrollToBottom = true;
        },
        [this](const std::string& finalResult, bool hadToolCall) {
            {
                std::lock_guard<std::mutex> lock(messagesMutex);
                if (!messages.empty() && messages.back().isStreaming) {
                    if (hadToolCall) {
                        // This is a tool call result - create a tool message
                        messages.back().text = finalResult;
                        messages.back().isStreaming = false;
                        messages.back().role = "tool";
                        messageDisplayLinesDirty = true;
                        
                        // Create a new assistant message for the AI's response
                        Message assistantMsg;
                        assistantMsg.text = "";
                        assistantMsg.role = "assistant";
                        assistantMsg.isStreaming = true;
                        assistantMsg.hide_message = false;
                        assistantMsg.timestamp = std::chrono::system_clock::now();
                        messages.push_back(assistantMsg);
                        
                        // Send a follow-up message to get the AI's response
                        needsFollowUpMessage = true;
                    } else {
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
            if (hadToolCall && !toolCallsProcessed) {
                toolCallsProcessed = true;
                needsFollowUpMessage = true;
            }
        }
    );
}

void AIAgent::loadConversationFromHistory(const std::vector<std::string>& formattedMessages, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(messagesMutex);
    
    // Store the timestamp of the loaded conversation
    currentConversationTimestamp = timestamp;
    
    // Clear current messages
    messages.clear();
    
    // Reset conversation state variables to prevent issues when switching conversations
    needsFollowUpMessage = false;
    toolCallsProcessed = false;
    failedToolCalls.clear();
    lastToolCallTime = std::chrono::system_clock::now();
    
    // Parse formatted messages and add them to the current conversation
    for (const auto& formattedMsg : formattedMessages) {
        Message msg;
        
        // Check if it's an agent, user, or tool message and set the role correctly
        if (formattedMsg.find("##### Agent: ") == 0) {
            msg.text = formattedMsg.substr(12); // Remove "##### Agent: " prefix
            msg.role = "assistant";
        } else if (formattedMsg.find("##### User: ") == 0) {
            msg.text = formattedMsg.substr(11); // Remove "##### User: " prefix
            msg.role = "user";
        } else if (formattedMsg.find("##### Tool Result: ") == 0) {
            msg.text = formattedMsg.substr(18); // Remove "##### Tool Result: " prefix
            msg.role = "tool";
        } else {
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
    
    std::cout << "Loaded " << messages.size() << " messages from conversation history" << std::endl;
}
