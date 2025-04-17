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
	if (!showCompletions || currentCompletionItems.empty())
	{
		// remvoed due to turning off blocking every frame...
		/*
		if (editor_state.block_input) {
			editor_state.block_input = false;
		}
		*/
		wasShowingLastFrame = false;
		return false;
	}
	return true;
}

bool LSPAutocomplete::handleInputAndCheckClose()
{
	if (!editor_state.block_input)
	{
		// std::cout << "[renderCompletions] Showing, setting block_input =
		// true" << std::endl; // Optional debug
	}
	editor_state.block_input = true;

	bool closeAndUnblock = false;
	bool navigationKeyPressed = false;

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		std::cout << "[renderCompletions] Escape pressed, hiding completions." << std::endl;
		closeAndUnblock = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		if (!currentCompletionItems.empty())
		{
			// Only decrement if not already at the top (index 0)
			if (selectedCompletionIndex > 0)
			{
				selectedCompletionIndex--;
			}
		}
		navigationKeyPressed = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		if (!currentCompletionItems.empty())
		{
			// Only increment if not already at the bottom
			if (selectedCompletionIndex < currentCompletionItems.size() - 1)
			{
				selectedCompletionIndex++;
			}
		}
		navigationKeyPressed = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
	{
		if (selectedCompletionIndex >= 0 && selectedCompletionIndex < currentCompletionItems.size())
		{
			blockEnter = true;
			const auto &selected_item = currentCompletionItems[selectedCompletionIndex];

			// Simplified debug output
			std::cout << "\n---ENTER PRESSED---\n";
			std::cout << "Selected text: " << selected_item.insertText << std::endl;
			std::cout << "Start position: L" << selected_item.startLine << " C"
					  << selected_item.startChar << std::endl;
			std::cout << "End position: L" << selected_item.endLine << " C" << selected_item.endChar
					  << std::endl;
			std::cout.flush(); // Force immediate output
			insertText(selected_item.startLine,
					   selected_item.startChar,
					   selected_item.endLine,
					   selected_item.endChar,
					   selected_item.insertText);

			// TODO: Insert text
		}
		closeAndUnblock = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		if (selectedCompletionIndex >= 0 && selectedCompletionIndex < currentCompletionItems.size())
		{
			blockTab = true;
			const auto &selected_item = currentCompletionItems[selectedCompletionIndex];
			// Simplified debug output
			std::cout << "\n---tab PRESSED---\n";
			std::cout << "Selected text: " << selected_item.insertText << std::endl;
			std::cout << "Start position: L" << selected_item.startLine << " C"
					  << selected_item.startChar << std::endl;
			std::cout << "End position: L" << selected_item.endLine << " C" << selected_item.endChar
					  << std::endl;
			std::cout.flush(); // Force immediate output
			insertText(selected_item.startLine,
					   selected_item.startChar,
					   selected_item.endLine,
					   selected_item.endChar,
					   selected_item.insertText);
		}
		closeAndUnblock = true;
	}

	if (!closeAndUnblock && !navigationKeyPressed)
	{
		ImGuiIO &io = ImGui::GetIO();
		if (io.InputQueueCharacters.Size > 0)
		{
			std::cout << "[renderCompletions] Character key pressed, hiding "
						 "completions."
					  << std::endl;
			closeAndUnblock = true;
		} else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
		{
			std::cout << "[renderCompletions] Left Arrow pressed, hiding completions." << std::endl;
			closeAndUnblock = true;
		} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
		{
			std::cout << "[renderCompletions] Right Arrow pressed, hiding "
						 "completions."
					  << std::endl;
			closeAndUnblock = true;
		}
	}

	// --- Process Closing Action ---
	if (closeAndUnblock)
	{
		editor_state.block_input = false;
		showCompletions = false;
		wasShowingLastFrame = false;
		// std::cout << "[renderCompletions] Closing: Set block_input = false"
		// << std::endl; // Optional debug
		return true;
	}

	return false;
}

void LSPAutocomplete::insertText(
	int row_start, int col__start, int row_end, int col__end, std::string text)
{
	int start_index = editor_state.editor_content_lines[row_start] + col__start;
	int end_index = editor_state.editor_content_lines[row_end] + col__end;
	if (start_index < 0 || end_index < 0 || start_index > end_index)
	{
		std::cerr << "Invalid positions - using cursor insertion" << std::endl;
		start_index = end_index = editor_state.cursor_index;
	}
	std::cout << "index start" << start_index << "index_end" << end_index << std::endl;

	// Delete existing content in both buffers
	if (start_index <= end_index && end_index <= editor_state.fileContent.size())
	{
		// Erase from fileContent
		editor_state.fileContent.erase(start_index, end_index - start_index);

		// Erase from fileColors
		auto colors_begin = editor_state.fileColors.begin() + start_index;
		auto colors_end = editor_state.fileColors.begin() + end_index;
		editor_state.fileColors.erase(colors_begin, colors_end);
	}

	// Insert new text and colors
	if (!text.empty())
	{
		// Insert into fileContent
		editor_state.fileContent.insert(start_index, text);

		// Insert into fileColors (using default color for new text)
		ImVec4 default_color(1.0f, 1.0f, 1.0f, 1.0f); // White as placeholder
		editor_state.fileColors.insert(editor_state.fileColors.begin() + start_index,
									   text.size(),
									   default_color);
	}
	editor_state.cursor_index = start_index + text.size();
	editor_state.text_changed = true;
	gEditorHighlight.highlightContent();
}

void LSPAutocomplete::calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos)
{
	const int current_item_count = currentCompletionItems.size();
	const float item_height = ImGui::GetTextLineHeightWithSpacing();
	const float window_padding = 5.0f;
	const float desired_width = 300.0f;
	const float max_visible_items = 10.0f;

	float current_list_height =
		std::min((float)current_item_count, max_visible_items) * item_height;
	outWindowSize = ImVec2(desired_width, current_list_height + window_padding * 2.0f);

	ImVec2 calculated_popup_anchor_pos = completionPopupPos;

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	outSafePos = calculated_popup_anchor_pos;
	float editor_char_height = editor_state.line_height;
	outSafePos.y += editor_char_height;

	if ((outSafePos.y + outWindowSize.y) > (viewport->Pos.y + viewport->Size.y - 5.0f))
	{
		outSafePos.y = calculated_popup_anchor_pos.y - outWindowSize.y - 2.0f;
	}
	if ((outSafePos.x + outWindowSize.x) > (viewport->Pos.x + viewport->Size.x - 5.0f))
	{
		outSafePos.x = calculated_popup_anchor_pos.x - outWindowSize.x;
	}
	if (outSafePos.x < (viewport->Pos.x + 5.0f))
	{
		outSafePos.x = viewport->Pos.x + 5.0f;
	}
	if (outSafePos.y < (viewport->Pos.y + 5.0f))
	{
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

	bool selection_changed_by_keyboard =
		ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow);

	if (use_child_window)
	{
		float child_height = max_visible_items * item_height;
		ImGui::BeginChild("##CompletionScroll", ImVec2(350.0f, child_height), false, 0);
		if (scroll_to_top)
		{
			ImGui::SetScrollY(0.0f);
		}
	}

	for (size_t i = 0; i < currentCompletionItems.size(); ++i)
	{
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

		if (is_selected)
		{
			draw_list->AddRectFilled(item_rect_min, item_rect_max, selection_color);
		}

		ImVec2 text_size = ImGui::CalcTextSize(item.insertText.c_str());
		float text_padding_y = (item_height - text_size.y) * 0.5f;
		text_padding_y = std::max(0.0f, text_padding_y);
		ImVec2 text_pos =
			ImVec2(item_rect_min.x + style.FramePadding.x, item_rect_min.y + text_padding_y);
		draw_list->AddText(text_pos, text_color, item.insertText.c_str());

		if (is_selected && selection_changed_by_keyboard)
		{
			ImGui::SetScrollHereY(0.5f);
		}
	}

	if (use_child_window)
	{
		ImGui::EndChild();
	}
}
bool LSPAutocomplete::handleClickOutside()
{
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
		ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			std::cout << "[renderCompletions] Clicked outside, hiding." << std::endl;
			showCompletions = false;
			editor_state.block_input = false; // Ensure unblocked
			wasShowingLastFrame = false;	  // Reset state
			return true;					  // Indicate closed by click outside
		}
	}
	return false;
}

void LSPAutocomplete::finalizeRenderState()
{
	if (!showCompletions && editor_state.block_input)
	{
		editor_state.block_input = false;
		std::cout << "[renderCompletions] Closed inside Begin/End, ensuring "
					 "input unblocked."
				  << std::endl;
	}
	wasShowingLastFrame = showCompletions;
}

void LSPAutocomplete::renderCompletions()
{
	if (!shouldRender())
	{
		return;
	}

	if (handleInputAndCheckClose())
	{
		return;
	}

	ImVec2 windowSize, safePos;
	calculateWindowGeometry(windowSize, safePos);

	if (showCompletions && !wasShowingLastFrame)
	{
		ImGui::SetNextWindowFocus();
	}

	ImGui::SetNextWindowPos(safePos);
	ImGui::SetNextWindowSize(windowSize);

	applyStyling(); // Now pushes 3 Colors, 3 Vars

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
								   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_NoScrollWithMouse |
								   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	if (ImGui::Begin("##CompletionPopupActual", nullptr, windowFlags))
	{
		renderCompletionListItems(); // Pushes/Pops 1 Color internally
		handleClickOutside();
		ImGui::End();
	} else
	{
		if (editor_state.block_input)
		{
			editor_state.block_input = false;
		}
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
	finalizeRenderState();
}
std::string LSPAutocomplete::formCompletionRequest(int requestId,
												   const std::string &filePath,
												   int line,
												   int character)
{
	// Determine if we're triggering after a '.' (common in Python)
	bool is_triggered_by_dot = false;
	if (editor_state.cursor_index > 0)
	{
		char prev_char = editor_state.fileContent[editor_state.cursor_index - 1];
		is_triggered_by_dot = (prev_char == '.');
	}

	return std::string(R"({
        "jsonrpc": "2.0",
        "id": )") +
		   std::to_string(requestId) +
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
                "triggerKind": )" +
		   (is_triggered_by_dot ? "1" : "2") + R"(,
                "triggerCharacter": )" +
		   (is_triggered_by_dot ? "\".\"" : "null") + R"(
            }
        }
    })";
}

bool LSPAutocomplete::processResponse(const std::string &response, int requestId)
{
	try
	{
		json j = json::parse(response);

		// Check if response matches our request ID
		if (!j.contains("id") || !j["id"].is_number_integer() || j["id"].get<int>() != requestId)
		{
			return false;
		}

		std::cout << "\033[32mLSP Autocomplete:\033[0m Received response for ID " << requestId
				  << std::endl;

		// Handle errors
		if (j.contains("error"))
		{
			std::cerr << "\033[31mLSP Autocomplete:\033[0m Error: " << j["error"].dump(2)
					  << std::endl;
			currentCompletionItems.clear();
			showCompletions = false;
			return true;
		}

		// Process result
		if (j.contains("result"))
		{
			parseCompletionResult(j["result"]);
			return true;
		}

		// Handle missing result
		std::cout << "\033[31mLSP Autocomplete:\033[0m Response missing "
					 "'result' field."
				  << std::endl;
		currentCompletionItems.clear();
		showCompletions = false;
		return true;
	} catch (const json::exception &e)
	{
		std::cerr << "\033[31mLSP Autocomplete:\033[0m JSON error: " << e.what() << std::endl;
		currentCompletionItems.clear();
		showCompletions = false;
		return true;
	}
}
void LSPAutocomplete::parseCompletionResult(const json &result)
{
	std::vector<json> items_json;
	bool is_incomplete = false;

	if (result.is_array())
	{
		items_json = result.get<std::vector<json>>();
	} else if (result.is_object())
	{
		if (result.contains("items") && result["items"].is_array())
		{
			items_json = result["items"].get<std::vector<json>>();
		}
		is_incomplete = result.value("isIncomplete", false);
	} else if (result.is_null())
	{
		std::cout << "\033[33mLSP Autocomplete:\033[0m No completions found "
					 "(result is null)."
				  << std::endl;
		currentCompletionItems.clear();
		showCompletions = false;
		return;
	} else
	{
		std::cout << "\033[31mLSP Autocomplete:\033[0m Unexpected result format: "
				  << result.type_name() << std::endl;
		currentCompletionItems.clear();
		showCompletions = false;
		return;
	}

	std::cout << "\033[32mFound " << items_json.size() << " completions"
			  << (is_incomplete ? " (incomplete list)" : "") << ":\033[0m" << std::endl;

	currentCompletionItems.clear();
	currentCompletionItems.reserve(items_json.size());

	for (const auto &item_json : items_json)
	{
		CompletionDisplayItem newItem;
		newItem.label = item_json.value("label", "[No Label]");
		newItem.detail = item_json.value("detail", "");
		newItem.kind = item_json.value("kind", 0);
		newItem.startLine = -1;
		newItem.startChar = -1;
		newItem.endLine = -1;
		newItem.endChar = -1;

		bool hasTextEdit = false;
		std::string positionInfo;

		// First try to get textEdit data
		if (item_json.contains("textEdit") && item_json["textEdit"].is_object())
		{
			const auto &textEdit = item_json["textEdit"];
			if (textEdit.contains("newText") && textEdit["newText"].is_string())
			{
				newItem.insertText = textEdit["newText"].get<std::string>();

				if (textEdit.contains("range") && textEdit["range"].is_object())
				{
					const auto &range = textEdit["range"];
					if (range.contains("start") && range["start"].is_object() &&
						range.contains("end") && range["end"].is_object())
					{
						const auto &start = range["start"];
						const auto &end = range["end"];

						newItem.startLine = start.value("line", -1);
						newItem.startChar = start.value("character", -1);
						newItem.endLine = end.value("line", -1);
						newItem.endChar = end.value("character", -1);

						if (newItem.startLine != -1)
						{
							positionInfo = "Replace from [L" + std::to_string(newItem.startLine) +
										   ":C" + std::to_string(newItem.startChar) + "] to [L" +
										   std::to_string(newItem.endLine) + ":C" +
										   std::to_string(newItem.endChar) + "]";
						}
					}
				}
				hasTextEdit = true;
			}
		}

		// If no textEdit, calculate word boundaries
		if (!hasTextEdit)
		{
			int cursor_pos = editor_state.cursor_index;

			// Find current word boundaries
			int word_start = cursor_pos;
			int word_end = cursor_pos;

			// Walk backwards to find word start (include underscores for
			// Python)
			while (word_start > 0 && (isalnum(editor_state.fileContent[word_start - 1]) ||
									  editor_state.fileContent[word_start - 1] == '_'))
			{
				word_start--;
			}

			// Walk forwards to find word end
			while (word_end < editor_state.fileContent.size() &&
				   (isalnum(editor_state.fileContent[word_end]) ||
					editor_state.fileContent[word_end] == '_'))
			{
				word_end++;
			}

			// Convert to line/character positions
			auto [startLine, startChar] = getLineAndCharFromIndex(word_start);
			auto [endLine, endChar] = getLineAndCharFromIndex(word_end);

			newItem.startLine = startLine;
			newItem.startChar = startChar;
			newItem.endLine = endLine;
			newItem.endChar = endChar;

			// Get insert text from either insertText or label
			if (item_json.contains("insertText") && item_json["insertText"].is_string())
			{
				newItem.insertText = item_json["insertText"].get<std::string>();
			} else
			{
				newItem.insertText = newItem.label;
			}

			positionInfo = "Replace current word";
		}

		currentCompletionItems.push_back(newItem);
	}

	// Update UI state
	if (!currentCompletionItems.empty())
	{
		updatePopupPosition();
		showCompletions = true;
		selectedCompletionIndex = 0;
	} else
	{
		showCompletions = false;
	}
}

void LSPAutocomplete::updatePopupPosition()
{
	try
	{
		int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
		float cursor_x = gEditorCursor.getCursorXPosition(editor_state.text_pos,
														  editor_state.fileContent,
														  editor_state.cursor_index);
		completionPopupPos = editor_state.text_pos;
		completionPopupPos.x = cursor_x;
		completionPopupPos.y += cursor_line * editor_state.line_height;
		std::cout << ">>> [requestCompletion] Stored Anchor Pos: (" << completionPopupPos.x << ", "
				  << completionPopupPos.y << ")" << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "!!! ERROR calculating cursor pos: " << e.what() << std::endl;
		completionPopupPos = ImVec2(0, 0);
	}
}

void LSPAutocomplete::requestCompletion(const std::string &filePath, int line, int character)
{
	if (!gLSPManager.isInitialized())
	{
		std::cout << "\033[31mLSP Autocomplete:\033[0m Not initialized" << std::endl;
		return;
	}

	if (!gLSPManager.selectAdapterForFile(filePath))
	{
		std::cout << "\033[31mLSP Autocomplete:\033[0m No LSP adapter "
					 "available for file: "
				  << filePath << std::endl;
		return;
	}

	int requestId = gEditorLSP.getNextRequestId();
	std::cout << "\033[35mLSP Autocomplete:\033[0m Requesting completions at line " << line
			  << ", char " << character << " (ID: " << requestId << ")" << std::endl;

	// Form and send request
	std::string request = formCompletionRequest(requestId, filePath, line, character);
	if (!gLSPManager.sendRequest(request))
	{
		std::cout << "\033[31mLSP Autocomplete:\033[0m Failed to send request" << std::endl;
		return;
	}

	// Await response
	const int MAX_ATTEMPTS = 15;
	const int WAIT_MS = 50;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
		int contentLength = 0;
		std::string response = gLSPManager.readResponse(&contentLength);

		if (response.empty())
		{
			if (attempt == 0)
				std::cout << "\033[35mLSP Autocomplete:\033[0m Waiting..." << std::endl;
			continue;
		}

		if (processResponse(response, requestId))
		{
			return; // Handled successfully
		}
	}

	std::cout << "\033[31mLSP Autocomplete:\033[0m Timed out waiting for response ID " << requestId
			  << "." << std::endl;
}