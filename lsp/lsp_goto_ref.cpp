
#include "lsp_goto_ref.h"
#include "../editor/editor.h"
#include "../editor/editor_line_jump.h"
#include "files.h"
#include <algorithm> // For std::min
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

// Global instance definition
LSPGotoRef gLSPGotoRef;

LSPGotoRef::LSPGotoRef() : currentRequestId(3000), showReferenceOptions(false), selectedReferenceIndex(0) {}

LSPGotoRef::~LSPGotoRef() = default;

// --- findReferences (Request Sending Logic) ---
// This part remains specific to 'textDocument/references'
bool LSPGotoRef::findReferences(const std::string &filePath, int line, int character)
{
    if (!gLSPManager.isInitialized()) {
        std::cout << "\033[31mLSP FindRef:\033[0m Not initialized" << std::endl;
        return false;
    }

    if (!gLSPManager.selectAdapterForFile(filePath)) {
        std::cout << "\033[31mLSP FindRef:\033[0m No LSP adapter available for file: " << filePath << std::endl;
        return false;
    }

    int requestId = getNextRequestId();
    std::cout << "\033[35mLSP FindRef:\033[0m Requesting references at line " << line << ", char " << character << " (ID: " << requestId << ")" << std::endl;

    std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(requestId) +
                                      R"(,
        "method": "textDocument/references",
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
            },
            "context": {
                "includeDeclaration": false
            }
        }
    })");

    if (!gLSPManager.sendRequest(request)) {
        std::cout << "\033[31mLSP FindRef:\033[0m Failed to send request" << std::endl;
        return false;
    }

    // --- Response Handling Logic (Adapted from LSPGotoDef, checking for correct ID) ---
    const int MAX_ATTEMPTS = 15;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        std::cout << "\033[35mLSP FindRef:\033[0m Waiting for references response (attempt " << (attempt + 1) << ")" << std::endl;

        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength);

        if (response.empty()) {
            std::cout << "\033[31mLSP FindRef:\033[0m Empty response received" << std::endl;
            if (attempt == MAX_ATTEMPTS - 1) {
                std::cout << "\033[31mLSP FindRef:\033[0m Timeout waiting for response." << std::endl;
                return false;
            }
            // usleep(50000); // Optional short delay
            continue;
        }

        // Check if the response ID matches our request
        if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos) {
            std::cout << "\033[32mLSP FindRef:\033[0m Received response for request ID " << requestId << std::endl;

            // References usually return an array (even if empty) or null.
            // Unlike definition, a single object result isn't standard for references.
            if (response.find("\"result\":[") != std::string::npos) {
                std::cout << "\033[32mLSP FindRef:\033[0m Found result array." << std::endl;
                parseReferenceResponse(response); // Parse the array
                return true;
            } else if (response.find("\"result\":null") != std::string::npos) {
                std::cout << "\033[33mLSP FindRef:\033[0m No references found (result is null)." << std::endl;
                referenceLocations.clear(); // Ensure list is empty
                showReferenceOptions = false;
                // We might still want to show an empty state or message? For now, just return true.
                // Optionally: Set a flag to show "No references found" message later.
                return true; // Successfully processed the response (no results)
            } else if (response.find("\"error\":") != std::string::npos) {
                std::cout << "\033[31mLSP FindRef:\033[0m Error in response: " << response << std::endl;
                referenceLocations.clear();
                showReferenceOptions = false;
                return false; // Indicate error
            } else {
                // Got a response with our ID, but unexpected format
                std::cout << "\033[31mLSP FindRef:\033[0m Response for ID " << requestId << " has unexpected format: " << response << std::endl;
                referenceLocations.clear();
                showReferenceOptions = false;
                return false; // Indicate unexpected format
            }
        }

        std::cout << "\033[33mLSP FindRef:\033[0m Received unrelated response. Continuing..." << std::endl;
    }

    std::cout << "\033[31mLSP FindRef:\033[0m Exceeded maximum attempts waiting for response ID " << requestId << std::endl;
    return false;
}
// --- parseReferenceResponse (Using nlohmann::json) ---
void LSPGotoRef::parseReferenceResponse(const std::string &response)
{
    // --- 1. Print the raw response for debugging (keeping this for now) ---
    std::cout << "\033[36mLSP FindRef Raw Response:\033[0m\n>>>>>>>>>>\n" << response << "\n<<<<<<<<<<\n" << std::endl;
    // --- End Printing ---

    referenceLocations.clear(); // Clear previous results

    try {
        // --- 2. Parse the JSON string ---
        json j = json::parse(response);

        // --- 3. Check for the "result" array ---
        if (j.contains("result") && j["result"].is_array()) {
            const auto &results = j["result"];

            std::cout << "\033[32mLSP FindRef Parse:\033[0m Found 'result' array with " << results.size() << " items." << std::endl;

            // --- 4. Iterate through each location object in the array ---
            for (const auto &loc_json : results) {
                // Ensure the item itself is an object
                if (!loc_json.is_object()) {
                    std::cout << "\033[33mLSP FindRef Parse:\033[0m Skipping non-object item in results array." << std::endl;
                    continue;
                }

                // Default values in case keys are missing
                std::string uri = "";
                int startLine = -1, startChar = -1;
                int endLine = -1, endChar = -1; // Also parse end if available

                // --- 5. Extract "uri" ---
                if (loc_json.contains("uri") && loc_json["uri"].is_string()) {
                    std::string fullUri = loc_json["uri"].get<std::string>();
                    // Strip "file://" prefix
                    if (fullUri.rfind("file://", 0) == 0) {
                        uri = fullUri.substr(7);
                    } else {
                        uri = fullUri; // Keep other URIs as is
                    }
                } else {
                    std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or invalid 'uri' in location object." << std::endl;
                    continue; // Skip this location if URI is missing
                }

                // --- 6. Extract "range" object ---
                if (loc_json.contains("range") && loc_json["range"].is_object()) {
                    const auto &range_json = loc_json["range"];

                    // --- 7. Extract "start" position ---
                    if (range_json.contains("start") && range_json["start"].is_object()) {
                        const auto &start_json = range_json["start"];
                        // Use .value() for safe extraction with default (-1 if missing/wrong type)
                        startLine = start_json.value("line", -1);
                        startChar = start_json.value("character", -1);
                    } else {
                        std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or invalid 'start' object within range for URI: " << uri << std::endl;
                    }

                    // --- 8. Extract "end" position (optional but good practice) ---
                    if (range_json.contains("end") && range_json["end"].is_object()) {
                        const auto &end_json = range_json["end"];
                        endLine = end_json.value("line", -1);
                        endChar = end_json.value("character", -1);
                    }
                    // No warning if end is missing, it's less critical than start

                } else {
                    std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or invalid 'range' object for URI: " << uri << std::endl;
                }

                // --- 9. Validate and Store ---
                // Check if we got the essential start information
                if (!uri.empty() && startLine != -1 && startChar != -1) {
                    std::cout << "\033[32mLSP FindRef Parse:\033[0m Successfully Parsed: " << uri << " [Line: " << startLine + 1 << " Char: " << startChar + 1 << "]" << std::endl;
                    referenceLocations.push_back({uri, startLine, startChar, endLine, endChar});
                } else {
                    std::cout << "\033[33mLSP FindRef Parse:\033[0m Failed to extract complete location info for object. URI:'" << uri << "', StartLine:" << startLine << ", StartChar:" << startChar << std::endl;
                }

            } // End loop through results array

        }
        // Handle cases where "result" exists but isn't an array (e.g., null)
        else if (j.contains("result") && j["result"].is_null()) {
            std::cout << "\033[32mLSP FindRef Parse:\033[0m 'result' is null. No references found." << std::endl;
            // referenceLocations is already cleared, showReferenceOptions remains false
        }
        // Handle missing "result" key (shouldn't happen for valid responses)
        else {
            std::cout << "\033[31mLSP FindRef Parse:\033[0m Response missing 'result' array or it's not an array." << std::endl;
        }

    } catch (json::parse_error &e) {
        // --- Handle JSON parsing errors ---
        std::cerr << "\033[31mLSP FindRef Parse:\033[0m JSON parsing error: " << e.what() << '\n' << "Exception id: " << e.id << std::endl;
        referenceLocations.clear(); // Ensure list is empty on error
        showReferenceOptions = false;
        return; // Stop processing on parse error
    } catch (json::exception &e) {
        // --- Handle other potential json library errors (e.g., type errors if not using .value()) ---
        std::cerr << "\033[31mLSP FindRef Parse:\033[0m JSON exception: " << e.what() << '\n' << "Exception id: " << e.id << std::endl;
        referenceLocations.clear();
        showReferenceOptions = false;
        return;
    }

    // --- 10. Update state based on parsed locations ---
    if (!referenceLocations.empty()) {
        std::cout << "\033[32mLSP FindRef Parse:\033[0m Finished Parsing. Found " << referenceLocations.size() << " reference location(s)." << std::endl;
        showReferenceOptions = true;
        selectedReferenceIndex = 0; // Reset selection
    } else {
        // This now means the array was empty, null, or parsing failed for all items found.
        std::cout << "\033[33mLSP FindRef Parse:\033[0m Finished Parsing. No valid reference locations were successfully extracted or none were found." << std::endl;
        showReferenceOptions = false; // Ensure this is false if no locations were added
    }
}
// --- hasReferenceOptions (Identical to hasDefinitionOptions) ---
bool LSPGotoRef::hasReferenceOptions() const
{
    // Keep the logic consistent, show window only if flag is true AND locations exist
    return showReferenceOptions && !referenceLocations.empty();
}

void LSPGotoRef::renderReferenceOptions()
{
    if (!showReferenceOptions || referenceLocations.empty()) {
        editor_state.block_input = false;
        return;
    }

    editor_state.block_input = true;

    // Height calculations
    float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    float padding = 16.0f;
    float separatorHeight = ImGui::GetTextLineHeight() * 0.4f;
    float titleHeight = itemHeight + separatorHeight + 4.0f;
    float footerHeight = itemHeight + padding;
    float contentHeight = itemHeight * referenceLocations.size();
    float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;
    const float maxHeight = ImGui::GetIO().DisplaySize.y * 0.5f;

    // Window size calculations
    float desiredWidth = 600.0f;
    desiredWidth = std::max(desiredWidth, 500.0f);
    ImVec2 windowSize(desiredWidth, std::min(totalHeight, maxHeight) + (referenceLocations.size() == 1 ? 10.0f : 25.0f));
    windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f);

    ImVec2 windowPos(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

    // Style setup
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

    if (ImGui::Begin("##ReferenceOptions", nullptr, windowFlags)) {
        // Handle click outside window
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 currentWindowPos = ImGui::GetWindowPos();
            ImVec2 currentWindowSize = ImGui::GetWindowSize();
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && (mousePos.x < currentWindowPos.x || mousePos.x > currentWindowPos.x + currentWindowSize.x || mousePos.y < currentWindowPos.y || mousePos.y > currentWindowPos.y + currentWindowSize.y)) {
                showReferenceOptions = false;
                editor_state.block_input = false;
            }
        }

        // Handle escape key
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showReferenceOptions = false;
            editor_state.block_input = false;
        }

        // Fixed header
        ImGui::BeginChild("##Header", ImVec2(0, titleHeight), false);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Find References (%zu)", referenceLocations.size());
        ImGui::Separator();
        ImGui::EndChild();

        // Scrollable content area
        float contentAvailableHeight = windowSize.y - titleHeight - footerHeight - padding * 2;
        ImGui::BeginChild("##ContentScroll", ImVec2(0, contentAvailableHeight), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        // Keyboard navigation
        if (!ImGui::IsAnyItemActive()) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                selectedReferenceIndex = (selectedReferenceIndex > 0) ? selectedReferenceIndex - 1 : referenceLocations.size() - 1;
                ImGui::SetScrollHereY(0.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                selectedReferenceIndex = (selectedReferenceIndex + 1) % referenceLocations.size();
                ImGui::SetScrollHereY(1.0f);
            }
        }

        // List items
        for (size_t i = 0; i < referenceLocations.size(); i++) {
            const auto &loc = referenceLocations[i];
            bool is_selected = (selectedReferenceIndex == i);

            // Format filename
            std::string filename = loc.uri;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            std::string label = filename + ":" + std::to_string(loc.startLine + 1) + ":" + std::to_string(loc.startChar + 1);

            if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
                selectedReferenceIndex = i;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    handleReferenceSelection();
                }
            }

            if (is_selected && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
                ImGui::SetScrollHereY();
            }
            if (is_selected && ImGui::IsWindowAppearing()) {
                ImGui::SetScrollHereY();
            }
        }

        ImGui::EndChild(); // End content scroll area

        // Fixed footer
        ImGui::BeginChild("##Footer", ImVec2(0, footerHeight), false);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Up/Down Enter");
        ImGui::EndChild();

        // Handle Enter key
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            handleReferenceSelection();
        }

        ImGui::End(); // End main window
    } else {
        if (showReferenceOptions) {
            showReferenceOptions = false;
            editor_state.block_input = false;
        }
    }

    // Cleanup
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(4);

    if (!showReferenceOptions) {
        editor_state.block_input = false;
    }
}

void LSPGotoRef::handleReferenceSelection()
{
    if (selectedReferenceIndex >= referenceLocations.size())
        return;

    const auto &selected = referenceLocations[selectedReferenceIndex];
    std::cout << "Selected reference at " << selected.uri << " line " << (selected.startLine + 1) << std::endl;

    // Close window first
    showReferenceOptions = false;
    editor_state.block_input = false;

    if (selected.uri != gFileExplorer.currentFile) {
        gFileExplorer.loadFileContent(selected.uri, nullptr);
    }

    int index = 0;
    int currentLine = 0;
    const std::string &content = editor_state.fileContent;

    while (currentLine < selected.startLine && index < content.length()) {
        if (content[index] == '\n')
            currentLine++;
        index++;
    }

    index += selected.startChar;
    index = std::min(index, static_cast<int>(content.length()));

    editor_state.cursor_index = index;
    gEditorScroll.setEnsureCursorVisibleFrames(-1);
    // editor_state.ensure_cursor_visible.horizontal = true;
    // editor_state.ensure_cursor_visible.vertical = true;
    editor_state.center_cursor_vertical = true;
}