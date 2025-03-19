#include "lsp.h"
#include "../editor/editor.h"
#include "../editor/editor_line_jump.h"
#include "../editor/editor_types.h"
#include "files.h"
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <sys/select.h>

// Global instance
EditorLSP gEditorLSP;

EditorLSP::EditorLSP() : currentRequestId(0), showDefinitionOptions(false), selectedDefinitionIndex(0) {}

EditorLSP::~EditorLSP() = default;

bool EditorLSP::initialize(const std::string &workspacePath)
{
    std::cout << "\033[35mLSP:\033[0m Initializing with workspace path: " << workspacePath << std::endl;

    // Delegate to the LSP manager
    return gLSPManager.initialize(workspacePath);
}

std::string EditorLSP::escapeJSON(const std::string &s) const
{
    std::string out;
    out.reserve(s.length() * 2);
    for (char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if ('\x00' <= c && c <= '\x1f') {
                char buf[8];
                snprintf(buf, sizeof buf, "\\u%04x", c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    return out;
}

void EditorLSP::didOpen(const std::string &filePath, const std::string &content)
{
    // Select the appropriate adapter for this file
    if (!gLSPManager.selectAdapterForFile(filePath)) {
        std::cout << "\033[31mLSP:\033[0m No LSP adapter available for file: " << filePath << std::endl;
        return;
    }

    // If the selected adapter is not initialized, initialize it with the current file's directory
    if (!gLSPManager.isInitialized()) {
        // Extract workspace path from file path (use directory containing the file)
        std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
        std::cout << "\033[35mLSP:\033[0m Auto-initializing with workspace: " << workspacePath << std::endl;

        if (!gLSPManager.initialize(workspacePath)) {
            std::cout << "\033[31mLSP:\033[0m Failed to initialize LSP for " << filePath << std::endl;
            return;
        }
    }

    std::cout << "\033[35mLSP:\033[0m Sending didOpen notification for file: " << filePath << std::endl;

    std::string notification = std::string(R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didOpen",
        "params": {
            "textDocument": {
                "uri": "file://)") +
                               filePath + R"(",
                "languageId": ")" +
                               gLSPManager.getLanguageId(filePath) + R"(",
                "version": 1,
                "text": ")" + escapeJSON(content) +
                               R"("
            }
        }
    })";

    gLSPManager.sendRequest(notification);
    std::cout << "\033[32mLSP:\033[0m didOpen notification sent successfully" << std::endl;
}
bool EditorLSP::gotoDefinition(const std::string &filePath, int line, int character)
{
    if (!gLSPManager.isInitialized()) {
        std::cout << "\033[31mLSP:\033[0m Not initialized" << std::endl;
        return false;
    }

    // Select the appropriate adapter for this file
    if (!gLSPManager.selectAdapterForFile(filePath)) {
        std::cout << "\033[31mLSP:\033[0m No LSP adapter available for file: " << filePath << std::endl;
        return false;
    }

    int requestId = getNextRequestId();
    std::cout << "\033[35mLSP:\033[0m Requesting definition at line " << line << ", char " << character << std::endl;

    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(requestId) +
                                      R"(,
        "method": "textDocument/definition",
        "params": {
            "textDocument": {
                "uri": "file://)" + filePath +
                                      R"("
            },
            "position": {
                "line": )" + std::to_string(line) +
                                      R"(,
                "character": )" + std::to_string(character) +
                                      R"(
            }
        }
    })");

    // Send the request through the manager
    if (!gLSPManager.sendRequest(request)) {
        std::cout << "\033[31mLSP:\033[0m Failed to send request" << std::endl;
        return false;
    }

    // Set a maximum number of attempts to avoid infinite loop
    const int MAX_ATTEMPTS = 15; // Increased from 5
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        std::cout << "\033[35mLSP:\033[0m Waiting for definition response (attempt " << (attempt + 1) << ")" << std::endl;

        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength);

        if (response.empty()) {
            std::cout << "\033[31mLSP:\033[0m Empty response received" << std::endl;
            if (attempt == MAX_ATTEMPTS - 1) {
                return false;
            }
            continue;
        }

        // Check if we received the correct response format
        if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos) {
            // We found a response matching our request ID
            std::cout << "\033[32mLSP:\033[0m Found response for our request ID" << std::endl;

            // Handle the various possible result formats

            // Format 1: Standard array of location objects
            if (response.find("\"result\":[{") != std::string::npos) {
                std::cout << "\033[32mLSP:\033[0m Found standard result array" << std::endl;
                parseDefinitionResponse(response);
                return true;
            }

            // Format 2: Single location object (not in an array)
            else if (response.find("\"result\":{") != std::string::npos) {
                std::cout << "\033[32mLSP:\033[0m Found single result object" << std::endl;
                // Modify response to make it look like an array for our parser
                std::string modifiedResponse = response;
                size_t pos = modifiedResponse.find("\"result\":{");
                if (pos != std::string::npos) {
                    modifiedResponse.insert(pos + 9, "[");
                    // Find the end of the result object
                    size_t endPos = modifiedResponse.find_last_of("}");
                    if (endPos != std::string::npos) {
                        modifiedResponse.insert(endPos + 1, "]");
                    }
                }
                parseDefinitionResponse(modifiedResponse);
                return true;
            }

            // Format 3: Empty array or null result (no definition found)
            else if (response.find("\"result\":null") != std::string::npos || response.find("\"result\":[]") != std::string::npos) {
                std::cout << "\033[33mLSP:\033[0m No definition found" << std::endl;
                return false;
            }

            // Format 4: Error in response
            else if (response.find("\"error\":") != std::string::npos) {
                std::cout << "\033[31mLSP:\033[0m Error in response: " << response << std::endl;
                return false;
            }

            // Unknown format, but we at least got a response to our request
            else {
                std::cout << "\033[33mLSP:\033[0m Got response to our request, but in unexpected format: " << response << std::endl;
                return false;
            }
        }

        // If we didn't find the response we're looking for, try again
        std::cout << "\033[33mLSP:\033[0m Received response but it's not what we're looking for. Continuing..." << std::endl;
    }

    std::cout << "\033[31mLSP:\033[0m Exceeded maximum attempts waiting for response" << std::endl;
    return false;
}

void EditorLSP::parseDefinitionResponse(const std::string &response)
{
    // Clear previous locations
    definitionLocations.clear();

    // Find result array
    size_t resultPos = response.find("\"result\":[");
    if (resultPos == std::string::npos) {
        return;
    }

    // Iterate through each result object
    size_t pos = resultPos;
    while ((pos = response.find("{", pos)) != std::string::npos) {
        // Look for the uri field in this object
        size_t uriPos = response.find("\"uri\":", pos);
        if (uriPos == std::string::npos)
            break;

        // Find the file path
        size_t uriStart = response.find("\"file://", uriPos);
        size_t uriEnd = response.find("\"", uriStart + 8);
        if (uriStart == std::string::npos || uriEnd == std::string::npos)
            break;

        std::string uri = response.substr(uriStart + 8, uriEnd - (uriStart + 8));

        // Find the range object
        size_t rangePos = response.find("\"range\":", pos);
        if (rangePos != std::string::npos) {
            int startLine = 0, startChar = 0, endLine = 0, endChar = 0;

            // Look for start position
            size_t startPos = response.find("\"start\":", rangePos);
            if (startPos != std::string::npos) {
                size_t linePos = response.find("\"line\":", startPos);
                size_t charPos = response.find("\"character\":", startPos);
                if (linePos != std::string::npos && charPos != std::string::npos) {
                    sscanf(response.c_str() + linePos, "\"line\":%d", &startLine);
                    sscanf(response.c_str() + charPos, "\"character\":%d", &startChar);
                }
            }

            // Look for end position
            size_t endPos = response.find("\"end\":", rangePos);
            if (endPos != std::string::npos) {
                size_t linePos = response.find("\"line\":", endPos);
                size_t charPos = response.find("\"character\":", endPos);
                if (linePos != std::string::npos && charPos != std::string::npos) {
                    sscanf(response.c_str() + linePos, "\"line\":%d", &endLine);
                    sscanf(response.c_str() + charPos, "\"character\":%d", &endChar);
                }
            }

            std::cout << "\033[35mLSP Parse:\033[0m Found position - Line: " << startLine << " Char: " << startChar << std::endl;

            definitionLocations.push_back({uri, startLine, startChar, endLine, endChar});
        }

        pos = uriEnd;
    }

    if (!definitionLocations.empty()) {
        std::cout << "\033[32mLSP Parse:\033[0m Found " << definitionLocations.size() << " definition locations" << std::endl;
        showDefinitionOptions = true;
        selectedDefinitionIndex = 0;
    } else {
        std::cout << "\033[31mLSP Parse:\033[0m No definitions found" << std::endl;
        showDefinitionOptions = false;
    }
}

bool EditorLSP::hasDefinitionOptions() const
{
    bool has_options = !definitionLocations.empty() && showDefinitionOptions;
    return has_options;
}

void EditorLSP::renderDefinitionOptions(EditorState &state)
{
    {
        if (!showDefinitionOptions || definitionLocations.empty()) {
            state.block_input = false;
            return;
        }
        state.block_input = true;
        // Calculate required height based on content
        float itemHeight = ImGui::GetTextLineHeightWithSpacing();
        float padding = 16.0f;
        float titleHeight = itemHeight + 4.0f;
        float footerHeight = itemHeight + padding;
        float contentHeight = itemHeight * definitionLocations.size();
        float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;

        // Calculate window position and size
        ImVec2 windowSize(500, totalHeight);
        ImVec2 windowPos(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f);

        // Set window position and size
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

        // Window flags
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

        // Push styles
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

        ImGui::Begin("##DefinitionOptions", nullptr, windowFlags);

        // Handle click outside window
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && (mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x || mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y)) {
                showDefinitionOptions = false;
                state.block_input = false;
                ImGui::End();
                ImGui::PopStyleColor(6);
                ImGui::PopStyleVar(4);
                return;
            }
        }

        // Rest of the rendering code...
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Go to Definition");
        ImGui::Separator();

        // Handle keyboard navigation
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            selectedDefinitionIndex = (selectedDefinitionIndex > 0) ? selectedDefinitionIndex - 1 : definitionLocations.size() - 1;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            selectedDefinitionIndex = (selectedDefinitionIndex + 1) % definitionLocations.size();
        }

        // Create child window to contain selectables with proper width
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float itemPadding = 8.0f; // Padding on each side

        // Display options
        for (size_t i = 0; i < definitionLocations.size(); i++) {
            const auto &loc = definitionLocations[i];
            bool is_selected = (selectedDefinitionIndex == i);

            // Add left padding
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + itemPadding);

            std::string label = loc.uri + " [" + std::to_string(loc.startLine + 1) + ":" + std::to_string(loc.startChar + 1) + "]";

            // Calculate selectable width to match separator (minus padding on both sides)
            if (ImGui::Selectable(label.c_str(), is_selected, 0, ImVec2(windowWidth - (itemPadding * 2), 0))) {
                selectedDefinitionIndex = i;
            }

            if (is_selected && ImGui::IsWindowAppearing()) {
                ImGui::SetScrollHereY();
            }
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "up/down to select and enter to confirm");

        // In the renderDefinitionOptions function, modify the Enter key handler:
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            const auto &selected = definitionLocations[selectedDefinitionIndex];
            std::cout << "Selected definition at " << selected.uri << " line " << (selected.startLine + 1) << " char " << (selected.startChar + 1) << std::endl;

            if (selected.uri != gFileExplorer.getCurrentFile()) {
                // Load the file first
                gFileExplorer.loadFileContent(selected.uri, nullptr);
            }

            // Calculate cursor position using correct global editor state
            int index = 0;
            int currentLine = 0;
            std::cout << "Calculating cursor position..." << std::endl;

            while (currentLine < selected.startLine && index < gFileExplorer.fileContent.length()) {
                if (gFileExplorer.fileContent[index] == '\n') {
                    currentLine++;
                }
                index++;
            }
            index += selected.startChar;
            index = std::min(index, (int)gFileExplorer.fileContent.length());
            state.cursor_index = index; //
            gEditorScroll.setEnsureCursorVisibleFrames(-1);
            showDefinitionOptions = false;
            state.block_input = false;
        } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showDefinitionOptions = false;
            state.block_input = false;
        }

        ImGui::End();

        // Pop styles
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(4);
    }
}