#include "lsp_autocomplete.h"
#include "lib/json.hpp"
#include "lsp.h" // Make sure lsp.h is included to use gEditorLSP
#include "lsp_manager.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using json = nlohmann::json;

LSPAutocomplete gLSPAutocomplete;

LSPAutocomplete::LSPAutocomplete() = default;
LSPAutocomplete::~LSPAutocomplete() = default;

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
                          std::to_string(line - 1) + R"(,
                "character": )" +
                          std::to_string(character - 1) + R"(
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

    // Wait for response with matching ID
    const int MAX_ATTEMPTS = 15; // Increased attempts slightly
    const int WAIT_MS = 100;     // Increased wait time slightly

    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        // Optional: Add a small delay before reading to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
        // std::cout << "\033[35mLSP Autocomplete:\033[0m Waiting for response (attempt " << (attempt + 1) << ")" << std::endl;

        int contentLength = 0;
        std::string response = gLSPManager.readResponse(&contentLength); // Assuming readResponse is non-blocking or has a timeout

        if (response.empty()) {
            if (attempt == 0) { // Only print wait message on first empty response
                std::cout << "\033[35mLSP Autocomplete:\033[0m Waiting for response..." << std::endl;
            }
            continue; // Try reading again
        }

        try {
            json j = json::parse(response);

            // Check if this response matches our request ID
            if (!j.contains("id") || !j["id"].is_number_integer() || j["id"].get<int>() != requestId) {
                // std::cout << "\033[33mLSP Autocomplete:\033[0m Ignoring response with different ID (" << (j.contains("id") ? j["id"].dump() : "no id") << ")" << std::endl;
                continue; // Skip unrelated responses or notifications
            }

            std::cout << "\033[32mLSP Autocomplete:\033[0m Received response for ID " << requestId << std::endl;

            // Handle errors
            if (j.contains("error")) {
                std::cerr << "\033[31mLSP Autocomplete:\033[0m Error: " << j["error"].dump(2) << std::endl; // Dump for more detail
                return;
            }

            // Parse completion items
            if (j.contains("result")) {
                json result = j["result"];

                if (result.is_null()) {
                    std::cout << "\033[33mLSP Autocomplete:\033[0m No completions found." << std::endl;
                    return;
                }

                std::vector<json> items;
                bool isIncomplete = false; // Check if the list might be incomplete

                if (result.is_array()) {
                    items = result.get<std::vector<json>>();
                } else if (result.is_object()) {
                    if (result.contains("items") && result["items"].is_array()) {
                        items = result["items"].get<std::vector<json>>();
                    }
                    if (result.contains("isIncomplete") && result["isIncomplete"].is_boolean()) {
                        isIncomplete = result["isIncomplete"].get<bool>();
                    }
                } else {
                    std::cout << "\033[31mLSP Autocomplete:\033[0m Unexpected result format: " << result.type_name() << std::endl;
                    return;
                }
                std::cout << "\033[32mFound " << items.size() << " completions" << (isIncomplete ? " (incomplete list)" : "") << ":\033[0m" << std::endl;
                if (items.empty()) {
                    std::cout << "  (No items in list)" << std::endl;
                } else {
                    for (const auto &item : items) {
                        // Extract common fields safely
                        std::string label = item.value("label", "[no label]");
                        std::string detail = item.value("detail", "");
                        int kind_val = item.value("kind", 0);
                        std::string kind_str = std::to_string(kind_val);

                        // Determine the text to insert based on LSP priority
                        std::string text_to_insert = "";

                        // 1. Check textEdit.newText
                        if (item.contains("textEdit") && item["textEdit"].is_object() && item["textEdit"].contains("newText") && item["textEdit"]["newText"].is_string()) {
                            text_to_insert = item["textEdit"]["newText"].get<std::string>();
                        }
                        // 2. Check insertText (only if textEdit wasn't used)
                        else if (item.contains("insertText") && item["insertText"].is_string()) {
                            text_to_insert = item["insertText"].get<std::string>();
                        }
                        // 3. Fallback to label
                        else {
                            text_to_insert = label; // Use the label if others are missing
                        }

                        // Print the determined insertion text and other details
                        std::cout << " - Insert: \"" << text_to_insert << "\""
                                  << " (Label: " << label;
                        if (!detail.empty()) {
                            std::cout << ", Detail: " << detail;
                        }
                        std::cout << ", Kind: " << kind_str << ")" << std::endl;
                    }
                }

                return; // Success
            } else {
                std::cout << "\033[31mLSP Autocomplete:\033[0m Response missing 'result' field." << std::endl;
                // Optionally print the full response for debugging:
                // std::cout << response << std::endl;
                return;
            }

        } catch (const json::exception &e) {
            std::cerr << "\033[31mLSP Autocomplete:\033[0m JSON error: " << e.what() << std::endl;
            std::cerr << "Received content: " << response << std::endl; // Print problematic response
            return;                                                     // Stop processing on error
        }
    }

    std::cout << "\033[31mLSP Autocomplete:\033[0m Timed out waiting for response ID " << requestId << "." << std::endl;
}