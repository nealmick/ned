#include "editor_lsp.h"
#include "editor.h"
#include "editor_line_jump.h"
#include "editor_types.h"
#include "files.h"
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Global instance
EditorLSP gEditorLSP;

class EditorLSP::LSPImpl
{
  public:
    LSPImpl() : input(nullptr), output(nullptr) {}
    ~LSPImpl()
    {
        if (input)
            pclose(input);
        if (output)
            pclose(output);
    }

    FILE *input;  // To LSP server
    FILE *output; // From LSP server
    int pid;      // LSP server process ID
};

EditorLSP::EditorLSP() : impl(new LSPImpl()), isInitialized(false)
{
    lspPath = "/usr/bin/clangd"; // Default MacOS Homebrew path
}

EditorLSP::~EditorLSP() = default;

bool EditorLSP::initialize(const std::string &workspacePath)
{
    if (isInitialized) {
        std::cout << "\033[35mLSP:\033[0m Already initialized" << std::endl;
        return true;
    }

    try {
        std::cout << "\033[35mLSP:\033[0m Starting LSP server with path: " << lspPath << std::endl;

        // Create pipes for communication
        int inPipe[2], outPipe[2];
        if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
            std::cerr << "\033[31mLSP:\033[0m Failed to create pipes" << std::endl;
            return false;
        }

        // Fork process
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "\033[31mLSP:\033[0m Fork failed" << std::endl;
            return false;
        }

        if (pid == 0) { // Child process
            std::cout << "\033[35mLSP:\033[0m Starting clangd process" << std::endl;

            dup2(outPipe[0], STDIN_FILENO);
            dup2(inPipe[1], STDOUT_FILENO);

            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);

            execl(lspPath.c_str(), "clangd", "--log=error", nullptr); // change to turn lsp logging on...

            std::cerr << "\033[31mLSP:\033[0m Failed to start clangd" << std::endl;
            exit(1);
        }

        // Parent process
        close(inPipe[1]);
        close(outPipe[0]);

        impl->input = fdopen(outPipe[1], "w");
        impl->output = fdopen(inPipe[0], "r");
        impl->pid = pid;

        if (!impl->input || !impl->output) {
            std::cerr << "\033[31mLSP:\033[0m Failed to open pipes" << std::endl;
            return false;
        }

        std::cout << "\033[35mLSP:\033[0m Sending initialize request for workspace: " << workspacePath << std::endl;

        // Create the initialize request string
        std::string initRequest = std::string(R"({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "processId": null,
                "rootUri": "file://)") +
                                  workspacePath + R"(",
                "capabilities": {
                    "textDocument": {
                        "definition": {
                            "dynamicRegistration": true
                        }
                    }
                }
            }
        })";

        fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", initRequest.length(), initRequest.c_str());
        fflush(impl->input);

        std::cout << "\033[32mLSP:\033[0m Initialize request sent successfully" << std::endl;

        isInitialized = true;
        return true;

    } catch (const std::exception &e) {
        std::cerr << "\033[31mLSP:\033[0m Initialization failed: " << e.what() << std::endl;
        return false;
    }
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

void EditorLSP::didChange(const std::string &filePath, const std::string &newContent, int version)
{
    if (!isInitialized) {
        std::cout << "\033[31mLSP:\033[0m Cannot send didChange - LSP not initialized" << std::endl;
        return;
    }

    std::cout << "\033[35mLSP:\033[0m Sending didChange notification for file: " << filePath << std::endl;

    std::string notification = std::string(R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {
                "uri": "file://)") +
                               filePath + R"(",
                "version": )" + std::to_string(version) +
                               R"(
            },
            "contentChanges": [
                {
                    "text": ")" +
                               escapeJSON(newContent) + R"("
                }
            ]
        }
    })";

    fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", notification.length(), notification.c_str());
    fflush(impl->input);

    std::cout << "\033[32mLSP:\033[0m didChange notification sent successfully" << std::endl;
}

std::string EditorLSP::getLanguageId(const std::string &filePath) const
{
    // Get file extension
    size_t dot_pos = filePath.find_last_of(".");
    if (dot_pos == std::string::npos) {
        return "plaintext";
    }

    std::string ext = filePath.substr(dot_pos + 1);

    // Map common extensions to language IDs
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp") {
        return "cpp";
    } else if (ext == "c") {
        return "c";
    } else if (ext == "py") {
        return "python";
    } else if (ext == "js") {
        return "javascript";
    } else if (ext == "ts") {
        return "typescript";
    } else if (ext == "rs") {
        return "rust";
    }
    // Add more mappings as needed

    return "plaintext";
}

void EditorLSP::didOpen(const std::string &filePath, const std::string &content)
{
    if (!isInitialized) {
        std::cout << "\033[31mLSP:\033[0m Cannot send didOpen - LSP not initialized" << std::endl;
        return;
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
                               getLanguageId(filePath) + R"(",
                "version": 1,
                "text": ")" + escapeJSON(content) +
                               R"("
            }
        }
    })";

    fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", notification.length(), notification.c_str());
    fflush(impl->input);

    std::cout << "\033[32mLSP:\033[0m didOpen notification sent successfully" << std::endl;
}

bool EditorLSP::gotoDefinition(const std::string &filePath, int line, int character)
{
    if (!isInitialized) {
        std::cout << "\033[31mLSP:\033[0m Not initialized" << std::endl;
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

    fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", request.length(), request.c_str());
    fflush(impl->input);

    while (true) {
        char header[1024];
        if (!fgets(header, sizeof(header), impl->output)) {
            std::cout << "\033[31mLSP:\033[0m Failed to read response header" << std::endl;
            return false;
        }

        int content_length = 0;
        if (sscanf(header, "Content-Length: %d\r\n", &content_length) != 1) {
            continue;
        }

        fgets(header, sizeof(header), impl->output);

        std::vector<char> buffer(content_length + 1);
        size_t bytes_read = fread(buffer.data(), 1, content_length, impl->output);
        buffer[bytes_read] = '\0';

        std::string response(buffer.data());

        // Only parse responses that:
        // 1. Match our request ID
        // 2. Are definition responses (have result array)
        if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos && response.find("\"result\":[{") != std::string::npos) {

            /*
            std::cout << "\033[32mLSP:\033[0m Definition response:" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            std::cout << buffer.data() << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            */
            // Parse response and show options window
            parseDefinitionResponse(response);
            return true;
        }
    }

    return false;
}

void EditorLSP::parseDefinitionResponse(const std::string &response)
{
    // std::cout << "\033[35mLSP Parse:\033[0m Starting parse of definition response" << std::endl;

    // Clear previous locations
    definitionLocations.clear();

    // Find result array
    size_t resultPos = response.find("\"result\":[");
    if (resultPos == std::string::npos) {
        // std::cout << "\033[31mLSP Parse:\033[0m No result array found in response" << std::endl;
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
        // std::cout << "\033[35mLSP Parse:\033[0m Found URI: " << uri << std::endl;

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

        // Handle Enter/Escape
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            const auto &selected = definitionLocations[selectedDefinitionIndex];
            std::cout << "Selected definition at " << selected.uri << " line " << (selected.startLine + 1) << " char " << (selected.startChar + 1) << std::endl;

            if (selected.uri != gFileExplorer.getCurrentFile()) {
                // Empty callback just like bookmarks
                auto emptyCallback = []() {};
                gFileExplorer.loadFileContent(selected.uri, emptyCallback);
            }

            // Calculate cursor position AFTER file load just like bookmarks does
            int index = 0;
            int currentLine = 0;
            while (currentLine < selected.startLine && index < gFileExplorer.fileContent.length()) {
                if (gFileExplorer.fileContent[index] == '\n') {
                    currentLine++;
                }
                index++;
            }
            index += selected.startChar;

            editor_state.cursor_index = index;
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
