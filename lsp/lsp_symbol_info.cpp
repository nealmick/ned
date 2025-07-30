#include "lsp_symbol_info.h"
#include "../editor/editor.h"
#include <iostream>
#include <sstream>
LSPSymbolInfo gLSPSymbolInfo;

LSPSymbolInfo::LSPSymbolInfo() = default;

void LSPSymbolInfo::fetchSymbolInfo(const std::string &filePath)
{
	// Get current cursor position in editor
	int cursor_pos = editor_state.cursor_index;

	// Convert to line number and character offset
	int current_line = gEditor.getLineFromPos(cursor_pos);
	int line_start = editor_state.editor_content_lines[current_line];
	int character = cursor_pos - line_start;

	// LSP uses 0-based line numbers
	int lsp_line = current_line;
	int lsp_char = character;

	std::cout << "\033[35mLSP SymbolInfo:\033[0m Requesting hover at "
			  << "Line: " << lsp_line << " (" << current_line + 1 << "), "
			  << "Char: " << lsp_char << " (abs pos: " << cursor_pos << ")\n";

	if (!gLSPManager.isInitialized())
	{
		std::cout << "\033[31mLSP SymbolInfo:\033[0m LSP not initialized\n";
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
                "line": )" + std::to_string(lsp_line) +
									  R"(,
                "character": )" + std::to_string(lsp_char) +
									  R"(
            }
        }
    })");

	std::cout << "\033[36mLSP SymbolInfo Request:\033[0m\n" << request << "\n";

	if (!gLSPManager.sendRequest(request))
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
		std::string response = gLSPManager.readResponse(&contentLength);

		if (!response.empty())
		{
			std::cout << "\033[36mLSP SymbolInfo Response:\033[0m\n" << response << "\n";
		}

		if (response.find("\"id\":" + std::to_string(requestId - 1)) != std::string::npos)
		{
			parseHoverResponse(response);
			return;
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
						currentSymbolInfo.erase(pos, end - pos + 3);
					} else
					{
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