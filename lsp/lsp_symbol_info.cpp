#include "lsp_symbol_info.h"
#include "../editor/editor.h"
#include <iostream>

LSPSymbolInfo gLSPSymbolInfo;

LSPSymbolInfo::LSPSymbolInfo() = default;

void LSPSymbolInfo::fetchSymbolInfo(const std::string &filePath)
{
    int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

    int line = editor_state.editor_content_lines[current_line];
    int character = editor_state.cursor_index - line;

    if (!gLSPManager.isInitialized()) {
        std::cout << "\033[31mLSP SymbolInfo:\033[0m LSP not initialized" << std::endl;
        return;
    }

    static int requestId = 5000;
    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(requestId++) +
                                      R"(,
        "method": "textDocument/hover",
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

    if (!gLSPManager.sendRequest(request)) {
        std::cout << "\033[31mLSP SymbolInfo:\033[0m Failed to send hover request" << std::endl;
        return;
    }

    // Store cursor position for display
    displayPosition = ImGui::GetMousePos();
    displayPosition.x += 20; // Offset from cursor

    // Read response with timeout
    const int MAX_ATTEMPTS = 10;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength);

        if (response.find("\"id\":" + std::to_string(requestId - 1)) != std::string::npos) {
            parseHoverResponse(response);
            return;
        }
    }
}

void LSPSymbolInfo::parseHoverResponse(const std::string &response)
{
    currentSymbolInfo.clear();
    showSymbolInfo = false;

    try {
        json j = json::parse(response);
        std::cout << j << std::endl;
        if (j.contains("result") && j["result"].is_object()) {
            auto result = j["result"];
            if (result.contains("contents")) {
                if (result["contents"].is_string()) {
                    currentSymbolInfo = result["contents"].get<std::string>();
                } else if (result["contents"].is_array()) {
                    for (auto &item : result["contents"]) {
                        if (item.is_string()) {
                            currentSymbolInfo += item.get<std::string>() + "\n";
                        }
                    }
                }

                if (!currentSymbolInfo.empty()) {
                    showSymbolInfo = true;
                    return;
                }
            }
        }

        std::cout << "\033[33mLSP SymbolInfo:\033[0m No hover information found" << std::endl;
    } catch (const json::exception &e) {
        std::cerr << "\033[31mLSP SymbolInfo:\033[0m JSON error: " << e.what() << std::endl;
    }
}

void LSPSymbolInfo::renderSymbolInfo()
{
    if (!hasSymbolInfo())
        return;

    ImGui::SetNextWindowPos(displayPosition, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

    if (ImGui::Begin("Symbol Info", &showSymbolInfo, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
        // Handle close on click outside or escape
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showSymbolInfo = false;
        }

        ImGui::TextWrapped("%s", currentSymbolInfo.c_str());
        ImGui::End();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}