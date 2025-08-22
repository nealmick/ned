#include "lsp_goto_ref.h"
#include "../editor/editor.h"
#include "../editor/editor_line_jump.h"
#include "files.h"
#include <algorithm> // For std::min
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <set>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

// Platform-specific LSP manager includes
#ifdef PLATFORM_WINDOWS
#include "lsp_manager_windows.h"
#else
#include "lsp_manager.h"
#endif

// Platform-specific LSP manager selection
#ifdef PLATFORM_WINDOWS
#define CURRENT_LSP_MANAGER gLSPManagerWindows
#else
#define CURRENT_LSP_MANAGER gLSPManager
#endif

LSPGotoRef gLSPGotoRef;

LSPGotoRef::LSPGotoRef()
	: currentRequestId(3000), showReferenceOptions(false), selectedReferenceIndex(0)
{
}

LSPGotoRef::~LSPGotoRef() = default;

bool LSPGotoRef::findReferences(const std::string &filePath, int line, int character)
{
#ifdef PLATFORM_WINDOWS
	// Windows-specific initialization and file type checking
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos) {
		return false; // No extension
	}
	std::string ext = filePath.substr(dot_pos + 1);
	if (ext != "py") {
		return false; // Only support Python files for now on Windows
	}

	// Select adapter and auto-initialize if needed
	if (!CURRENT_LSP_MANAGER.selectAdapterForFile(filePath)) {
		std::cout << "\033[33mLSP FindRef:\033[0m No adapter available for file: " << filePath << std::endl;
		return false;
	}

	if (!CURRENT_LSP_MANAGER.isInitialized()) {
		// Extract workspace path from file path
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		if (!CURRENT_LSP_MANAGER.initialize(workspacePath)) {
			std::cout << "\033[31mLSP FindRef:\033[0m Failed to initialize LSP" << std::endl;
			return false;
		}
	}
#else
	// Linux/macOS LSP initialization
	if (!CURRENT_LSP_MANAGER.isInitialized())
	{
		std::cout << "\033[31mLSP FindRef:\033[0m Not initialized" << std::endl;
		return false;
	}

	if (!CURRENT_LSP_MANAGER.selectAdapterForFile(filePath))
	{
		std::cout << "\033[31mLSP FindRef:\033[0m No LSP adapter available for file: "
				  << filePath << std::endl;
		return false;
	}
#endif

	int requestId = getNextRequestId();
	std::cout << "\033[35mLSP FindRef:\033[0m Requesting references at line " << line
			  << ", char " << character << " (ID: " << requestId << ")" << std::endl;

#ifdef PLATFORM_WINDOWS
	// Convert Windows path to proper URI format and handle didOpen
	std::string fileURI = "file:///" + filePath;
	for (char &c : fileURI) {
		if (c == '\\') c = '/';
	}

	// Send didOpen notification if needed
	static std::set<std::string> openedFiles;
	if (openedFiles.find(fileURI) == openedFiles.end()) {
		std::ifstream fileStream(filePath);
		if (fileStream.is_open()) {
			std::string fileContent((std::istreambuf_iterator<char>(fileStream)),
									std::istreambuf_iterator<char>());
			fileStream.close();

			// Escape content for JSON
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

			std::cout << "\033[35mLSP FindRef:\033[0m Sending didOpen notification for file" << std::endl;
			CURRENT_LSP_MANAGER.sendRequest(didOpenRequest);
			openedFiles.insert(fileURI);
			Sleep(100);
		}
	}

	std::string uriForRequest = fileURI;
#else
	std::string uriForRequest = "file://" + filePath;
#endif

	std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(requestId) +
									  R"(,
        "method": "textDocument/references",
        "params": {
            "textDocument": {
                "uri": ")" + uriForRequest +
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

	if (!CURRENT_LSP_MANAGER.sendRequest(request))
	{
		std::cout << "\033[31mLSP FindRef:\033[0m Failed to send request" << std::endl;
		return false;
	}

	const int MAX_ATTEMPTS = 15;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
	{
		std::cout << "\033[35mLSP FindRef:\033[0m Waiting for references "
					 "response (attempt "
				  << (attempt + 1) << ")" << std::endl;

		int contentLength = 0;
		std::string response = CURRENT_LSP_MANAGER.readResponse(&contentLength);

		if (response.empty())
		{
			std::cout << "\033[31mLSP FindRef:\033[0m Empty response received"
					  << std::endl;
			if (attempt == MAX_ATTEMPTS - 1)
			{
				std::cout << "\033[31mLSP FindRef:\033[0m Timeout waiting for "
							 "response."
						  << std::endl;
				return false;
			}
			continue;
		}

		if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos)
		{
			std::cout << "\033[32mLSP FindRef:\033[0m Received response for "
						 "request ID "
					  << requestId << std::endl;

			if (response.find("\"result\":[") != std::string::npos)
			{
				std::cout << "\033[32mLSP FindRef:\033[0m Found result array."
						  << std::endl;
				parseReferenceResponse(response); // Parse the array
				return true;
			} else if (response.find("\"result\":null") != std::string::npos)
			{
				std::cout << "\033[33mLSP FindRef:\033[0m No references found "
							 "(result is null)."
						  << std::endl;
				referenceLocations.clear(); // Ensure list is empty
				showReferenceOptions = false;
				return true; // Successfully processed the response (no results)
			} else if (response.find("\"error\":") != std::string::npos)
			{
				std::cout << "\033[31mLSP FindRef:\033[0m Error in response: " << response
						  << std::endl;
				referenceLocations.clear();
				showReferenceOptions = false;
				return false; // Indicate error
			} else
			{
				std::cout << "\033[31mLSP FindRef:\033[0m Response for ID " << requestId
						  << " has unexpected format: " << response << std::endl;
				referenceLocations.clear();
				showReferenceOptions = false;
				return false; // Indicate unexpected format
			}
		}

		std::cout << "\033[33mLSP FindRef:\033[0m Received unrelated response. "
					 "Continuing..."
				  << std::endl;
	}

	std::cout << "\033[31mLSP FindRef:\033[0m Exceeded maximum attempts "
				 "waiting for response ID "
			  << requestId << std::endl;
	return false;
}
void LSPGotoRef::parseReferenceResponse(const std::string &response)
{
	std::cout << "\033[36mLSP FindRef Raw Response:\033[0m\n>>>>>>>>>>\n"
			  << response << "\n<<<<<<<<<<\n"
			  << std::endl;

	referenceLocations.clear();
	try
	{
		json j = json::parse(response);

		// --- 3. Check for the "result" array ---
		if (j.contains("result") && j["result"].is_array())
		{
			const auto &results = j["result"];

			std::cout << "\033[32mLSP FindRef Parse:\033[0m Found 'result' "
						 "array with "
					  << results.size() << " items." << std::endl;

			for (const auto &loc_json : results)
			{
				// Ensure the item itself is an object
				if (!loc_json.is_object())
				{
					std::cout << "\033[33mLSP FindRef Parse:\033[0m Skipping "
								 "non-object item in results array."
							  << std::endl;
					continue;
				}

				std::string uri = "";
				int startLine = -1, startChar = -1;
				int endLine = -1, endChar = -1;
				// --- 5. Extract "uri" ---
				if (loc_json.contains("uri") && loc_json["uri"].is_string())
				{
					std::string fullUri = loc_json["uri"].get<std::string>();
#ifdef PLATFORM_WINDOWS
					// Windows uses file:/// format
					if (fullUri.rfind("file:///", 0) == 0) {
						uri = fullUri.substr(8); // Remove file:///
						
						// Decode URL-encoded characters (e.g., %3A -> :)
						std::string decodedPath;
						for (size_t i = 0; i < uri.length(); ++i) {
							if (uri[i] == '%' && i + 2 < uri.length()) {
								// Convert hex to character
								std::string hexStr = uri.substr(i + 1, 2);
								try {
									char decodedChar = static_cast<char>(std::stoi(hexStr, nullptr, 16));
									decodedPath += decodedChar;
									i += 2; // Skip the hex digits
								} catch (const std::exception& e) {
									// If hex parsing fails, just keep the original characters
									decodedPath += uri[i];
								}
							} else {
								decodedPath += uri[i];
							}
						}
						uri = decodedPath;
						
						// Convert forward slashes back to backslashes for Windows
						for (char &c : uri) {
							if (c == '/') c = '\\';
						}
					} else {
						uri = fullUri;
					}
#else
					// Linux/macOS uses file:// format
					if (fullUri.rfind("file://", 0) == 0)
					{
						uri = fullUri.substr(7);
					} else
					{
						uri = fullUri;
					}
#endif
				} else
				{
					std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or "
								 "invalid 'uri' in location object."
							  << std::endl;
					continue;
				}

				if (loc_json.contains("range") && loc_json["range"].is_object())
				{
					const auto &range_json = loc_json["range"];

					if (range_json.contains("start") && range_json["start"].is_object())
					{
						const auto &start_json = range_json["start"];
						startLine = start_json.value("line", -1);
						startChar = start_json.value("character", -1);
					} else
					{
						std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or "
									 "invalid 'start' object within range for URI: "
								  << uri << std::endl;
					}

					if (range_json.contains("end") && range_json["end"].is_object())
					{
						const auto &end_json = range_json["end"];
						endLine = end_json.value("line", -1);
						endChar = end_json.value("character", -1);
					}
				} else
				{
					std::cout << "\033[33mLSP FindRef Parse:\033[0m Missing or "
								 "invalid 'range' object for URI: "
							  << uri << std::endl;
				}

				if (!uri.empty() && startLine != -1 && startChar != -1)
				{
					std::cout << "\033[32mLSP FindRef Parse:\033[0m "
								 "Successfully Parsed: "
							  << uri << " [Line: " << startLine + 1
							  << " Char: " << startChar + 1 << "]" << std::endl;
					referenceLocations.push_back(
						{uri, startLine, startChar, endLine, endChar});
				} else
				{
					std::cout << "\033[33mLSP FindRef Parse:\033[0m Failed to "
								 "extract complete location info for object. URI:'"
							  << uri << "', StartLine:" << startLine
							  << ", StartChar:" << startChar << std::endl;
				}
			}
		} else if (j.contains("result") && j["result"].is_null())
		{
			std::cout << "\033[32mLSP FindRef Parse:\033[0m 'result' is null. "
						 "No references found."
					  << std::endl;
		} else
		{
			std::cout << "\033[31mLSP FindRef Parse:\033[0m Response missing "
						 "'result' array or it's not an array."
					  << std::endl;
		}
	} catch (json::parse_error &e)
	{
		std::cerr << "\033[31mLSP FindRef Parse:\033[0m JSON parsing error: " << e.what()
				  << '\n'
				  << "Exception id: " << e.id << std::endl;
		referenceLocations.clear(); // Ensure list is empty on error
		showReferenceOptions = false;
		return; // Stop processing on parse error
	} catch (json::exception &e)
	{
		std::cerr << "\033[31mLSP FindRef Parse:\033[0m JSON exception: " << e.what()
				  << '\n'
				  << "Exception id: " << e.id << std::endl;
		referenceLocations.clear();
		showReferenceOptions = false;
		return;
	}

	if (!referenceLocations.empty())
	{
		std::cout << "\033[32mLSP FindRef Parse:\033[0m Finished Parsing. Found "
				  << referenceLocations.size() << " reference location(s)." << std::endl;
		showReferenceOptions = true;
		selectedReferenceIndex = 0; // Reset selection
	} else
	{
		std::cout << "\033[33mLSP FindRef Parse:\033[0m Finished Parsing. No "
					 "valid reference locations were successfully extracted or "
					 "none were found."
				  << std::endl;
		showReferenceOptions = false; // Ensure this is false if no locations were added
	}
}
bool LSPGotoRef::hasReferenceOptions() const
{
	return showReferenceOptions && !referenceLocations.empty();
}

void LSPGotoRef::renderReferenceOptions()
{
	if (!showReferenceOptions || referenceLocations.empty())
	{
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

	float desiredWidth = 600.0f;
	desiredWidth = std::max(desiredWidth, 500.0f);
	ImVec2 windowSize(desiredWidth,
					  std::min(totalHeight, maxHeight) +
						  (referenceLocations.size() == 1 ? 10.0f : 25.0f));
	windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f);

	ImVec2 windowPos;
	if (isEmbedded)
	{
		// In embedded mode, position relative to the editor pane bounds
		// Get the editor pane bounds from the current ImGui window
		ImVec2 editorPanePos = ImGui::GetWindowPos();
		ImVec2 editorPaneSize = ImGui::GetWindowSize();

		// Position the popup within the editor pane bounds
		windowPos =
			ImVec2(editorPanePos.x + editorPaneSize.x * 0.5f - windowSize.x * 0.5f,
				   editorPanePos.y + editorPaneSize.y * 0.35f - windowSize.y * 0.5f);

		// Ensure the popup stays within the editor pane bounds
		if (windowPos.x < editorPanePos.x)
			windowPos.x = editorPanePos.x;
		if (windowPos.x + windowSize.x > editorPanePos.x + editorPaneSize.x)
			windowPos.x = editorPanePos.x + editorPaneSize.x - windowSize.x;
		if (windowPos.y < editorPanePos.y)
			windowPos.y = editorPanePos.y;
		if (windowPos.y + windowSize.y > editorPanePos.y + editorPaneSize.y)
			windowPos.y = editorPanePos.y + editorPaneSize.y - windowSize.y;
	} else
	{
		// In standalone mode, use the original positioning
		windowPos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f,
						   ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f);
	}

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoMouseInputs;

	// Style setup
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

	ImGui::PushStyleColor(
		ImGuiCol_WindowBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
			   1.0f));

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

	if (ImGui::Begin("##ReferenceOptions", nullptr, windowFlags))
	{
		// Handle click outside window
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 currentWindowPos = ImGui::GetWindowPos();
			ImVec2 currentWindowSize = ImGui::GetWindowSize();
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
				(mousePos.x < currentWindowPos.x ||
				 mousePos.x > currentWindowPos.x + currentWindowSize.x ||
				 mousePos.y < currentWindowPos.y ||
				 mousePos.y > currentWindowPos.y + currentWindowSize.y))
			{
				showReferenceOptions = false;
				editor_state.block_input = false;
			} else
			{
				showReferenceOptions = false;
				editor_state.block_input = false;
			}
		}

		// Handle escape key
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showReferenceOptions = false;
			editor_state.block_input = false;
		}

		// Fixed header
		ImGui::BeginChild("##Header", ImVec2(0, titleHeight), false);
		ImGui::Text("Find References (%zu)", referenceLocations.size());
		ImGui::Separator();
		ImGui::EndChild();

		// Scrollable content area
		float contentAvailableHeight =
			windowSize.y - titleHeight - footerHeight - padding * 2;
		ImGui::BeginChild("##ContentScroll",
						  ImVec2(0, contentAvailableHeight),
						  false,
						  ImGuiWindowFlags_HorizontalScrollbar |
							  ImGuiWindowFlags_AlwaysVerticalScrollbar |
							  ImGuiWindowFlags_NoMouseInputs);

		// Keyboard navigation
		if (!ImGui::IsAnyItemActive())
		{
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				selectedReferenceIndex = (selectedReferenceIndex > 0)
											 ? selectedReferenceIndex - 1
											 : referenceLocations.size() - 1;
				ImGui::SetScrollHereY(0.0f);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				selectedReferenceIndex =
					(selectedReferenceIndex + 1) % referenceLocations.size();
				ImGui::SetScrollHereY(1.0f);
			}
		}

		// List items
		for (size_t i = 0; i < referenceLocations.size(); i++)
		{
			const auto &loc = referenceLocations[i];
			bool is_selected = (selectedReferenceIndex == i);

			// Format filename
			std::string filename = loc.uri;
			size_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string::npos)
			{
				filename = filename.substr(lastSlash + 1);
			}
			std::string label = filename + ":" + std::to_string(loc.startLine + 1) + ":" +
								std::to_string(loc.startChar + 1);

			if (ImGui::Selectable(label.c_str(),
								  is_selected,
								  ImGuiSelectableFlags_AllowDoubleClick |
									  ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedReferenceIndex = i;
				if (ImGui::IsMouseDoubleClicked(0))
				{
					handleReferenceSelection();
				}
			}

			if (is_selected && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
								ImGui::IsKeyPressed(ImGuiKey_DownArrow)))
			{
				ImGui::SetScrollHereY();
			}
			if (is_selected && ImGui::IsWindowAppearing())
			{
				ImGui::SetScrollHereY();
			}
		}

		ImGui::EndChild(); // End content scroll area

		// Fixed footer
		ImGui::BeginChild("##Footer", ImVec2(0, footerHeight), false);
		ImGui::Separator();
		ImGui::Text("Up/Down Enter");
		ImGui::EndChild();

		// Handle Enter key
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
			ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
		{
			handleReferenceSelection();
		}

		ImGui::End(); // End main window
	} else
	{
		if (showReferenceOptions)
		{
			showReferenceOptions = false;
			editor_state.block_input = false;
		}
	}

	// Cleanup
	ImGui::PopStyleColor(6);
	ImGui::PopStyleVar(4);

	if (!showReferenceOptions)
	{
		editor_state.block_input = false;
	}
}

void LSPGotoRef::handleReferenceSelection()
{
	if (selectedReferenceIndex >= referenceLocations.size())
		return;

	const auto &selected = referenceLocations[selectedReferenceIndex];
	std::cout << "Selected reference at " << selected.uri << " line "
			  << (selected.startLine + 1) << std::endl;
	std::cout << "Input Block... " << editor_state.block_input << std::endl;

	// Close window first
	showReferenceOptions = false;

	if (selected.uri != gFileExplorer.currentFile)
	{
		gFileExplorer.loadFileContent(selected.uri, [selected]() {
			gEditorScroll.pending_cursor_centering = true;
			gEditorScroll.pending_cursor_line = selected.startLine;
			gEditorScroll.pending_cursor_char = selected.startChar;
		});
	} else
	{
		int index = 0;
		int currentLine = 0;
		const std::string &content = editor_state.fileContent;

		// Find the start of the target line
		while (currentLine < selected.startLine && index < static_cast<int>(content.length()))
		{
			if (content[index] == '\n')
				currentLine++;
			index++;
		}

		// Add character offset within the line, but make sure we don't go past the line end
		int lineStart = index;
		int lineEnd = index;
		while (lineEnd < static_cast<int>(content.length()) && content[lineEnd] != '\n') {
			lineEnd++;
		}
		
		// Clamp character position to be within the line bounds
		int charOffset = std::min(selected.startChar, lineEnd - lineStart);
		index = lineStart + charOffset;
		
		// Final bounds check
		index = std::max(0, std::min(index, static_cast<int>(content.length())));

		editor_state.cursor_index = index;
		editor_state.center_cursor_vertical = true;
		gEditorScroll.centerCursorVertically();
	}
	editor_state.block_input = false;
}