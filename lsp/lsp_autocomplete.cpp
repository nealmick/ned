// lsp_autocomplete.cpp
#include "lsp_autocomplete.h"
#include "../editor/editor.h"
#include "../editor/editor_cursor.h"
#include "lib/json.hpp"
#include "lsp.h"
#include "lsp_manager.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

LSPAutocomplete gLSPAutocomplete;
bool LSPAutocomplete::wasShowingLastFrame = false;

LSPAutocomplete::LSPAutocomplete() = default;
LSPAutocomplete::~LSPAutocomplete() = default;

bool LSPAutocomplete::shouldRender()
{
    if (!showCompletions || currentCompletionItems.empty()) {
        if (editor_state.block_input) {
            editor_state.block_input = false;
        }
        wasShowingLastFrame = false;
        return false;
    }
    return true;
}

bool LSPAutocomplete::handleInputAndCheckClose()
{
    if (!editor_state.block_input) {
        // std::cout << "[renderCompletions] Showing, setting block_input = true" << std::endl; // Optional debug
    }
    editor_state.block_input = true;

    bool closeAndUnblock = false;
    bool navigationKeyPressed = false;

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        std::cout << "[renderCompletions] Escape pressed, hiding completions." << std::endl;
        closeAndUnblock = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (!currentCompletionItems.empty()) {
            // Only decrement if not already at the top (index 0)
            if (selectedCompletionIndex > 0) {
                selectedCompletionIndex--;
            }
        }
        navigationKeyPressed = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (!currentCompletionItems.empty()) {
            // Only increment if not already at the bottom
            if (selectedCompletionIndex < currentCompletionItems.size() - 1) {
                selectedCompletionIndex++;
            }
        }
        navigationKeyPressed = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        if (selectedCompletionIndex >= 0 && selectedCompletionIndex < currentCompletionItems.size()) {
            blockEnter = true;
            const auto &selected_item = currentCompletionItems[selectedCompletionIndex];
            std::cout << "[renderCompletions] Selected (Enter): " << selected_item.insertText << std::endl;
            // TODO: Insert text
        }
        closeAndUnblock = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (selectedCompletionIndex >= 0 && selectedCompletionIndex < currentCompletionItems.size()) {
            blockTab = true;
            const auto &selected_item = currentCompletionItems[selectedCompletionIndex];
            std::cout << "[renderCompletions] Selected (Tab): " << selected_item.insertText << std::endl;
            // TODO: Insert text
        }
        std::cout << "[renderCompletions] Tab pressed, hiding completions." << std::endl;
        closeAndUnblock = true;
    }

    if (!closeAndUnblock && !navigationKeyPressed) {
        ImGuiIO &io = ImGui::GetIO();
        if (io.InputQueueCharacters.Size > 0) {
            std::cout << "[renderCompletions] Character key pressed, hiding completions." << std::endl;
            closeAndUnblock = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            std::cout << "[renderCompletions] Left Arrow pressed, hiding completions." << std::endl;
            closeAndUnblock = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            std::cout << "[renderCompletions] Right Arrow pressed, hiding completions." << std::endl;
            closeAndUnblock = true;
        }
    }

    // --- Process Closing Action ---
    if (closeAndUnblock) {
        editor_state.block_input = false;
        showCompletions = false;
        wasShowingLastFrame = false;
        // std::cout << "[renderCompletions] Closing: Set block_input = false" << std::endl; // Optional debug
        return true;
    }

    return false;
}

void LSPAutocomplete::calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos)
{
    const int current_item_count = currentCompletionItems.size();
    const float item_height = ImGui::GetTextLineHeightWithSpacing();
    const float window_padding = 5.0f;
    const float desired_width = 300.0f;
    const float max_visible_items = 10.0f;

    float current_list_height = std::min((float)current_item_count, max_visible_items) * item_height;
    outWindowSize = ImVec2(desired_width, current_list_height + window_padding * 2.0f);

    ImVec2 calculated_popup_anchor_pos = completionPopupPos;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    outSafePos = calculated_popup_anchor_pos;
    float editor_char_height = editor_state.line_height;
    outSafePos.y += editor_char_height;

    if ((outSafePos.y + outWindowSize.y) > (viewport->Pos.y + viewport->Size.y - 5.0f)) {
        outSafePos.y = calculated_popup_anchor_pos.y - outWindowSize.y - 2.0f;
    }
    if ((outSafePos.x + outWindowSize.x) > (viewport->Pos.x + viewport->Size.x - 5.0f)) {
        outSafePos.x = calculated_popup_anchor_pos.x - outWindowSize.x;
    }
    if (outSafePos.x < (viewport->Pos.x + 5.0f)) {
        outSafePos.x = viewport->Pos.x + 5.0f;
    }
    if (outSafePos.y < (viewport->Pos.y + 5.0f)) {
        outSafePos.y = viewport->Pos.y + 5.0f;
    }
}

void LSPAutocomplete::applyStyling()
{
    const float window_padding = 5.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
}

void LSPAutocomplete::renderCompletionListItems()
{
    const int current_item_count = currentCompletionItems.size();
    const float max_visible_items = 10.0f;
    const float item_height = ImGui::GetTextLineHeightWithSpacing(); // Includes spacing
    const ImGuiStyle &style = ImGui::GetStyle();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImU32 selection_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Header]);
    const ImU32 text_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    bool use_child_window = current_item_count > max_visible_items;

    bool scroll_to_top = use_child_window && showCompletions && !wasShowingLastFrame;

    bool selection_changed_by_keyboard = ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow);

    if (use_child_window) {
        float child_height = max_visible_items * item_height;
        ImGui::BeginChild("##CompletionScroll", ImVec2(350.0f, child_height), false, 0);
        if (scroll_to_top) {
            ImGui::SetScrollY(0.0f);
        }
    }

    for (size_t i = 0; i < currentCompletionItems.size(); ++i) {
        const auto &item = currentCompletionItems[i];
        bool is_selected = (selectedCompletionIndex == i);

        ImVec2 item_pos = ImGui::GetCursorScreenPos();
        float item_width = ImGui::GetContentRegionAvail().x;
        float adjusted_item_width = item_width - (use_child_window ? style.ScrollbarSize : 0.0f);
        adjusted_item_width = std::max(1.0f, adjusted_item_width);
        ImVec2 item_rect_min = item_pos;
        ImVec2 item_rect_max = ImVec2(item_pos.x + adjusted_item_width, item_pos.y + item_height);

        ImGui::Dummy(ImVec2(0.0f, item_height));

        if (!ImGui::IsRectVisible(item_pos, item_rect_max))
            continue;

        if (is_selected) {
            draw_list->AddRectFilled(item_rect_min, item_rect_max, selection_color);
        }

        ImVec2 text_size = ImGui::CalcTextSize(item.insertText.c_str());
        float text_padding_y = (item_height - text_size.y) * 0.5f;
        text_padding_y = std::max(0.0f, text_padding_y);
        ImVec2 text_pos = ImVec2(item_rect_min.x + style.FramePadding.x, item_rect_min.y + text_padding_y);
        draw_list->AddText(text_pos, text_color, item.insertText.c_str());

        if (is_selected && selection_changed_by_keyboard) {
            ImGui::SetScrollHereY(0.5f);
        }
    }

    if (use_child_window) {
        ImGui::EndChild();
    }
}
bool LSPAutocomplete::handleClickOutside()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            std::cout << "[renderCompletions] Clicked outside, hiding." << std::endl;
            showCompletions = false;
            editor_state.block_input = false; // Ensure unblocked
            wasShowingLastFrame = false;      // Reset state
            return true;                      // Indicate closed by click outside
        }
    }
    return false;
}

void LSPAutocomplete::finalizeRenderState()
{
    if (!showCompletions && editor_state.block_input) {
        editor_state.block_input = false;
        std::cout << "[renderCompletions] Closed inside Begin/End, ensuring input unblocked." << std::endl;
    }
    wasShowingLastFrame = showCompletions;
}

void LSPAutocomplete::renderCompletions()
{
    if (!shouldRender()) {
        return;
    }

    if (handleInputAndCheckClose()) {
        return;
    }

    ImVec2 windowSize, safePos;
    calculateWindowGeometry(windowSize, safePos);

    if (showCompletions && !wasShowingLastFrame) {
        ImGui::SetNextWindowFocus();
    }

    ImGui::SetNextWindowPos(safePos);
    ImGui::SetNextWindowSize(windowSize);

    applyStyling(); // Now pushes 3 Colors, 3 Vars

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##CompletionPopupActual", nullptr, windowFlags)) {
        renderCompletionListItems(); // Pushes/Pops 1 Color internally
        handleClickOutside();
        ImGui::End();
    } else {
        if (editor_state.block_input) {
            editor_state.block_input = false;
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
    finalizeRenderState();
}

void LSPAutocomplete::requestCompletion(const std::string &filePath, int line, int character)
{
    if (!gLSPManager.isInitialized()) {
        std::cout << "\033[31mLSP Autocomplete:\033[0m Not initialized" << std::endl;
        return;
    }

    if (!gLSPManager.selectAdapterForFile(filePath)) {
        std::cout << "\033[31mLSP Autocomplete:\033[0m No LSP adapter available for file: " << filePath << std::endl;
        return;
    }

    // *** Use the global ID generator from gEditorLSP ***
    int requestId = gEditorLSP.getNextRequestId();
    std::cout << "\033[35mLSP Autocomplete:\033[0m Requesting completions at line " << line << ", char " << character << " (ID: " << requestId << ")" << std::endl;

    // Prepare JSON-RPC request (line/character are 0-based in LSP)
    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )") + std::to_string(requestId) +
                          R"(,
        "method": "textDocument/completion",
        "params": {
            "textDocument": {
                "uri": "file://)" +
                          filePath + R"("
            },
            "position": {
                "line": )" +
                          std::to_string(line) + R"(,
                "character": )" +
                          std::to_string(character) + R"(
            },
            "context": {
                "triggerKind": 2
            }
        }
    })";

    if (!gLSPManager.sendRequest(request)) {
        std::cout << "\033[31mLSP Autocomplete:\033[0m Failed to send request" << std::endl;
        return;
    }
    const int MAX_ATTEMPTS = 15;
    const int WAIT_MS = 50; // Or your preferred wait time
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength);
        if (response.empty()) {
            if (attempt == 0) {
                std::cout << "\033[35mLSP Autocomplete:\033[0m Waiting..." << std::endl;
            }
            continue;
        }
        try {
            json j = json::parse(response);
            if (!j.contains("id") || !j["id"].is_number_integer() || j["id"].get<int>() != requestId) {
                continue; // Skip other responses
            }

            std::cout << "\033[32mLSP Autocomplete:\033[0m Received response for ID " << requestId << std::endl;

            if (j.contains("error")) {
                std::cerr << "\033[31mLSP Autocomplete:\033[0m Error: " << j["error"].dump(2) << std::endl;
                currentCompletionItems.clear(); // Clear items on error
                showCompletions = false;
                return;
            }

            if (j.contains("result")) {
                json result = j["result"];
                std::vector<json> items_json; // Temporary vector to hold json items
                bool is_incomplete = false;

                // --- Extract the list of items from JSON ---
                if (result.is_array()) {
                    items_json = result.get<std::vector<json>>();
                } else if (result.is_object()) {
                    if (result.contains("items") && result["items"].is_array()) {
                        items_json = result["items"].get<std::vector<json>>();
                    }
                    is_incomplete = result.value("isIncomplete", false); // Check if list is incomplete
                } else if (result.is_null()) {
                    std::cout << "\033[33mLSP Autocomplete:\033[0m No completions found (result is null)." << std::endl;
                    currentCompletionItems.clear(); // Clear just in case
                    showCompletions = false;
                    return; // Exit successfully, nothing to show
                } else {
                    std::cout << "\033[31mLSP Autocomplete:\033[0m Unexpected result format: " << result.type_name() << std::endl;
                    currentCompletionItems.clear();
                    showCompletions = false;
                    return;
                }

                std::cout << "\033[32mFound " << items_json.size() << " completions" << (is_incomplete ? " (incomplete list)" : "") << ":\033[0m" << std::endl;

                // --- Clear previous items and parse new ones ---
                currentCompletionItems.clear();
                currentCompletionItems.reserve(items_json.size()); // Optional optimization

                for (const auto &item_json : items_json) {
                    CompletionDisplayItem newItem;

                    // Extract label (required)
                    newItem.label = item_json.value("label", "[No Label]");

                    // Extract detail (optional)
                    newItem.detail = item_json.value("detail", "");

                    // Extract kind (optional, default to 0 or a 'Text' kind if defined)
                    newItem.kind = item_json.value("kind", 0); // 0 is often 'Unknown' or not specified

                    // Determine insertText (using LSP priority: textEdit > insertText > label)
                    if (item_json.contains("textEdit") && item_json["textEdit"].is_object() && item_json["textEdit"].contains("newText") && item_json["textEdit"]["newText"].is_string()) {
                        newItem.insertText = item_json["textEdit"]["newText"].get<std::string>();
                    } else if (item_json.contains("insertText") && item_json["insertText"].is_string()) {
                        newItem.insertText = item_json["insertText"].get<std::string>();
                    } else // Fallback to label
                    {
                        newItem.insertText = newItem.label;
                    }

                    currentCompletionItems.push_back(newItem);
                }

                if (!currentCompletionItems.empty()) {
                    try {
                        int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
                        float cursor_x = gEditorCursor.getCursorXPosition(editor_state.text_pos, editor_state.fileContent, editor_state.cursor_index);
                        completionPopupPos = editor_state.text_pos;
                        completionPopupPos.x = cursor_x;
                        completionPopupPos.y += cursor_line * editor_state.line_height;
                        std::cout << ">>> [requestCompletion] Stored Anchor Pos: (" << completionPopupPos.x << ", " << completionPopupPos.y << ")" << std::endl;
                    } catch (const std::exception &e) {
                        std::cerr << "!!! ERROR calculating cursor pos in requestCompletion: " << e.what() << std::endl;
                        completionPopupPos = ImVec2(0, 0); // Fallback on error
                    }

                    showCompletions = true;
                    selectedCompletionIndex = 0; // Reset selection to the top

                } else {
                    showCompletions = false;
                }

                return; // Success (handled response)
            } else {
                std::cout << "\033[31mLSP Autocomplete:\033[0m Response missing 'result' field." << std::endl;
                currentCompletionItems.clear();
                showCompletions = false;
                return;
            }

        } catch (const json::exception &e) {
            std::cerr << "\033[31mLSP Autocomplete:\033[0m JSON error: " << e.what() << std::endl;
            currentCompletionItems.clear();
            showCompletions = false;
            return;
        }
    }
    std::cout << "\033[31mLSP Autocomplete:\033[0m Timed out waiting for response ID " << requestId << "." << std::endl;
}