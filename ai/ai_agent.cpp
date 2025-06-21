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
    textSelect([](size_t){ return ""; }, []{ return 0; }, true), // dummy accessors, word wrap enabled
    forceScrollToBottomNextFrame(false),
    lastKnownWidth(0.0f)
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
    // If we are currently streaming, set the cancel flag
    if (isStreaming.load()) {
        shouldCancelStreaming.store(true);
        isStreaming.store(false);
    }

    // Always handle thread cleanup if joinable, regardless of streaming state
    if (streamingThread.joinable()) {
        try {
            streamingThread.join();
        } catch (const std::exception& e) {
            // Exception during join - silently handle
        }
    }
    
    shouldCancelStreaming.store(false);
}

void AIAgent::startStreamingRequest(const std::string& prompt, const std::string& api_key) {
    // Safety check: ensure we're not already streaming
    if (isStreaming.load()) {
        return;
    }
    
    // Build conversation history from existing messages
    std::string fullPrompt;
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        for (const auto& msg : messages) {
            fullPrompt += (msg.isAgent ? "Assistant: " : "User: ") + msg.text + "\n";
        }
    }
    fullPrompt += "User: " + prompt;
    
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
        streamingThread = std::thread([this, fullPrompt, api_key]() {
            try {
                bool success = OpenRouter::promptRequestStream(fullPrompt, api_key, [this](const std::string& token) {
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
                    } catch (const std::exception& e) {
                        // Exception in token callback - silently handle
                    }
                }, &shouldCancelStreaming);
            } catch (const std::exception& e) {
                // Exception in streaming thread - silently handle
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
                // Exception in cleanup - silently handle
            }
        });
    } catch (const std::exception& e) {
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
    float historyHeight = windowHeight * 0.7f;
    ImVec2 historySize = ImVec2(textBoxWidth - 2 * horizontalPadding + 50.0f, historyHeight);
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

    // Calculate button size based on font size
    ImVec2 textSize = ImGui::CalcTextSize("Send");
    ImVec2 buttonSize = ImVec2(textSize.x + 16.0f, 0); // Add padding for comfortable button size

    if (ImGui::Button("Send", buttonSize)) {
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
    
    // Calculate text box height based on font size
    float fontSize = ImGui::GetFontSize();
    int numLines = (fontSize > 30.0f) ? 2 : 3.5; // 2 lines for large fonts, 3 for normal fonts
    ImVec2 textBoxSize = ImVec2(textBoxWidth - 2 * horizontalPadding, lineHeight * numLines + 16.0f);
    
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
    
    // Calculate available width for text (accounting for padding, scrollbar, and some buffer)
    float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
    float childPadding = ImGui::GetStyle().ChildRounding * 2.0f; // Account for child window padding
    float availableWidth = lastKnownWidth - scrollbarWidth - childPadding - 8.0f; // scrollbar + child padding + buffer
    if (availableWidth < 50.0f) availableWidth = 50.0f; // Minimum width
    
    for (size_t i = 0; i < messagesCopy.size(); ++i) {
        const auto& msg = messagesCopy[i];
        std::string displayText = msg.text;
        displayText = (msg.isAgent ? "Agent: " : "User: ") + displayText;
        
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
        
        if (i > 0 && i % 2 == 1 && i < messagesCopy.size() - 1) {
            messageDisplayLines.push_back("--- next message ---");
        }
    }
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

void AIAgent::renderMessageHistory(const ImVec2& size) {
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

    // Use vertical scrollbar only
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | 
                            ImGuiWindowFlags_AlwaysVerticalScrollbar;
    
    ImGui::BeginChild("text", size, false, flags);

    // Render each line as TextUnformatted (or TextWrapped if you want word wrap)
    for (size_t i = 0; i < messageDisplayLines.size(); ++i) {
        ImGui::TextWrapped("%s", messageDisplayLines[i].c_str());
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
        return true;
    }
    return false;
}

// Returns a substring of s from character index start to end (exclusive)
static std::string_view utf8_substr(std::string_view s, std::size_t start, std::size_t end) {
    auto it_start = s.begin();
    utf8::unchecked::advance(it_start, start);
    auto it_end = it_start;
    utf8::unchecked::advance(it_end, end - start);
    return std::string_view(&*it_start, it_end - it_start);
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
        ImGuiIO& io = ImGui::GetIO();
        if (!io.KeyShift) {
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
        
        // Calculate available width for hint text
        float availableWidth = textBoxWidth - padding.x * 2.0f - 4.0f; // Account for padding and small buffer
        
        // Choose appropriate hint text based on font size and available width
        const char* hintText = "prompt here";
        float fontSize = ImGui::GetFontSize();
        ImVec2 textSize = ImGui::CalcTextSize(hintText);
        
        // If the text is too wide for the container, use a shorter version
        if (textSize.x > availableWidth) {
            if (fontSize > 20.0f) {
                hintText = "prompt..."; // Very short for large fonts
            } else if (textSize.x > availableWidth * 0.8f) {
                hintText = "prompt"; // Shorter version for medium fonts
            }
        }
        
        // Recalculate text size with the potentially shorter text
        textSize = ImGui::CalcTextSize(hintText);
        
        // Only draw if the text actually fits
        if (textSize.x <= availableWidth) {
            ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), hintText);
        }
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
        } else {
            editor_state.block_input = false;
        }
        wasFocused = isFocused;
    }
    if (isFocused) {
        editor_state.block_input = true;
    }

    // ImGui::EndChild();
}

void AIAgent::printAllMessages() {
    // Function kept for compatibility but no longer prints anything
}

void AIAgent::sendMessage(const char* msg) {
    {
        std::lock_guard<std::mutex> lock(messagesMutex);
        messages.push_back({msg ? msg : "", false});
        messageDisplayLinesDirty = true;
    }
    userScrolledUp = false;
    scrollToBottom = true;
    forceScrollToBottomNextFrame = true;
    
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
        return;
    }
    
    // Start streaming request (this will set isStreaming to true internally)
    startStreamingRequest(msg, api_key);
}
