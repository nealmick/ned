#include "lsp_goto_def.h"
#include "../editor/editor.h"           // Access to gEditor, editor_state
#include "../editor/editor_line_jump.h" // Access to gEditorScroll
#include "files.h"                      // Access to gFileExplorer
#include <algorithm>                    // For std::min, std::max
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

// Use the nlohmann::json namespace (already declared in header, but fine here too)
using json = nlohmann::json;

// Global instance definition (remains the same)
LSPGotoDef gLSPGotoDef;

// Constructor and Destructor (remain the same)
LSPGotoDef::LSPGotoDef() : currentRequestId(2000), showDefinitionOptions(false), selectedDefinitionIndex(0) {}
LSPGotoDef::~LSPGotoDef() = default;

// --- gotoDefinition (Updated Response Handling) ---
    bool LSPGotoDef::gotoDefinition(const std::string &filePath, int line, int character)
    {
        if (!gLSPManager.isInitialized()) {
            std::cout << "\033[31mLSP GotoDef:\033[0m Not initialized" << std::endl;
            return false;
        }

        if (!gLSPManager.selectAdapterForFile(filePath)) {
            std::cout << "\033[31mLSP GotoDef:\033[0m No LSP adapter available for file: " << filePath << std::endl;
            return false;
        }

        int requestId = getNextRequestId();
        std::cout << "\033[35mLSP GotoDef:\033[0m Requesting definition at line " << line << ", char " << character << " (ID: " << requestId << ")" << std::endl;

        // Request structure remains the same
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

    if (!gLSPManager.sendRequest(request)) {
        std::cout << "\033[31mLSP GotoDef:\033[0m Failed to send request" << std::endl;
        return false;
    }

    // --- MODIFIED Response Handling Loop ---
    const int MAX_ATTEMPTS = 15;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        std::cout << "\033[35mLSP GotoDef:\033[0m Waiting for definition response (attempt " << (attempt + 1) << ")" << std::endl;

        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength);

        if (response.empty()) {
            std::cout << "\033[31mLSP GotoDef:\033[0m Empty response received" << std::endl;
            if (attempt == MAX_ATTEMPTS - 1) {
                std::cout << "\033[31mLSP GotoDef:\033[0m Timeout waiting for response." << std::endl;
                return false;
            }
            // Consider adding sleep: std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Check if the response ID matches our request
        if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos) {
            std::cout << "\033[32mLSP GotoDef:\033[0m Received response for request ID " << requestId << std::endl;

            // Use the NEW robust JSON parser function
            parseDefinitionResponse(response); // Pass the raw response

            // Check if parsing actually found locations
            // (parseDefinitionResponse now sets showDefinitionOptions internally)
            if (showDefinitionOptions) { // Check the flag set by the parser
                return true;             // Success, options window will show
            }
            // Check if the response indicates a server-side error *after* attempting to parse
            else if (response.find("\"error\":") != std::string::npos) {
                std::cout << "\033[31mLSP GotoDef:\033[0m Error reported in server response." << std::endl;
                return false; // Error from server
            }
            // If parsing didn't yield options and there wasn't an error object,
            // it means result was null, empty array, or parsing failed internally.
            else {
                std::cout << "\033[33mLSP GotoDef:\033[0m No definition locations found or parsed from the response." << std::endl;
                // Return true because we got *a* valid response for our ID, even if it contained no results.
                // The UI simply won't show the options window because hasDefinitionOptions() will be false.
                return true;
            }
        }

        // If it wasn't our response ID, log and continue waiting
        std::cout << "\033[33mLSP GotoDef:\033[0m Received unrelated response. Continuing..." << std::endl;
    } // End for loop

    std::cout << "\033[31mLSP GotoDef:\033[0m Exceeded maximum attempts waiting for response ID " << requestId << std::endl;
    return false;
}

// --- parseDefinitionResponse (REWRITTEN using nlohmann::json) ---
void LSPGotoDef::parseDefinitionResponse(const std::string &response)
{
    // Optional: Print raw response for debugging
    // std::cout << "\033[36mLSP GotoDef Raw Response:\033[0m\n>>>>>>>>>>\n" << response << "\n<<<<<<<<<<\n" << std::endl;

    definitionLocations.clear();   // Clear previous results
    showDefinitionOptions = false; // Assume no options initially

    try {
        json j = json::parse(response);

        // Check if 'result' key exists
        if (!j.contains("result")) {
            // Check if 'error' key exists instead
            if (j.contains("error")) {
                std::cout << "\033[31mLSP GotoDef Parse:\033[0m Response contains an error object." << std::endl;
                // Optionally log error details: std::cerr << j["error"].dump(2) << std::endl;
            } else {
                std::cout << "\033[31mLSP GotoDef Parse:\033[0m Response missing 'result' key." << std::endl;
            }
            return; // Nothing to parse
        }

        const auto &result = j["result"];

        // Case 1: Result is null
        if (result.is_null()) {
            std::cout << "\033[32mLSP GotoDef Parse:\033[0m 'result' is null. No definition found." << std::endl;
            // definitionLocations is already cleared, showDefinitionOptions remains false
        }
        // Case 2: Result is a single Location object (check for uri/range keys)
        else if (result.is_object() && result.contains("uri") && result.contains("range")) {
            std::cout << "\033[32mLSP GotoDef Parse:\033[0m Found single 'result' object (Location)." << std::endl;
            // Create a temporary array containing just this single object
            json results_array = json::array({result});
            parseDefinitionArray(results_array); // Call helper to parse
        }
        // Case 3: Result is an array (either Location[] or LocationLink[])
        else if (result.is_array()) {
            std::cout << "\033[32mLSP GotoDef Parse:\033[0m Found 'result' array with " << result.size() << " items." << std::endl;
            if (result.empty()) {
                std::cout << "\033[33mLSP GotoDef Parse:\033[0m Result array is empty." << std::endl;
            } else {
                parseDefinitionArray(result); // Call helper to parse the array
            }
        }
        // Case 4: Unexpected result type
        else {
            std::cout << "\033[31mLSP GotoDef Parse:\033[0m 'result' key contains unexpected data type: " << result.type_name() << std::endl;
        }

    } catch (json::parse_error &e) {
        std::cerr << "\033[31mLSP GotoDef Parse:\033[0m JSON parsing error: " << e.what() << '\n' << "Exception id: " << e.id << std::endl;
        definitionLocations.clear(); // Ensure list is empty on error
        showDefinitionOptions = false;
        return; // Stop processing on parse error
    } catch (json::exception &e) {
        std::cerr << "\033[31mLSP GotoDef Parse:\033[0m JSON exception: " << e.what() << '\n' << "Exception id: " << e.id << std::endl;
        definitionLocations.clear();
        showDefinitionOptions = false;
        return;
    }

    // Update state based on whether locations were found AFTER parsing
    if (!definitionLocations.empty()) {
        std::cout << "\033[32mLSP GotoDef Parse:\033[0m Finished Parsing. Found " << definitionLocations.size() << " definition location(s)." << std::endl;
        showDefinitionOptions = true; // Set flag to true ONLY if locations were added
        selectedDefinitionIndex = 0;  // Reset selection
    } else {
        // This means result was null, empty array, or parsing failed for all items.
        std::cout << "\033[33mLSP GotoDef Parse:\033[0m Finished Parsing. No valid definition locations were successfully extracted or none were found." << std::endl;
        // showDefinitionOptions remains false
    }
}

// --- Helper function to parse array items (Location or LocationLink) ---
// --- ADDED IMPLEMENTATION ---
void LSPGotoDef::parseDefinitionArray(const json &results_array)
{
    for (const auto &item : results_array) {
        if (!item.is_object()) {
            std::cout << "\033[33mLSP GotoDef Parse:\033[0m Skipping non-object item in results array." << std::endl;
            continue;
        }

        std::string uri = "";
        int startLine = -1, startChar = -1;
        int endLine = -1, endChar = -1;
        bool parsed_successfully = false;

        // Try parsing as LocationLink first (more specific keys)
        if (item.contains("targetUri") && item.contains("targetRange")) {
            if (item["targetUri"].is_string() && item["targetRange"].is_object()) {
                std::string fullUri = item["targetUri"].get<std::string>();
                if (fullUri.rfind("file://", 0) == 0)
                    uri = fullUri.substr(7);
                else
                    uri = fullUri;

                const auto &range_json = item["targetRange"];
                // Check the *inner* structure of targetRange as well
                if (range_json.is_object() && range_json.contains("start") && range_json["start"].is_object()) {
                    startLine = range_json["start"].value("line", -1);
                    startChar = range_json["start"].value("character", -1);

                    if (range_json.contains("end") && range_json["end"].is_object()) {
                        endLine = range_json["end"].value("line", -1);
                        endChar = range_json["end"].value("character", -1);
                    }
                    // Consider parse successful if start is valid
                    parsed_successfully = (!uri.empty() && startLine != -1 && startChar != -1);
                    if (parsed_successfully)
                        std::cout << "\033[35mLSP GotoDef Parse:\033[0m Parsed as LocationLink." << std::endl;
                }
            }
        }
        // Else, try parsing as regular Location
        else if (item.contains("uri") && item.contains("range")) {
            if (item["uri"].is_string() && item["range"].is_object()) {
                std::string fullUri = item["uri"].get<std::string>();
                if (fullUri.rfind("file://", 0) == 0)
                    uri = fullUri.substr(7);
                else
                    uri = fullUri;

                const auto &range_json = item["range"];
                // Check the *inner* structure of range as well
                if (range_json.is_object() && range_json.contains("start") && range_json["start"].is_object()) {
                    startLine = range_json["start"].value("line", -1);
                    startChar = range_json["start"].value("character", -1);

                    if (range_json.contains("end") && range_json["end"].is_object()) {
                        endLine = range_json["end"].value("line", -1);
                        endChar = range_json["end"].value("character", -1);
                    }
                    // Consider parse successful if start is valid
                    parsed_successfully = (!uri.empty() && startLine != -1 && startChar != -1);
                    if (parsed_successfully)
                        std::cout << "\033[35mLSP GotoDef Parse:\033[0m Parsed as Location." << std::endl;
                }
            }
        }

        // Add if parsing was successful for either type
        if (parsed_successfully) {
            std::cout << "\033[32mLSP GotoDef Parse:\033[0m Adding Location: " << uri << " [Line: " << startLine + 1 << " Char: " << startChar + 1 << "]" << std::endl;
            definitionLocations.push_back({uri, startLine, startChar, endLine, endChar});
        } else {
            // Only log if it was an object but didn't match expected structures
            if (item.is_object()) {
                std::cout << "\033[33mLSP GotoDef Parse:\033[0m Skipping item - failed to parse as Location or LocationLink. Content: " << item.dump() << std::endl;
            }
        }
    } // End for loop
}

// --- hasDefinitionOptions (remains the same) ---
bool LSPGotoDef::hasDefinitionOptions() const
{
    // This check remains correct. The window should only show if the flag is true
    // AND the vector actually contains items.
    return showDefinitionOptions && !definitionLocations.empty();
}

// --- renderDefinitionOptions (NO CHANGES NEEDED - Uses definitionLocations) ---
void LSPGotoDef::renderDefinitionOptions()
{
    // Guard clause remains the same - uses hasDefinitionOptions() effectively
    if (!hasDefinitionOptions()) { // Combined check using the method
        editor_state.block_input = false;
        return;
    }

    editor_state.block_input = true;

    // --- All calculation logic below remains the same ---
    // It correctly uses definitionLocations.size()

    // Calculate required height based on content
    float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    float padding = 16.0f;
    float titleHeight = itemHeight + 4.0f;
    float footerHeight = itemHeight + padding;
    float contentHeight = itemHeight * definitionLocations.size(); // Uses the vector size
    float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;

    // --- Limit height for long lists (like reference window) ---
    totalHeight = std::min(totalHeight, ImGui::GetIO().DisplaySize.y * 0.6f);

    // Calculate window position and size - Use same width logic as reference for consistency?
    float desiredWidth = 600.0f; // Use same width as references? Or keep 500? Let's use 600 for consistency.
    // desiredWidth = std::max(desiredWidth, 500.0f); // Ensure minimum width if needed
    ImVec2 windowSize(desiredWidth, totalHeight);
    windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f); // Limit width

    ImVec2 windowPos(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f); // Same pos logic

    // Set window position and size
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // --- Window flags - Add scrollbar if content overflows ---
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
    float availableContentHeight = totalHeight - titleHeight - footerHeight - padding * 2;
    if (contentHeight > availableContentHeight) {
        windowFlags &= ~ImGuiWindowFlags_NoScrollbar; // Enable scrollbar if needed
    }

    // --- Styling remains the same ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f)); // Unused?
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

    // --- Begin Window ---
    if (ImGui::Begin("##DefinitionOptions", nullptr, windowFlags)) { // Use nullptr for p_open

        // --- Click outside / Escape handling remains the same ---
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 currentWindowPos = ImGui::GetWindowPos();
            ImVec2 currentWindowSize = ImGui::GetWindowSize();
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && (mousePos.x < currentWindowPos.x || mousePos.x > currentWindowPos.x + currentWindowSize.x || mousePos.y < currentWindowPos.y || mousePos.y > currentWindowPos.y + currentWindowSize.y)) {
                showDefinitionOptions = false;
                editor_state.block_input = false;
                // End() is not needed here, Begin returns false next frame
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showDefinitionOptions = false;
            editor_state.block_input = false;
        }

        // --- Title and separator remain the same ---
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Go to Definition (%zu)", definitionLocations.size());
        ImGui::Separator();

        // --- Content Area (with conditional child window for scrolling) ---
        bool useChildWindow = !(windowFlags & ImGuiWindowFlags_NoScrollbar);
        if (useChildWindow) {
            ImGui::BeginChild("##DefListScroll", ImVec2(0, availableContentHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
        }

        // --- Keyboard navigation remains the same ---
        if (!ImGui::IsAnyItemActive()) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                selectedDefinitionIndex = (selectedDefinitionIndex > 0) ? selectedDefinitionIndex - 1 : definitionLocations.size() - 1;
                if (useChildWindow)
                    ImGui::SetScrollHereY(0.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                selectedDefinitionIndex = (selectedDefinitionIndex + 1) % definitionLocations.size();
                if (useChildWindow)
                    ImGui::SetScrollHereY(1.0f);
            }
        }

        // --- Display options loop remains the same ---
        // float windowWidth = ImGui::GetContentRegionAvail().x; // Not needed if using SpanAllColumns
        // float itemPadding = 8.0f; // Not needed if not using SetCursorPosX

        for (size_t i = 0; i < definitionLocations.size(); i++) {
            const auto &loc = definitionLocations[i];
            bool is_selected = (selectedDefinitionIndex == i);

            // ImGui::SetCursorPosX(ImGui::GetCursorPosX() + itemPadding); // Removed for simplicity, use SpanAllColumns

            // Format label (Filename:Line:Char is good here too)
            std::string filename = loc.uri;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            std::string label = filename + ":" + std::to_string(loc.startLine + 1) + ":" + std::to_string(loc.startChar + 1);

            // Use AllowDoubleClick and SpanAllColumns
            if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
                selectedDefinitionIndex = i;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    goto handle_enter_key_def; // Use unique label
                }
            }

            // Ensure selected item is visible on navigation
            if (is_selected && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
                ImGui::SetScrollHereY();
            }
            if (is_selected && ImGui::IsWindowAppearing()) {
                ImGui::SetScrollHereY();
            }
        }

        if (useChildWindow) {
            ImGui::EndChild();
        }
        // --- End Content Area ---

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Up/Down Enter");

        // --- Handle Enter key remains the same (uses definitionLocations) ---
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        handle_enter_key_def:                                                                           // Unique label
            if (selectedDefinitionIndex >= 0 && selectedDefinitionIndex < definitionLocations.size()) { // Check bounds
                const auto &selected = definitionLocations[selectedDefinitionIndex];
                std::cout << "Selected definition at " << selected.uri << " line " << (selected.startLine + 1) << " char " << (selected.startChar + 1) << std::endl;

                if (selected.uri != gFileExplorer.currentFile) {
                    gFileExplorer.loadFileContent(selected.uri, nullptr);
                }
                // loop through each char in file until we reach a current line = to selection jump line... currentLine
                // index tracks every character to so we know which character index the line starts on
                // then we can just add the char response to the line index start... and we have the index of the jump
                int index = 0;
                int currentLine = 0;
                std::cout << "Calculating cursor position..." << std::endl;

                while (currentLine < selected.startLine && index < editor_state.fileContent.length()) {
                    if (editor_state.fileContent[index] == '\n') {
                        currentLine++;
                    }
                    index++;
                }

                index += selected.startChar;
                index = std::min(index, (int)editor_state.fileContent.length());
                editor_state.cursor_index = index;
                gEditorScroll.setEnsureCursorVisibleFrames(-1);
                showDefinitionOptions = false;
                editor_state.block_input = false;
                editor_state.ensure_cursor_visible.horizontal = true;
                editor_state.ensure_cursor_visible.vertical = true;

                showDefinitionOptions = false;
                editor_state.block_input = false;
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { // Already handled, but keep for clarity
            showDefinitionOptions = false;
            editor_state.block_input = false;
        }

        ImGui::End(); // End the window

    } else {
        // If Begin returned false
        if (showDefinitionOptions) {
            showDefinitionOptions = false;
            editor_state.block_input = false;
        }
    }

    // Pop styles
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(4);

    // Final check
    if (!showDefinitionOptions) {
        editor_state.block_input = false;
    }
}