// Windows stub implementations for LSP functionality
// Most LSP features are disabled on Windows due to Unix-specific process management requirements
// However, basic Python support via pyright is implemented using Windows-native LSP manager

#include "lsp.h"
#include "lsp_autocomplete.h"
#include "lsp_goto_def.h"
#include "lsp_goto_ref.h"
#include "lsp_symbol_info.h"
#include "lsp_manager_windows.h"
#include "../editor/editor.h"
#include "../editor/editor_types.h"
#include "../editor/editor_scroll.h"
#include "../files/files.h"
#include "../globals.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <windows.h>

// Global instances - these need to exist for linking
EditorLSP gEditorLSP;
LSPAutocomplete gLSPAutocomplete;
LSPGotoDef gLSPGotoDef;
LSPGotoRef gLSPGotoRef;
LSPSymbolInfo gLSPSymbolInfo;

// Static member definition
bool LSPAutocomplete::wasShowingLastFrame = false;

// EditorLSP stub implementation
EditorLSP::EditorLSP() {}
EditorLSP::~EditorLSP() {}

bool EditorLSP::initialize(const std::string &workspacePath) {
    std::cout << "LSP functionality is disabled on Windows" << std::endl;
    return false;
}

void EditorLSP::didOpen(const std::string &filePath, const std::string &content) {
    // Stub - no action on Windows
}

void EditorLSP::didChange(const std::string &filePath, int version) {
    // Stub - no action on Windows
}

std::string EditorLSP::escapeJSON(const std::string &s) const {
    return s;
}

// LSPAutocomplete stub implementation
LSPAutocomplete::LSPAutocomplete() {}
LSPAutocomplete::~LSPAutocomplete() {}

void LSPAutocomplete::requestCompletion(const std::string &filePath, int line, int character) {
    // Stub - no action on Windows
}

void LSPAutocomplete::renderCompletions() {
    // Stub - no action on Windows
}

void LSPAutocomplete::processPendingResponses() {
    // Stub - no action on Windows
}

bool LSPAutocomplete::shouldRender() {
    return false;
}

bool LSPAutocomplete::handleInputAndCheckClose() {
    return true;
}

void LSPAutocomplete::calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos) {
    // Stub - no action on Windows
}

void LSPAutocomplete::applyStyling() {
    // Stub - no action on Windows
}

void LSPAutocomplete::renderCompletionListItems() {
    // Stub - no action on Windows
}

bool LSPAutocomplete::handleClickOutside() {
    return false;
}

void LSPAutocomplete::finalizeRenderState() {
    // Stub - no action on Windows
}

void LSPAutocomplete::resetPopupPosition() {
    // Stub - no action on Windows
}

std::string LSPAutocomplete::formCompletionRequest(int requestId, const std::string &filePath, int line, int character) {
    return "";
}

bool LSPAutocomplete::processResponse(const std::string &response, int requestId) {
    return false;
}

void LSPAutocomplete::parseCompletionResult(const json &result, int requestLine, int requestCharacter) {
    // Stub - no action on Windows
}

void LSPAutocomplete::updatePopupPosition() {
    // Stub - no action on Windows
}

void LSPAutocomplete::workerFunction() {
    // Stub - no action on Windows
}

void LSPAutocomplete::insertText(int row_start, int col_start, int row_end, int col_end, std::string text) {
    // Stub - no action on Windows
}

// LSPGotoDef implementation now in main lsp_goto_def.cpp for cross-platform support

// LSPGotoDef::gotoDefinition now implemented in main lsp_goto_def.cpp for cross-platform support
        return false;
    }

    if (!gLSPManagerWindows.isInitialized()) {
        // Extract workspace path from file path
        std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
        std::cout << "\033[35mLSP GotoDef Windows:\033[0m Auto-initializing with workspace: " << workspacePath << std::endl;
        
        if (!gLSPManagerWindows.initialize(workspacePath)) {
            std::cout << "\033[31mLSP GotoDef Windows:\033[0m Failed to initialize LSP" << std::endl;
            return false;
        }
    }

    static int requestId = 6000;
    int currentRequestId = requestId++;
    
    // Convert Windows path to proper URI format
    std::string fileURI = "file:///" + filePath;
    // Replace backslashes with forward slashes
    for (char &c : fileURI) {
        if (c == '\\') c = '/';
    }

    // Track which files we've sent didOpen notifications for to avoid duplicates
    static std::set<std::string> openedFiles;
    
    // Send didOpen notification only if we haven't opened this file yet
    if (openedFiles.find(fileURI) == openedFiles.end()) {
        // Read the file content
        std::ifstream fileStream(filePath);
        std::string fileContent;
        if (fileStream.is_open()) {
            std::ostringstream contentStream;
            contentStream << fileStream.rdbuf();
            fileContent = contentStream.str();
            fileStream.close();
        } else {
            std::cout << "\033[31mLSP GotoDef Windows:\033[0m Failed to read file: " << filePath << std::endl;
            return false;
        }

        // Escape the file content for JSON
        std::string escapedContent;
        for (char c : fileContent) {
            switch (c) {
                case '"': escapedContent += "\\\""; break;
                case '\\': escapedContent += "\\\\"; break;
                case '\n': escapedContent += "\\n"; break;
                case '\r': escapedContent += "\\r"; break;
                case '\t': escapedContent += "\\t"; break;
                default: escapedContent += c; break;
            }
        }

        // Send didOpen notification
        std::string didOpenRequest = R"({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": ")" + fileURI + R"(",
                    "languageId": "python",
                    "version": 1,
                    "text": ")" + escapedContent + R"("
                }
            }
        })";

        std::cout << "\033[35mLSP GotoDef Windows:\033[0m Sending didOpen notification for file" << std::endl;
        if (!gLSPManagerWindows.sendRequest(didOpenRequest)) {
            std::cout << "\033[31mLSP GotoDef Windows:\033[0m Failed to send didOpen notification\n";
            return false;
        }
        
        // Mark this file as opened
        openedFiles.insert(fileURI);
        
        // Wait a bit for the file to be processed
        Sleep(100);
    } else {
        std::cout << "\033[33mLSP GotoDef Windows:\033[0m File already opened, skipping didOpen" << std::endl;
    }

    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )") + std::to_string(currentRequestId) + R"(,
        "method": "textDocument/definition",
        "params": {
            "textDocument": {
                "uri": ")" + fileURI + R"("
            },
            "position": {
                "line": )" + std::to_string(line) + R"(,
                "character": )" + std::to_string(character) + R"(
            }
        }
    })";

    std::cout << "\033[36mLSP GotoDef Windows Request:\033[0m\n" << request << "\n";

    if (!gLSPManagerWindows.sendRequest(request)) {
        std::cout << "\033[31mLSP GotoDef Windows:\033[0m Failed to send definition request\n";
        return false;
    }

    // Response handling with timeout
    const int MAX_ATTEMPTS = 15;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        int contentLength = 0;
        std::string response = gLSPManagerWindows.readResponse(&contentLength);

        if (!response.empty()) {
            std::cout << "\033[36mLSP GotoDef Windows Response:\033[0m\n" << response << "\n";
        }

        if (response.find("\"id\":" + std::to_string(currentRequestId)) != std::string::npos) {
            parseDefinitionResponse(response);
            return true; // We got a response, even if no definitions found
        }
        
        // Small delay between attempts
        Sleep(50);
    }
    
    std::cout << "\033[33mLSP GotoDef Windows:\033[0m No response received after " << MAX_ATTEMPTS << " attempts\n";
    return false;
}

// LSPGotoDef render methods now implemented in main lsp_goto_def.cpp for cross-platform support

// LSPGotoDef::parseDefinitionResponse now implemented in main lsp_goto_def.cpp for cross-platform support
    
    try {
        json j = json::parse(response);
        std::cout << "\033[36mRaw Definition JSON Response Windows:\033[0m\n" << j.dump(4) << "\n";

        if (j.contains("result")) {
            auto result = j["result"];

            // Handle the case where result is null (no definition found)
            if (result.is_null()) {
                std::cout << "\033[33mLSP GotoDef Windows:\033[0m No definition found" << std::endl;
                return;
            }

            // Handle array of Location objects (most common case)
            if (result.is_array()) {
                if (result.empty()) {
                    std::cout << "\033[33mLSP GotoDef Windows:\033[0m No definitions in response array" << std::endl;
                    return;
                }

                // Process all definitions for popup display
                for (const auto& def : result) {
                    if (def.contains("uri") && def.contains("range")) {
                        std::string uri = def["uri"].get<std::string>();
                        auto range = def["range"];
                        auto start = range["start"];
                        auto end = range["end"];
                        int defLine = start["line"].get<int>();
                        int defChar = start["character"].get<int>();
                        int endLine = end["line"].get<int>();
                        int endChar = end["character"].get<int>();

                        // Convert file:/// URI back to Windows path
                        std::string filePath = uri;
                        if (filePath.substr(0, 8) == "file:///") {
                            filePath = filePath.substr(8); // Remove file:///
                            
                            // Decode URL-encoded characters (e.g., %3A -> :)
                            std::string decodedPath;
                            for (size_t i = 0; i < filePath.length(); ++i) {
                                if (filePath[i] == '%' && i + 2 < filePath.length()) {
                                    // Convert hex to character
                                    std::string hexStr = filePath.substr(i + 1, 2);
                                    char decodedChar = static_cast<char>(std::stoi(hexStr, nullptr, 16));
                                    decodedPath += decodedChar;
                                    i += 2; // Skip the hex digits
                                } else {
                                    decodedPath += filePath[i];
                                }
                            }
                            filePath = decodedPath;
                            
                            // Convert forward slashes back to backslashes for Windows
                            for (char &c : filePath) {
                                if (c == '/') c = '\\';
                            }
                        }

                        // Add to definitions list for popup
                        definitionLocations.push_back({filePath, defLine, defChar, endLine, endChar});
                        std::cout << "\033[32mLSP GotoDef Windows:\033[0m Added definition: " << filePath 
                                  << " line " << (defLine + 1) << " char " << (defChar + 1) << std::endl;
                    }
                }
                
                // Show popup if definitions were found
                if (!definitionLocations.empty()) {
                    showDefinitionOptions = true;
                    selectedDefinitionIndex = 0;
                    std::cout << "\033[32mLSP GotoDef Windows:\033[0m Found " << definitionLocations.size() 
                              << " definition(s), showing popup" << std::endl;
                } else {
                    std::cout << "\033[33mLSP GotoDef Windows:\033[0m No valid definitions found" << std::endl;
                }
                return; // Done processing array
            }
            // Handle single Location object
            else if (result.is_object() && result.contains("uri") && result.contains("range")) {
                std::string uri = result["uri"].get<std::string>();
                auto range = result["range"];
                auto start = range["start"];
                auto end = range["end"];
                int defLine = start["line"].get<int>();
                int defChar = start["character"].get<int>();
                int endLine = end["line"].get<int>();
                int endChar = end["character"].get<int>();

                // Convert file:/// URI back to Windows path
                std::string filePath = uri;
                if (filePath.substr(0, 8) == "file:///") {
                    filePath = filePath.substr(8); // Remove file:///
                    
                    // Decode URL-encoded characters (e.g., %3A -> :)
                    std::string decodedPath;
                    for (size_t i = 0; i < filePath.length(); ++i) {
                        if (filePath[i] == '%' && i + 2 < filePath.length()) {
                            // Convert hex to character
                            std::string hexStr = filePath.substr(i + 1, 2);
                            char decodedChar = static_cast<char>(std::stoi(hexStr, nullptr, 16));
                            decodedPath += decodedChar;
                            i += 2; // Skip the hex digits
                        } else {
                            decodedPath += filePath[i];
                        }
                    }
                    filePath = decodedPath;
                    
                    // Convert forward slashes back to backslashes for Windows
                    for (char &c : filePath) {
                        if (c == '/') c = '\\';
                    }
                }

                // Add single definition to popup
                definitionLocations.push_back({filePath, defLine, defChar, endLine, endChar});
                showDefinitionOptions = true;
                selectedDefinitionIndex = 0;
                std::cout << "\033[32mLSP GotoDef Windows:\033[0m Found single definition, showing popup" << std::endl;

                return; // Popup will handle navigation
            }
        }

        if (j.contains("error")) {
            std::cerr << "\033[31mLSP GotoDef Error Windows:\033[0m " << j["error"].dump() << "\n";
        }

        std::cout << "\033[33mLSP GotoDef Windows:\033[0m No valid definition found in response\033[0m\n";
    } catch (const json::exception &e) {
        std::cerr << "\033[31mJSON Parsing Error Windows:\033[0m " << e.what()
                  << "\nResponse Data:\n" << response << "\n";
    }
}

// LSPGotoDef::parseDefinitionArray now implemented in main lsp_goto_def.cpp for cross-platform support

// LSPGotoRef stub implementation
LSPGotoRef::LSPGotoRef() {}
LSPGotoRef::~LSPGotoRef() {}

bool LSPGotoRef::findReferences(const std::string &filePath, int line, int character) {
    return false;
}

void LSPGotoRef::renderReferenceOptions() {
    // Stub - no action on Windows
}

bool LSPGotoRef::hasReferenceOptions() const {
    return false;
}

void LSPGotoRef::parseReferenceResponse(const std::string &response) {
    // Stub - no action on Windows
}

void LSPGotoRef::handleReferenceSelection() {
    // Stub - no action on Windows
}

// LSPSymbolInfo Windows implementation (Python support only)
LSPSymbolInfo::LSPSymbolInfo() = default;

void LSPSymbolInfo::fetchSymbolInfo(const std::string &filePath) {
    // Clear any previous symbol info to ensure we can process new requests
    currentSymbolInfo.clear();
    showSymbolInfo = false;
    
    // Check if this is a Python file
    size_t dot_pos = filePath.find_last_of(".");
    if (dot_pos == std::string::npos) {
        std::cout << "\033[31mDEBUG:\033[0m No file extension found" << std::endl;
        return; // No extension
    }

    std::string ext = filePath.substr(dot_pos + 1);
    std::cout << "\033[31mDEBUG:\033[0m File extension: " << ext << std::endl;
    
    if (ext != "py") {
        std::cout << "\033[31mDEBUG:\033[0m Not a Python file, skipping LSP" << std::endl;
        // Only support Python files for now
        return;
    }
    
    std::cout << "\033[31mDEBUG:\033[0m Python file detected, proceeding with LSP" << std::endl;

    // Get current cursor position in editor
    int cursor_pos = editor_state.cursor_index;

    // Convert to line number and character offset
    int current_line = gEditor.getLineFromPos(cursor_pos);
    int line_start = editor_state.editor_content_lines[current_line];
    int character = cursor_pos - line_start;

    // LSP uses 0-based line numbers
    int lsp_line = current_line;
    int lsp_char = character;

    std::cout << "\033[35mLSP SymbolInfo Windows:\033[0m Requesting hover at "
              << "Line: " << lsp_line << " (" << current_line + 1 << "), "
              << "Char: " << lsp_char << " (abs pos: " << cursor_pos << ")\n";

    // Select adapter for this file and initialize if needed
    if (!gLSPManagerWindows.selectAdapterForFile(filePath)) {
        std::cout << "\033[33mLSP SymbolInfo Windows:\033[0m No adapter available for file: " << filePath << std::endl;
        return;
    }

    if (!gLSPManagerWindows.isInitialized()) {
        // Extract workspace path from file path (use directory containing the file)
        std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
        std::cout << "\033[35mLSP SymbolInfo Windows:\033[0m Auto-initializing with workspace: " << workspacePath << std::endl;
        
        if (!gLSPManagerWindows.initialize(workspacePath)) {
            std::cout << "\033[31mLSP SymbolInfo Windows:\033[0m Failed to initialize LSP" << std::endl;
            return;
        }
    }

    static int requestId = 5000;
    int currentRequestId = requestId++;
    
    // Convert Windows path to proper URI format
    std::string fileURI = "file:///" + filePath;
    // Replace backslashes with forward slashes
    for (char &c : fileURI) {
        if (c == '\\') c = '/';
    }

    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(currentRequestId) + R"(,
        "method": "textDocument/hover",
        "params": {
            "textDocument": {
                "uri": ")" + fileURI + R"("
            },
            "position": {
                "line": )" + std::to_string(lsp_line) + R"(,
                "character": )" + std::to_string(lsp_char) + R"(
            }
        }
    })");

    // Track which files we've sent didOpen notifications for to avoid duplicates
    static std::set<std::string> openedFiles;
    
    // Send didOpen notification only if we haven't opened this file yet
    if (openedFiles.find(fileURI) == openedFiles.end()) {
        // Read the file content
        std::ifstream fileStream(filePath);
        std::string fileContent;
        if (fileStream.is_open()) {
            std::ostringstream contentStream;
            contentStream << fileStream.rdbuf();
            fileContent = contentStream.str();
            fileStream.close();
        } else {
            std::cout << "\033[31mLSP SymbolInfo Windows:\033[0m Failed to read file: " << filePath << std::endl;
            return;
        }

        // Escape the file content for JSON
        std::string escapedContent;
        for (char c : fileContent) {
            switch (c) {
                case '"': escapedContent += "\\\""; break;
                case '\\': escapedContent += "\\\\"; break;
                case '\n': escapedContent += "\\n"; break;
                case '\r': escapedContent += "\\r"; break;
                case '\t': escapedContent += "\\t"; break;
                default: escapedContent += c; break;
            }
        }

        // Send didOpen notification
        std::string didOpenRequest = R"({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": ")" + fileURI + R"(",
                    "languageId": "python",
                    "version": 1,
                    "text": ")" + escapedContent + R"("
                }
            }
        })";

        std::cout << "\033[35mLSP SymbolInfo Windows:\033[0m Sending didOpen notification for file" << std::endl;
        if (!gLSPManagerWindows.sendRequest(didOpenRequest)) {
            std::cout << "\033[31mLSP SymbolInfo Windows:\033[0m Failed to send didOpen notification\n";
            return;
        }
        
        // Mark this file as opened
        openedFiles.insert(fileURI);
        
        // Wait a bit for the file to be processed
        Sleep(100);
    } else {
        std::cout << "\033[33mLSP SymbolInfo Windows:\033[0m File already opened, skipping didOpen" << std::endl;
    }

    std::cout << "\033[36mLSP SymbolInfo Windows Request:\033[0m\n" << request << "\n";

    if (!gLSPManagerWindows.sendRequest(request)) {
        std::cout << "\033[31mLSP SymbolInfo Windows:\033[0m Failed to send hover request\n";
        return;
    }

    // Store display position
    displayPosition = ImGui::GetMousePos();
    displayPosition.x += 20;

    // Response handling with timeout
    const int MAX_ATTEMPTS = 15;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        int contentLength = 0;
        std::string response = gLSPManagerWindows.readResponse(&contentLength);

        if (!response.empty()) {
            std::cout << "\033[36mLSP SymbolInfo Windows Response:\033[0m\n" << response << "\n";
        }

        if (response.find("\"id\":" + std::to_string(currentRequestId)) != std::string::npos) {
            parseHoverResponse(response);
            return;
        }
        
        // Small delay between attempts
        Sleep(50);
    }
    
    std::cout << "\033[33mLSP SymbolInfo Windows:\033[0m No response received after " << MAX_ATTEMPTS << " attempts\n";
}

void LSPSymbolInfo::renderSymbolInfo() {
    if (!hasSymbolInfo()) {
        return;
    }

    // Calculate cursor position in screen space
    int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
    float cursor_x = gEditorCursor.getCursorXPosition(editor_state.text_pos,
                                                      editor_state.fileContent,
                                                      editor_state.cursor_index);

    // Get the actual screen position of the text cursor
    ImVec2 cursor_screen_pos = editor_state.text_pos;
    cursor_screen_pos.x = cursor_x;
    cursor_screen_pos.y += cursor_line * editor_state.line_height;

    // Set initial display position relative to cursor
    displayPosition = cursor_screen_pos;
    displayPosition.y += editor_state.line_height + 5.0f; // Position below cursor line
    displayPosition.x += 5.0f;                           // Small horizontal offset

    // Width configuration
    const float min_width = 450.0f;
    const float max_width = 650.0f;
    const float screen_padding = 20.0f;

    // Set initial window position and size constraints
    ImGui::SetNextWindowPos(displayPosition, ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(min_width, 0), // Minimum size
                                        ImVec2(max_width, FLT_MAX) // Maximum size
    );

    // Style settings
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

    // ImGui::Begin() MUST be paired with ImGui::End() regardless of return value
    // DON'T pass &showSymbolInfo to Begin() - it creates persistent focus issues
    bool windowOpen = ImGui::Begin("SymbolInfoWindowWindows",
                                  nullptr,  // Changed from &showSymbolInfo to nullptr
                                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);

    if (windowOpen) {
        // Dismissal interactions - check if user clicked outside the popup
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
                showSymbolInfo = false;
                currentSymbolInfo.clear();
            }
        }

        // Allow Escape key to dismiss
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showSymbolInfo = false;
            currentSymbolInfo.clear();
        }

        // Content rendering
        ImGui::PushTextWrapPos(max_width - 30.0f);
        ImGui::TextWrapped("%s", currentSymbolInfo.c_str());
        ImGui::PopTextWrapPos();
    }
    
    // ALWAYS call End() to match Begin(), regardless of return value
    ImGui::End();

    // Cleanup
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    
}

void LSPSymbolInfo::parseHoverResponse(const std::string &response) {
    currentSymbolInfo.clear();
    showSymbolInfo = false;

    try {
        json j = json::parse(response);
        std::cout << "\033[36mRaw JSON Response Windows:\033[0m\n" << j.dump(4) << "\n";

        if (j.contains("result") && !j["result"].is_null()) {
            auto result = j["result"];

            // Handle different LSP server implementations
            if (result.is_object()) {
                // Pyright/clangd style: {"contents":{"kind":"markdown","value":"..."}}
                if (result.contains("contents")) {
                    auto contents = result["contents"];

                    // Handle markdown content
                    if (contents.is_object() && contents.contains("value")) {
                        currentSymbolInfo = contents["value"].get<std::string>();
                    }
                    // Handle plain string
                    else if (contents.is_string()) {
                        currentSymbolInfo = contents.get<std::string>();
                    }
                    // Handle array of content objects
                    else if (contents.is_array()) {
                        for (auto &item : contents) {
                            if (item.is_object() && item.contains("value")) {
                                currentSymbolInfo += item["value"].get<std::string>() + "\n";
                            } else if (item.is_string()) {
                                currentSymbolInfo += item.get<std::string>() + "\n";
                            }
                        }
                    }
                }
            }
            // Handle Python LSP servers (pyright/pylsp)
            else if (result.is_string()) {
                currentSymbolInfo = result.get<std::string>();
            }

            // Clean up and show if we got content
            if (!currentSymbolInfo.empty()) {
                // Better markdown cleanup - extract content from code blocks and clean up
                std::string cleanedContent;
                
                // Look for code blocks with ```python\n...\n```
                size_t pos = 0;
                while (pos < currentSymbolInfo.length()) {
                    size_t codeBlockStart = currentSymbolInfo.find("```", pos);
                    if (codeBlockStart != std::string::npos) {
                        // Add any text before the code block
                        if (codeBlockStart > pos) {
                            cleanedContent += currentSymbolInfo.substr(pos, codeBlockStart - pos);
                        }
                        
                        // Find the language identifier (e.g., "python")
                        size_t languageStart = codeBlockStart + 3;
                        size_t languageEnd = currentSymbolInfo.find('\n', languageStart);
                        if (languageEnd == std::string::npos) languageEnd = languageStart;
                        
                        // Find the end of the code block
                        size_t codeBlockEnd = currentSymbolInfo.find("```", languageEnd);
                        if (codeBlockEnd != std::string::npos) {
                            // Extract the code content (skip the language line)
                            size_t codeStart = languageEnd;
                            if (codeStart < currentSymbolInfo.length() && currentSymbolInfo[codeStart] == '\n') {
                                codeStart++; // Skip the newline after language
                            }
                            std::string codeContent = currentSymbolInfo.substr(codeStart, codeBlockEnd - codeStart);
                            
                            // Clean up the code content (remove leading/trailing whitespace)
                            size_t contentStart = codeContent.find_first_not_of(" \t\n\r");
                            size_t contentEnd = codeContent.find_last_not_of(" \t\n\r");
                            if (contentStart != std::string::npos && contentEnd != std::string::npos) {
                                cleanedContent += codeContent.substr(contentStart, contentEnd - contentStart + 1);
                            }
                            
                            pos = codeBlockEnd + 3; // Move past the closing ```
                        } else {
                            // No closing ```, just take everything after opening ```
                            cleanedContent += currentSymbolInfo.substr(languageEnd);
                            break;
                        }
                    } else {
                        // No more code blocks, add the rest of the content
                        cleanedContent += currentSymbolInfo.substr(pos);
                        break;
                    }
                }
                
                // If we didn't find any code blocks, use the original content
                if (cleanedContent.empty()) {
                    cleanedContent = currentSymbolInfo;
                }
                
                currentSymbolInfo = cleanedContent;
                showSymbolInfo = true;
                std::cout << "\033[32mProcessed Hover Content Windows:\033[0m\n" << currentSymbolInfo << "\n";
                return;
            }
        }

        if (j.contains("error")) {
            std::cerr << "\033[31mLSP Error Windows:\033[0m " << j["error"].dump() << "\n";
        }

        std::cout << "\033[33mNo hover information found in response Windows\033[0m\n";
    } catch (const json::exception &e) {
        std::cerr << "\033[31mJSON Parsing Error Windows:\033[0m " << e.what()
                  << "\nResponse Data:\n" << response << "\n";
    }
}