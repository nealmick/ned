#include "lsp_symbol_info.h"
#include "../editor/editor.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <set>
#include <algorithm>

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

LSPSymbolInfo gLSPSymbolInfo;

LSPSymbolInfo::LSPSymbolInfo() = default;

void LSPSymbolInfo::fetchSymbolInfo(const std::string &filePath)
{
	// Clear any previous symbol info to ensure we can process new requests
	currentSymbolInfo.clear();
	showSymbolInfo = false;

#ifdef PLATFORM_WINDOWS
	// Windows-specific file type checking - only support Python for now
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos) {
		return; // No extension
	}
	std::string ext = filePath.substr(dot_pos + 1);
	if (ext != "py") {
		// Only support Python files for now on Windows
		return;
	}

	// Select adapter and auto-initialize if needed on Windows
	if (!CURRENT_LSP_MANAGER.selectAdapterForFile(filePath)) {
		std::cout << "\033[33mLSP SymbolInfo:\033[0m No adapter available for file: " << filePath << std::endl;
		return;
	}

	if (!CURRENT_LSP_MANAGER.isInitialized()) {
		// Extract workspace path from file path
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		std::cout << "\033[35mLSP SymbolInfo:\033[0m Auto-initializing with workspace: " << workspacePath << std::endl;
		
		if (!CURRENT_LSP_MANAGER.initialize(workspacePath)) {
			std::cout << "\033[31mLSP SymbolInfo:\033[0m Failed to initialize LSP" << std::endl;
			return;
		}
	}
#else
	// Linux/macOS: Just check if initialized
	if (!CURRENT_LSP_MANAGER.isInitialized())
	{
		std::cout << "\033[31mLSP SymbolInfo:\033[0m LSP not initialized\n";
		return;
	}
#endif

	// Get current cursor position in editor
	int cursor_pos = editor_state.cursor_index;

	// Convert to line number and character offset
	int current_line = gEditor.getLineFromPos(cursor_pos);
	
	// Validate line number
	if (current_line < 0 || current_line >= static_cast<int>(editor_state.editor_content_lines.size())) {
		std::cout << "\033[31mLSP SymbolInfo:\033[0m Invalid cursor line: " << current_line << std::endl;
		return;
	}
	
	int line_start = editor_state.editor_content_lines[current_line];
	int character = cursor_pos - line_start;

	// Ensure character offset is non-negative (same as goto def/ref)
	character = std::max(0, character);
	
	// Additional validation: ensure character doesn't exceed line length
	if (current_line + 1 < static_cast<int>(editor_state.editor_content_lines.size())) {
		int next_line_start = editor_state.editor_content_lines[current_line + 1];
		int line_length = next_line_start - line_start - 1; // -1 for newline
		character = std::min(character, line_length);
	} else {
		// Last line - check against file content length
		int line_length = static_cast<int>(editor_state.fileContent.length()) - line_start;
		character = std::min(character, line_length);
	}

	// LSP uses 0-based line numbers
	int lsp_line = current_line;
	int lsp_char = character;

	std::cout << "\033[35mLSP SymbolInfo:\033[0m Requesting hover at "
			  << "Line: " << lsp_line << " (" << current_line + 1 << "), "
			  << "Char: " << lsp_char << " (abs pos: " << cursor_pos << ")\n";

	static int requestId = 5000;
	int currentRequestId = requestId++;

#ifdef PLATFORM_WINDOWS
	// Convert Windows path to proper URI format
	std::string fileURI = "file:///" + filePath;
	// Replace backslashes with forward slashes
	for (char &c : fileURI) {
		if (c == '\\') c = '/';
	}

	// Send didOpen notification if needed (same logic as goto definition)
	static std::set<std::string> openedFiles;
	if (openedFiles.find(fileURI) == openedFiles.end()) {
		// Read the file content
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

			std::cout << "\033[35mLSP SymbolInfo:\033[0m Sending didOpen notification for file" << std::endl;
			CURRENT_LSP_MANAGER.sendRequest(didOpenRequest);
			openedFiles.insert(fileURI);
			Sleep(150); // Wait a bit longer for the file to be processed
		}
	}

	std::string uriForRequest = fileURI;
#else
	std::string uriForRequest = "file://" + filePath;
#endif

	std::string request = std::string(R"({
        "jsonrpc": "2.0",
        "id": )" + std::to_string(currentRequestId) +
									  R"(,
        "method": "textDocument/hover",
        "params": {
            "textDocument": {
                "uri": ")" + uriForRequest +
									  R"("
            },
            "position": {
                "line": )" + std::to_string(lsp_line) +
									  R"(,
                "character": )" + std::to_string(lsp_char) +
									  R"(
            }
        }
    })");

	std::cout << "\033[36mLSP SymbolInfo Request:\033[0m\n" << request << "\n";

	if (!CURRENT_LSP_MANAGER.sendRequest(request))
	{
		std::cout << "\033[31mLSP SymbolInfo:\033[0m Failed to send hover request\n";
		return;
	}

	// Store display position
	displayPosition = ImGui::GetMousePos();
	displayPosition.x += 20;

	// Response handling with timeout
	const int MAX_ATTEMPTS = 15;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
	{
		int contentLength = 0;
		std::string response = CURRENT_LSP_MANAGER.readResponse(&contentLength);

		if (!response.empty())
		{
			std::cout << "\033[36mLSP SymbolInfo Response:\033[0m\n" << response << "\n";
		}

		if (response.find("\"id\":" + std::to_string(currentRequestId)) != std::string::npos)
		{
			parseHoverResponse(response);
			return;
		}

		// Add small delay between attempts (same as goto def/ref)
		if (response.empty())
		{
#ifdef PLATFORM_WINDOWS
			Sleep(50);
#else
			usleep(50000);
#endif
		}
	}
}

void LSPSymbolInfo::parseHoverResponse(const std::string &response)
{
	currentSymbolInfo.clear();
	showSymbolInfo = false;

	try
	{
		json j = json::parse(response);
		std::cout << "\033[36mRaw JSON Response:\033[0m\n" << j.dump(4) << "\n";

		if (j.contains("result") && !j["result"].is_null())
		{
			auto result = j["result"];

			// Handle different LSP server implementations
			if (result.is_object())
			{
				// Clangd style: {"contents":{"kind":"markdown","value":"..."}}
				if (result.contains("contents"))
				{
					auto contents = result["contents"];

					// Handle markdown content
					if (contents.is_object() && contents.contains("value"))
					{
						currentSymbolInfo = contents["value"].get<std::string>();
					}
					// Handle plain string
					else if (contents.is_string())
					{
						currentSymbolInfo = contents.get<std::string>();
					}
					// Handle array of content objects
					else if (contents.is_array())
					{
						for (auto &item : contents)
						{
							if (item.is_object() && item.contains("value"))
							{
								currentSymbolInfo +=
									item["value"].get<std::string>() + "\n";
							} else if (item.is_string())
							{
								currentSymbolInfo += item.get<std::string>() + "\n";
							}
						}
					}
				}
			}
			// Handle Python LSP servers (pyright/pylsp)
			else if (result.is_string())
			{
				currentSymbolInfo = result.get<std::string>();
			}

			// Clean up Markdown formatting and special symbols
			if (!currentSymbolInfo.empty())
			{
				// Replace Unicode right arrow (→) with text arrow
				size_t arrow_pos = 0;
				while ((arrow_pos = currentSymbolInfo.find("→", arrow_pos)) !=
					   std::string::npos)
				{
					currentSymbolInfo.replace(arrow_pos,
											  3,
											  "->"); // Unicode arrow is 3 bytes
					arrow_pos += 2;					 // Move past the replacement
				}

				// Remove code blocks but keep content
				size_t pos = 0;
				while ((pos = currentSymbolInfo.find("```", pos)) != std::string::npos)
				{
					size_t end = currentSymbolInfo.find("```", pos + 3);
					if (end != std::string::npos)
					{
						// Find the start of actual content (after language identifier)
						size_t content_start = currentSymbolInfo.find("\n", pos + 3);
						if (content_start != std::string::npos)
						{
							content_start++; // Move past the newline
							// Extract the content between the code blocks
							std::string content = currentSymbolInfo.substr(content_start, end - content_start);
							// Replace the entire code block with just the content
							currentSymbolInfo.replace(pos, end - pos + 3, content);
							pos += content.length();
						}
						else
						{
							// No newline found, just remove the opening ```
							currentSymbolInfo.erase(pos, 3);
							end = currentSymbolInfo.find("```", pos);
							if (end != std::string::npos)
							{
								currentSymbolInfo.erase(end, 3);
							}
						}
					} else
					{
						// Only opening ```, remove it
						currentSymbolInfo.erase(pos, 3);
					}
				}

				// Remove excess newlines
				pos = 0;
				while ((pos = currentSymbolInfo.find("\n\n\n", pos)) != std::string::npos)
				{
					currentSymbolInfo.replace(pos, 3, "\n\n");
				}

				showSymbolInfo = true;
				std::cout << "\033[32mProcessed Hover Content:\033[0m\n"
						  << currentSymbolInfo << "\n";
				return;
			}
		}

		if (j.contains("error"))
		{
			std::cerr << "\033[31mLSP Error:\033[0m " << j["error"].dump() << "\n";
		}

		std::cout << "\033[33mNo hover information found in response\033[0m\n";
	} catch (const json::exception &e)
	{
		std::cerr << "\033[31mJSON Parsing Error:\033[0m " << e.what()
				  << "\nResponse Data:\n"
				  << response << "\n";
	}
}

void LSPSymbolInfo::renderSymbolInfo()
{
	if (!hasSymbolInfo())
		return;

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
	displayPosition.x += 5.0f;							  // Small horizontal offset

	// Width configuration
	const float min_width = 450.0f;
	const float max_width = 650.0f;
	const float screen_padding = 20.0f;
	const float text_wrap_padding = 30.0f;

	// Set initial window position and size constraints
	ImGui::SetNextWindowPos(displayPosition, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(min_width, 0), // Minimum size
										ImVec2(max_width,
											   FLT_MAX) // Maximum size
	);

	// Style settings
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
	ImGui::PushStyleColor(
		ImGuiCol_WindowBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
			   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

	if (ImGui::Begin("SymbolInfoWindow",
					 &showSymbolInfo,
					 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoNav))
	{
		// Get current window dimensions
		ImVec2 window_pos = ImGui::GetWindowPos();
		ImVec2 window_size = ImGui::GetWindowSize();
		ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;

		// Calculate available space with new width considerations
		ImVec2 clamped_pos = window_pos;
		const float right_bound = work_size.x - window_size.x - screen_padding;
		const float bottom_bound = work_size.y - window_size.y - screen_padding;

		// Horizontal clamping
		if (window_pos.x < screen_padding)
		{
			clamped_pos.x = screen_padding;
		} else if (window_pos.x > right_bound)
		{
			clamped_pos.x = right_bound;
		}

		// Vertical clamping with flip to above cursor if needed
		if (window_pos.y < screen_padding)
		{
			clamped_pos.y = screen_padding;
		} else if (window_pos.y > bottom_bound)
		{
			// If window would be cutoff at bottom, display above cursor
			clamped_pos.y = cursor_screen_pos.y - window_size.y - 15.0f;
			// Ensure we don't go above the screen
			clamped_pos.y = std::max(clamped_pos.y, screen_padding);
		}

		// FIXED: Compare ImVec2 components individually
		if (clamped_pos.x != window_pos.x || clamped_pos.y != window_pos.y)
		{
			ImGui::SetWindowPos(clamped_pos);
			// Update window size after position change
			window_size = ImGui::GetWindowSize();
		}

		// Force reasonable minimum width
		if (window_size.x < min_width)
		{
			ImGui::SetWindowSize(ImVec2(min_width, window_size.y));
		}

		// Focus handling
		if (ImGui::IsWindowAppearing())
		{
			ImGui::SetWindowFocus();
		}

		// Dismissal interactions
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			{
				showSymbolInfo = false;
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showSymbolInfo = false;
		}

		// Content rendering with fixed wrapping
		ImGui::PushTextWrapPos(max_width - text_wrap_padding);
		std::istringstream iss(currentSymbolInfo);
		std::string line;

		while (std::getline(iss, line))
		{
			if (line.empty())
				continue;

			// Style different parts
			if (line.find("Type:") == 0)
			{
				ImGui::TextUnformatted(line.c_str());
			} else if (line.find("File:") == 0)
			{
				ImGui::TextUnformatted(line.c_str());
			} else if (line.find("//") == 0)
			{
				ImGui::TextUnformatted(line.c_str());
			} else
			{
				ImGui::TextWrapped("%s", line.c_str());
			}
		}
		ImGui::PopTextWrapPos();

		ImGui::End();
	}

	// Cleanup
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}