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

// ImGuiTextSelect integration for selectable/copyable message history
// Requires textselect.cpp/textselect.hpp and utfcpp (utf8.h) in the include path
// See: https://github.com/WhaleConnect/mGuiTextSelect

AIAgent::AIAgent() : frameCounter(0),
    textSelect([](size_t){ return ""; }, []{ return 0; }, true) // dummy accessors, word wrap enabled
{
    // Initialize input buffer
    strncpy(inputBuffer, "", sizeof(inputBuffer));
    
    // Initialize thread object to ensure it's in a valid state
    streamingThread = std::thread();
    
    // Initialize focus restoration flag
    shouldRestoreFocus = false;

    // Initialize utf8_buffer to empty
    utf8_buffer.clear();
}

AIAgent::~AIAgent() {
    stopStreaming();
}

void AIAgent::stopStreaming() {
    std::cout << "[AIAgent] Stopping streaming..." << std::endl;
    
    // If we are currently streaming, set the cancel flag
    if (isStreaming.load()) {
        shouldCancelStreaming.store(true);
        isStreaming.store(false);
    }

    // Always handle thread cleanup if joinable, regardless of streaming state
    if (streamingThread.joinable()) {
        std::cout << "[AIAgent] Joining streaming thread..." << std::endl;
        try {
            streamingThread.join();
            std::cout << "[AIAgent] Thread joined successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[AIAgent] Exception during join: " << e.what() << std::endl;
        }
    }
    
    shouldCancelStreaming.store(false);
}

void AIAgent::startStreamingRequest(const std::string& prompt, const std::string& api_key) {
    std::cout << "[Streaming] Starting streaming request for prompt: " << prompt << std::endl;
    
    // Safety check: ensure we're not already streaming
    if (isStreaming.load()) {
        std::cout << "[Streaming] Already streaming, aborting new request" << std::endl;
        return;
    }
    
    // Set streaming flag before creating thread
    isStreaming.store(true);
    
    // Add a streaming message
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messages.push_back({"", true, true});
        messageDisplayLinesDirty = true;
    }
    scrollToBottom = true;
    
    // Start streaming in a separate thread
    try {
        std::cout << "[Streaming] Creating streaming thread..." << std::endl;
        streamingThread = std::thread([this, prompt, api_key]() {
            std::cout << "[Streaming] Thread started, calling promptRequestStream" << std::endl;
            
            try {
                bool success = OpenRouter::promptRequestStream(prompt, api_key, [this](const std::string& token) {
                    try {
                        // UTF-8 safe streaming: buffer incomplete bytes
                        std::string combined = utf8_buffer + token;
                        auto valid_end = combined.begin();
                        try {
                            valid_end = utf8::find_invalid(combined.begin(), combined.end());
                        } catch (...) {
                            valid_end = combined.begin();
                        }
                        {
                            std::lock_guard<std::mutex> lock(messagesMutex);
                            if (!messages.empty() && messages.back().isStreaming) {
                                messages.back().text += std::string(combined.begin(), valid_end);
                                messageDisplayLinesDirty = true;
                            }
                        }
                        utf8_buffer = std::string(valid_end, combined.end());
                        scrollToBottom = true;

                        for (unsigned char c : token) {
                            printf("%02X ", c);
                        }
                        printf("\n");
                    } catch (const std::exception& e) {
                        std::cout << "[Streaming] Exception in token callback: " << e.what() << std::endl;
                    }
                }, &shouldCancelStreaming);
                
                std::cout << "[Streaming] Stream completed, success: " << success << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[Streaming] Exception in streaming thread: " << e.what() << std::endl;
            }
            
            // Mark streaming as complete
            try {
                std::lock_guard<std::mutex> lock(messagesMutex);
                if (!messages.empty() && messages.back().isStreaming) {
                    messages.back().isStreaming = false;
                    messageDisplayLinesDirty = true;
                }
                isStreaming.store(false);
            } catch (const std::exception& e) {
                std::cout << "[Streaming] Exception in cleanup: " << e.what() << std::endl;
            }
        });
        std::cout << "[Streaming] Thread created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "[Streaming] Failed to create thread: " << e.what() << std::endl;
        isStreaming.store(false);
    }
}

void AIAgent::render(float agentPaneWidth) {
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
    float historyHeight = windowHeight * 0.6f;
    ImVec2 historySize = ImVec2(textBoxWidth - 2 * horizontalPadding, historyHeight);
    renderMessageHistory(historySize);
    ImGui::Spacing();

    // Style the Send button to match the input box
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Safe access to backgroundColor with null checks
    auto& bgColor = gSettings.getSettings()["backgroundColor"];
    float bgR = (bgColor.is_array() && bgColor.size() > 0 && !bgColor[0].is_null()) ? bgColor[0].get<float>() : 0.1f;
    float bgG = (bgColor.is_array() && bgColor.size() > 1 && !bgColor[1].is_null()) ? bgColor[1].get<float>() : 0.1f;
    float bgB = (bgColor.is_array() && bgColor.size() > 2 && !bgColor[2].is_null()) ? bgColor[2].get<float>() : 0.1f;
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(bgR * 0.8f, bgG * 0.8f, bgB * 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(bgR * 0.95f, bgG * 0.95f, bgB * 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(bgR * 0.7f, bgG * 0.7f, bgB * 0.7f, 1.0f));

    if (ImGui::Button("Send", ImVec2(80, 0))) {
        const char* str = inputBuffer;
        // Skip leading whitespace
        while (*str && isspace((unsigned char)*str)) {
            str++;
        }

        if (*str == '\0') {
            // String is empty or contains only whitespace
            gSettings.renderNotification("No prompt provided", 3.0f);
        } else {
            sendMessage(inputBuffer);
            inputBuffer[0] = '\0';
        }
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    ImGui::Spacing();

    float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 textBoxSize = ImVec2(textBoxWidth - 2 * horizontalPadding, lineHeight * 3 + 16.0f); // Subtract padding from the size, not the width
    AgentInput(textBoxSize, textBoxWidth - 2 * horizontalPadding, horizontalPadding);
    ImGui::EndGroup(); // End vertical group

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(horizontalPadding, 0)); // Right padding
    ImGui::EndGroup();
}

void AIAgent::rebuildMessageDisplayLines() {
    std::vector<Message> messagesCopy;
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messagesCopy = messages;
    }
    messageDisplayLines.clear();
    for (size_t i = 0; i < messagesCopy.size(); ++i) {
        const auto& msg = messagesCopy[i];
        std::string displayText = msg.text;
        displayText = (msg.isAgent ? "Agent: " : "User: ") + displayText;
        size_t start = 0;
        while (start < displayText.size()) {
            size_t end = displayText.find('\n', start);
            if (end == std::string::npos) {
                messageDisplayLines.push_back(displayText.substr(start));
                break;
            } else {
                messageDisplayLines.push_back(displayText.substr(start, end - start));
                start = end + 1;
            }
        }
        if (i > 0 && i % 2 == 1 && i < messagesCopy.size() - 1) {
            messageDisplayLines.push_back("--- next message ---");
        }
    }
}

void AIAgent::renderMessageHistory(const ImVec2& size) {
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

    ImGui::BeginChild("text", size, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);

    ImVector<TextSelect::SubLine> subLines = textSelect.getSubLines();
    for (const auto& subLine : subLines) {
        ImGui::TextUnformatted(subLine.string.data(), subLine.string.data() + subLine.string.size());
    }

    textSelect.update();

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

    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }
    ImGui::EndChild();
}

// Helper function to handle word breaking and line wrapping
static bool HandleWordBreakAndWrap(char* inputBuffer, size_t bufferSize, ImGuiInputTextState* state, float max_width, float text_box_x) {
    int cursor = state->Stb.cursor;
    // Find start of current line
    int line_start = cursor;
    while (line_start > 0 && inputBuffer[line_start - 1] != '\n') {
        line_start--;
    }
    // Find end of current line
    int line_end = cursor;
    while (inputBuffer[line_end] != '\0' && inputBuffer[line_end] != '\n') {
        line_end++;
    }
    // Extract current line
    std::string current_line(inputBuffer + line_start, line_end - line_start);
    // Measure width
    float line_width = ImGui::CalcTextSize(current_line.c_str()).x;
    // Calculate width of 3 average characters (use 'a' as a rough estimate)
    float char_width = ImGui::CalcTextSize("aaa").x / 3.0f;
    float wrap_trigger_width = max_width - char_width * 3.0f;
    if (line_width <= wrap_trigger_width) return false;
    // Find start of current word
    int word_start = cursor;
    while (word_start > line_start && !isspace(inputBuffer[word_start - 1])) {
        word_start--;
    }
    // Check if there are any spaces before the start of the line
    bool has_space = false;
    for (int i = word_start - 1; i >= line_start; --i) {
        if (isspace(inputBuffer[i])) {
            has_space = true;
            break;
        }
    }
    bool all_spaces = true;
    for (char c : current_line) {
        if (!isspace(c)) {
            all_spaces = false;
            break;
        }
    }
    // Check if the last character before the cursor is a space
    bool last_char_is_space = (cursor > line_start && isspace(inputBuffer[cursor - 1]));
    int insert_pos = word_start;
    int move_cursor_to = cursor + 1; // default: after the cursor
    if ((!has_space && word_start == line_start) || all_spaces || last_char_is_space) {
        // No spaces before the start of the line, or line is all spaces, or last char is space: split three chars before the cursor (if possible)
        insert_pos = (cursor - 3 > line_start) ? cursor - 3 : line_start;
        move_cursor_to = insert_pos + 4; // after the split char and newline
    } else {
        // Normal word wrap
        // Find end of current word
        int word_end = cursor;
        while (inputBuffer[word_end] != '\0' && !isspace(inputBuffer[word_end]) && inputBuffer[word_end] != '\n') {
            word_end++;
        }
        move_cursor_to = word_end + 1; // after the word
    }
    // Insert newline
    if (insert_pos >= line_start && insert_pos < (int)strlen(inputBuffer)) {
        std::string before(inputBuffer, insert_pos);
        std::string after(inputBuffer + insert_pos);
        std::string new_text = before + "\n" + after;
        strncpy(inputBuffer, new_text.c_str(), bufferSize);
        // Update ImGui internal state
        state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, inputBuffer, nullptr);
        state->CurLenA = (int)strlen(inputBuffer);
        state->TextAIsValid = true;
        // Move cursor after the split
        state->Stb.cursor = move_cursor_to;
        state->Stb.select_start = move_cursor_to;
        state->Stb.select_end = move_cursor_to;
        state->CursorAnimReset();
        // Print debug info
        std::cout << "[Wrap] Inserted newline at: " << insert_pos << ", moved cursor to: " << move_cursor_to << std::endl;
        std::cout << "[Wrap] Buffer now: '" << inputBuffer << "'" << std::endl;
        return true;
    }
    return false;
}

void AIAgent::AgentInput(const ImVec2& textBoxSize, float textBoxWidth, float horizontalPadding) {
    // NOTE: Temporarily removed BeginChild/EndChild for diagnostics
    // ImGui::BeginChild("InputScrollRegion", textBoxSize, false, ImGuiWindowFlags_HorizontalScrollbar);

    // Apply custom styles
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Safe access to backgroundColor with null checks
    auto& bgColor = gSettings.getSettings()["backgroundColor"];
    float bgR = (bgColor.is_array() && bgColor.size() > 0 && !bgColor[0].is_null()) ? bgColor[0].get<float>() : 0.1f;
    float bgG = (bgColor.is_array() && bgColor.size() > 1 && !bgColor[1].is_null()) ? bgColor[1].get<float>() : 0.1f;
    float bgB = (bgColor.is_array() && bgColor.size() > 2 && !bgColor[2].is_null()) ? bgColor[2].get<float>() : 0.1f;
    
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(bgR * 0.8f, bgG * 0.8f, bgB * 0.8f, 1.0f));

    bool inputActive = ImGui::InputTextMultiline(
        "##AIAgentInput",
        inputBuffer,
        sizeof(inputBuffer),
        textBoxSize, // Use original textBoxSize
        ImGuiInputTextFlags_NoHorizontalScroll
    );

    // Restore focus if needed
    if (shouldRestoreFocus) {
        ImGui::SetKeyboardFocusHere(-1);
        shouldRestoreFocus = false;
    }

    // Check focus state immediately after the input widget
    bool isFocused = ImGui::IsItemActive();

    // Simple Enter key detection
    static bool enterPressed = false;
    if (isFocused && ImGui::IsKeyDown(ImGuiKey_Enter) && !enterPressed) {
        enterPressed = true;
        std::cout << "ENTER KEY DETECTED!" << std::endl;
        ImGuiIO& io = ImGui::GetIO();
        if (!io.KeyShift) {
            std::cout << "No shift, processing message" << std::endl;
            const char* str = inputBuffer;
            // Skip leading whitespace
            while (*str && isspace((unsigned char)*str)) {
                str++;
            }

            if (*str == '\0') {
                // String is empty or contains only whitespace
                gSettings.renderNotification("No prompt provided", 3.0f);
            } else {
                std::cout << "Sending message: '" << inputBuffer << "'" << std::endl;
                sendMessage(inputBuffer);
                // Clear the input buffer and reset ImGui state
                inputBuffer[0] = '\0';
                ImGui::ClearActiveID();
                // Set flag to restore focus on next frame
                shouldRestoreFocus = true;
            }
        }
    } else if (!ImGui::IsKeyDown(ImGuiKey_Enter)) {
        enterPressed = false;
    }

    // Draw hint if empty and not focused
    if (inputBuffer[0] == '\0' && !isFocused) {
        ImVec2 pos = ImGui::GetItemRectMin();
        ImVec2 padding = ImGui::GetStyle().FramePadding;
        pos.x += padding.x + 2.0f;
        pos.y += padding.y;
        ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "prompt here");
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##AIAgentInput");
    ImGuiInputTextState* state = ImGui::GetInputTextState(id);

    bool did_wrap = false;
    if (isFocused && state) {
        // Check if we need to wrap the text
        did_wrap = HandleWordBreakAndWrap(inputBuffer, sizeof(inputBuffer), state, textBoxWidth - 10.0f, textBoxWidth);
    }

    if (isFocused && (ImGui::IsItemEdited() || did_wrap)) {
        // Auto-scroll to bottom when text is edited or wrapped
        ImGui::SetScrollHereY(1.0f);
    }

    static bool wasFocused = false;
    if (isFocused != wasFocused) {
        if (isFocused) {
            editor_state.block_input = true;
            std::cout << "focused" << std::endl;
        } else {
            editor_state.block_input = false;
            std::cout << "unfocused" << std::endl;
        }
        wasFocused = isFocused;
    }
    if (isFocused) {
        editor_state.block_input = true;
    }

    // ImGui::EndChild();
}

void AIAgent::printAllMessages() {
    std::cout << "--- Message History ---" << std::endl;
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        for (const auto& msg : messages) {
            std::cout << (msg.isAgent ? "Agent: " : "User: ") << msg.text << std::endl;
        }
    }
    std::cout << "-----------------------" << std::endl;
}

void AIAgent::sendMessage(const char* msg) {
    std::cout << "[AIAgent] sendMessage called with: " << (msg ? msg : "null") << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messages.push_back({msg ? msg : "", false});
        messageDisplayLinesDirty = true;
    }
    std::cout << "sending message: " << msg << std::endl;
    printAllMessages();
    scrollToBottom = true;
    
    // Get API key from settings
    std::string api_key = gSettingsFileManager.getOpenRouterKey();
    if (api_key.empty()) {
        // Add error message if no API key is configured
        {
            std::lock_guard<std::mutex> lock(messagesMutex);
            messages.push_back({"Error: No OpenRouter API key configured. Please set your API key in Settings.", true, false});
        }
        scrollToBottom = true;
        return;
    }
    
    // Stop any existing streaming and wait for it to finish
    stopStreaming();
    
    // Ensure we're not streaming before starting a new request
    if (isStreaming.load()) {
        std::cout << "[AIAgent] Still streaming after stop, aborting new request" << std::endl;
        return;
    }
    
    // Start streaming request (this will set isStreaming to true internally)
    std::cout << "[AIAgent] Starting new streaming request" << std::endl;
    startStreamingRequest(msg, api_key);
}
