/*
	File: editor_render.cpp
	Description: Handles rendering for the editor, separating rendering logic
   from the main editor class.
*/

#include "editor_render.h"
#include "../files/file_finder.h"

#include "../lsp/lsp_client.h"
#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_tree_sitter.h"
#include "editor_utils.h"

#include <iostream>

EditorRender gEditorRender;

void EditorRender::renderEditorFrame()
{

	gEditorRender.renderEditorContent();

	gLineJump.renderLineJumpWindow();

	gFileFinder.renderWindow();

	// Render all LSP UI components
	gLSPClient.render();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + editor_state.total_height +
						 editor_state.editor_top_margin);
	ImGui::Dummy(ImVec2(0, 0)); // Required by ImGui 1.92+ to extend parent boundaries

	// Get final scroll positions from ImGui
	float scrollY = ImGui::GetScrollY();
	float scrollX = ImGui::GetScrollX();

	// Update scroll manager with final positions
	gEditorScroll.setScrollPosition(ImVec2(scrollX, scrollY));
	gEditorScroll.setScrollX(scrollX);

	// End the editor child window
	ImGui::EndChild();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(4);

	// Render line numbers with proper clipping
	ImGui::PushClipRect(editor_state.line_numbers_pos,
						ImVec2(editor_state.line_numbers_pos.x +
								   editor_state.line_number_width,
							   editor_state.line_numbers_pos.y + editor_state.size.y -
								   editor_state.editor_top_margin),
						true);

	gEditorLineNumbers.renderLineNumbers();

	// Cleanup
	ImGui::PopClipRect();
	ImGui::EndGroup();
	ImGui::PopID();
}
bool EditorRender::validateAndResizeColors()
{
	// Ensure theme colors are updated first
	TreeSitter::updateThemeColors();

	if (editor_state.fileColors.size() != editor_state.fileContent.size())
	{
		std::cout << "Warning: colors vector size (" << editor_state.fileColors.size()
				  << ") does not match text size (" << editor_state.fileContent.size()
				  << "). Resizing." << std::endl;

		// Use text color if available, otherwise fallback to white
		ImVec4 defaultColor = TreeSitter::cachedColors.text;
		if (defaultColor.x == 0 && defaultColor.y == 0 && defaultColor.z == 0)
		{
			defaultColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		std::cout << "set to default color.... " << std::endl;

		editor_state.fileColors.resize(editor_state.fileContent.size(), defaultColor);
		return true;
	}
	return false;
}
void EditorRender::setupEditorWindow(const char *label)
{
	editor_state.size = ImGui::GetContentRegionAvail();
	editor_state.line_number_width = ImGui::CalcTextSize("0").x * 4 + 8.0f;
	editor_state.line_height = ImGui::GetTextLineHeight();
	editor_state.editor_top_margin = 2.0f;
	editor_state.text_left_margin = 7.0f;
	ImGui::PushID(label);
}

void EditorRender::beginTextEditorChild(const char *label,
										float remaining_width,
										float content_width,
										float content_height)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
	ImGui::SetNextWindowContentSize(ImVec2(content_width, content_height));
	ImGui::BeginChild(label,
					  ImVec2(remaining_width, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_HorizontalScrollbar);

	// Check if this editor child window is focused and set block_input accordingly
	// We need to check if the parent window (editor pane) is focused, not just this child
	// window This prevents blocking input when clicking on file explorer within the
	// editor pane
	bool isEditorFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	// Only update block_input if the focus state has changed and no other component
	// has explicitly set block_input to true (like LSP popups)
	static bool wasEditorFocused = true; // Track previous focus state
	if (wasEditorFocused != isEditorFocused)
	{
		// Only set block_input to false if no other component has set it to true
		// This allows LSP popups and other components to control block_input
		if (isEditorFocused || !editor_state.block_input)
		{
			editor_state.block_input = !isEditorFocused;
		}
		wasEditorFocused = isEditorFocused;
	}

	// Set keyboard focus to this child window if appropriate.
	if (!gBookmarks.isWindowOpen() && !editor_state.block_input)
	{
		if (ImGui::IsWindowAppearing() ||
			(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			 !ImGui::IsAnyItemActive()))
		{
			ImGui::SetKeyboardFocusHere();
		}
	}

	// Get current scroll positions from ImGui
	editor_state.current_scroll_y = ImGui::GetScrollY();
	editor_state.current_scroll_x = ImGui::GetScrollX();

	// Update EditorScroll with the latest scroll values
	gEditorScroll.setScrollPosition(
		ImVec2(editor_state.current_scroll_x, editor_state.current_scroll_y));
	gEditorScroll.setScrollX(editor_state.current_scroll_x);

	editor_state.text_pos = ImGui::GetCursorScreenPos();
	editor_state.text_pos.y += editor_state.editor_top_margin;
	editor_state.text_pos.x += editor_state.text_left_margin;
}

void EditorRender::renderEditorContent()
{
	// Render whitespace guides first (behind text)
	renderWhitespaceGuides();

	// Render current line highlighting (behind text)
	renderCurrentLineHighlight();

	renderText();

	gEditorCursor.renderCursor();
}

void EditorRender::renderLineBackground(int line_num,
										int start_visible_line,
										int end_visible_line,
										size_t cursor_line,
										const ImVec2 &line_start_draw_pos)
{
	if (static_cast<size_t>(line_num) != cursor_line ||
		editor_state.selection_active &&
			editor_state.selection_start != editor_state.selection_end)
	{
		return; // Not the cursor line
	}
	if (line_num < start_visible_line || line_num > end_visible_line)
	{
		return; // Line is not visible
	}

	const ImU32 highlight_color =
		ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.18f, 0.18f, 0.3f));
	ImVec2 window_pos = ImGui::GetWindowPos();
	float window_width = ImGui::GetWindowWidth();

	float hl_x_start = window_pos.x + 5.0f; // Keep the offset as it was
	float hl_x_end = window_pos.x + window_width;
	float hl_y_start = line_start_draw_pos.y;
	float hl_y_end = line_start_draw_pos.y + editor_state.line_height;

	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(hl_x_start, hl_y_start),
											  ImVec2(hl_x_end, hl_y_end),
											  highlight_color);
}

void EditorRender::renderCharacterAndSelection(size_t char_index,
											   int selection_start,
											   int selection_end,
											   ImVec2 &current_draw_pos)
{
	if (char_index >= editor_state.fileColors.size())
	{
		return; // Skip rendering this char if color is missing
	}

	const char *char_start = &editor_state.fileContent[char_index];
	const char *char_end = (char_index + 1 < editor_state.fileContent.size())
							   ? &editor_state.fileContent[char_index + 1]
							   : nullptr;

	// Handle tab characters specially to avoid font-specific rendering issues
	if (*char_start == '\t')
	{
		// Calculate tab width based on current position
		float space_width = ImGui::CalcTextSize(" ").x;
		const int TAB_SIZE = 4;
		float current_column_pixels = current_draw_pos.x - editor_state.text_pos.x;
		int current_column = static_cast<int>(current_column_pixels / space_width);
		int next_tab_stop = ((current_column / TAB_SIZE) + 1) * TAB_SIZE;
		float tab_width = (next_tab_stop - current_column) * space_width;

		// Render selection if needed
		int s_start = selection_start;
		int s_end = selection_end;
		int current_char_idx = static_cast<int>(char_index);

		bool is_selected = (s_start <= s_end && // Normal order: start <= end
							current_char_idx >= s_start && current_char_idx < s_end) ||
						   (s_start > s_end && // Inverse order: start > end
							current_char_idx >= s_end && current_char_idx < s_start);

		if (!is_selected && editor_state.selection_active &&
			!editor_state.multi_selections.empty())
		{
			for (const auto &multi_sel : editor_state.multi_selections)
			{
				if (static_cast<int>(char_index) >=
						std::min(multi_sel.start_index, multi_sel.end_index) &&
					static_cast<int>(char_index) <
						std::max(multi_sel.start_index, multi_sel.end_index))
				{
					is_selected = true;
					break;
				}
			}
		}

		if (is_selected)
		{
			ImVec2 sel_start_pos = current_draw_pos;
			ImVec2 sel_end_pos = ImVec2(sel_start_pos.x + tab_width,
										sel_start_pos.y + editor_state.line_height);
			const ImU32 selection_color =
				ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
			ImGui::GetWindowDrawList()->AddRectFilled(sel_start_pos,
													  sel_end_pos,
													  selection_color);
		}

		// Don't render anything - just advance the cursor by tab width
		current_draw_pos.x += tab_width;
		return;
	}

	// For multi-byte characters (like emojis), we need to find the end of the character
	if (char_end && (*char_start & 0x80)) // Check if it's a multi-byte character
	{
		// Find the end of this UTF-8 character
		while (char_end < &editor_state.fileContent[editor_state.fileContent.size()] &&
			   (*char_end & 0xC0) == 0x80) // Continuation byte
		{
			char_end++;
		}
	}

	float char_width = ImGui::CalcTextSize(char_start, char_end).x;

	int s_start = selection_start; // Or editor_state.selection_start
	int s_end = selection_end;	   // Or editor_state.selection_end
	int current_char_idx = static_cast<int>(char_index);

	bool is_selected = (s_start <= s_end && // Normal order: start <= end
						current_char_idx >= s_start && current_char_idx < s_end) ||
					   (s_start > s_end && // Inverse order: start > end
						current_char_idx >= s_end && current_char_idx < s_start);

	if (!is_selected && editor_state.selection_active &&
		!editor_state.multi_selections.empty())
	{
		for (const auto &multi_sel : editor_state.multi_selections)
		{
			if (static_cast<int>(char_index) >=
					std::min(multi_sel.start_index, multi_sel.end_index) &&
				static_cast<int>(char_index) <
					std::max(multi_sel.start_index, multi_sel.end_index))
			{
				is_selected = true;
				break;
			}
		}
	}
	if (is_selected)
	{
		ImVec2 sel_start_pos = current_draw_pos;
		ImVec2 sel_end_pos = ImVec2(sel_start_pos.x + char_width,
									sel_start_pos.y + editor_state.line_height);
		const ImU32 selection_color =
			ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
		ImGui::GetWindowDrawList()->AddRectFilled(sel_start_pos,
												  sel_end_pos,
												  selection_color);
	}

	ImU32 text_color =
		ImGui::ColorConvertFloat4ToU32(editor_state.fileColors[char_index]);
	ImGui::GetWindowDrawList()->AddText(
		current_draw_pos, text_color, char_start, char_end);

	current_draw_pos.x += char_width;
}

bool EditorRender::skipLineIfAboveVisible(size_t &char_index,
										  int line_num,
										  int start_visible_line,
										  ImVec2 &current_draw_pos)
{
	if (line_num < start_visible_line)
	{
		while (char_index < editor_state.fileContent.size() &&
			   editor_state.fileContent[char_index] != '\n')
		{
			// Handle multi-byte UTF-8 characters
			if ((editor_state.fileContent[char_index] & 0x80) == 0)
			{
				// Single byte character
				char_index++;
			} else
			{
				// Multi-byte character, find the end
				while (char_index < editor_state.fileContent.size() &&
					   (editor_state.fileContent[char_index] & 0xC0) == 0x80)
				{
					char_index++;
				}
				char_index++; // Move past the last byte of the character
			}
		}
		return true;
	}
	return false;
}
void EditorRender::renderWhitespaceGuides()
{
	ImVec2 base_text_pos = editor_state.text_pos;
	const float scroll_x = ImGui::GetScrollX();
	const float scroll_y = ImGui::GetScrollY();
	const float window_height = ImGui::GetWindowHeight();
	const float line_height = editor_state.line_height;

	if (line_height <= 0.0f || editor_state.editor_content_lines.empty())
	{
		return;
	}

	// Calculate visible line range
	int start_line_idx = static_cast<int>(scroll_y / line_height);
	start_line_idx = std::max(0, start_line_idx - 2);
	int end_line_idx = static_cast<int>((scroll_y + window_height) / line_height);
	end_line_idx =
		std::min(static_cast<int>(editor_state.editor_content_lines.size() - 1),
				 end_line_idx + 2);

	if (start_line_idx > end_line_idx)
	{
		return;
	}

	// Calculate character width for spaces and tabs
	float space_width = ImGui::CalcTextSize(" ").x;
	float tab_width = space_width * 4; // Assuming 4-space tabs

	// Color for whitespace guides (subtle gray)
	const ImU32 guide_color =
		ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 0.4f));

	// Iterate through visible lines
	for (int line_num = start_line_idx; line_num <= end_line_idx; ++line_num)
	{
		size_t line_char_start_idx = editor_state.editor_content_lines[line_num];
		size_t line_char_end_idx;

		if (static_cast<size_t>(line_num + 1) < editor_state.editor_content_lines.size())
		{
			line_char_end_idx = editor_state.editor_content_lines[line_num + 1];
		} else
		{
			line_char_end_idx = editor_state.fileContent.size();
		}

		// Count leading whitespace and find first non-whitespace position
		int total_whitespace_chars = 0;
		size_t first_non_whitespace_idx = line_char_end_idx; // Default to end of line

		for (size_t i = line_char_start_idx; i < line_char_end_idx; ++i)
		{
			if (editor_state.fileContent[i] == ' ')
			{
				total_whitespace_chars++;
			} else if (editor_state.fileContent[i] == '\t')
			{
				total_whitespace_chars += 4; // Convert tab to 4 spaces
			} else if (editor_state.fileContent[i] != '\n')
			{
				// Found first non-whitespace character (excluding newline)
				first_non_whitespace_idx = i;
				break;
			} else
			{
				// Hit newline, this is an empty/whitespace-only line
				break;
			}
		}

		// Draw vertical guides for each indentation level (full height, shifted up)
		float line_y_start =
			base_text_pos.y + (static_cast<float>(line_num) * line_height) - 2.0f;
		float line_y_end = line_y_start + line_height;

		// Draw guides every 4 characters, but stop before where text begins
		for (int level = 1; level * 4 < total_whitespace_chars; ++level)
		{
			float guide_x =
				base_text_pos.x + (static_cast<float>(level * 4) * space_width);

			ImGui::GetWindowDrawList()->AddLine(ImVec2(guide_x, line_y_start),
												ImVec2(guide_x, line_y_end),
												guide_color,
												1.0f);
		}
	}
}

void EditorRender::renderCurrentLineHighlight()
{
	ImVec2 base_text_pos = editor_state.text_pos;
	const float scroll_x = ImGui::GetScrollX();
	const float scroll_y = ImGui::GetScrollY();
	const float window_height = ImGui::GetWindowHeight();
	const float line_height = editor_state.line_height;

	if (line_height <= 0.0f || editor_state.editor_content_lines.empty())
	{
		return;
	}

	// Find which line the cursor is on - optimized calculation
	static size_t cached_cursor_line = 0;
	static size_t cached_cursor_pos = SIZE_MAX;

	size_t cursor_pos = editor_state.cursor_index;
	size_t cursor_line = cached_cursor_line;

	// Only recalculate if cursor position changed
	if (cursor_pos != cached_cursor_pos)
	{
		// Start search from cached position for efficiency
		size_t search_start = (cached_cursor_line > 0) ? cached_cursor_line - 1 : 0;
		size_t search_end =
			std::min(cached_cursor_line + 2, editor_state.editor_content_lines.size());

		bool found = false;

		// Quick search around cached position first
		for (size_t i = search_start;
			 i < search_end && i < editor_state.editor_content_lines.size();
			 ++i)
		{
			size_t line_start = editor_state.editor_content_lines[i];
			size_t line_end = (i + 1 < editor_state.editor_content_lines.size())
								  ? editor_state.editor_content_lines[i + 1]
								  : editor_state.fileContent.size();

			if ((cursor_pos >= line_start && cursor_pos < line_end) ||
				(i == editor_state.editor_content_lines.size() - 1 &&
				 cursor_pos == line_end))
			{
				cursor_line = i;
				found = true;
				break;
			}
		}

		// If not found in quick search, do full search
		if (!found)
		{
			for (size_t i = 0; i < editor_state.editor_content_lines.size(); ++i)
			{
				size_t line_start = editor_state.editor_content_lines[i];
				size_t line_end = (i + 1 < editor_state.editor_content_lines.size())
									  ? editor_state.editor_content_lines[i + 1]
									  : editor_state.fileContent.size();

				if ((cursor_pos >= line_start && cursor_pos < line_end) ||
					(i == editor_state.editor_content_lines.size() - 1 &&
					 cursor_pos == line_end))
				{
					cursor_line = i;
					break;
				}
			}
		}

		// Update cache
		cached_cursor_line = cursor_line;
		cached_cursor_pos = cursor_pos;
	}

	// Calculate visible line range
	int start_line_idx = static_cast<int>(scroll_y / line_height);
	start_line_idx = std::max(0, start_line_idx - 2);
	int end_line_idx = static_cast<int>((scroll_y + window_height) / line_height);
	end_line_idx =
		std::min(static_cast<int>(editor_state.editor_content_lines.size() - 1),
				 end_line_idx + 2);

	// Only highlight if the cursor line is visible
	if (static_cast<int>(cursor_line) < start_line_idx ||
		static_cast<int>(cursor_line) > end_line_idx)
	{
		return;
	}

	// Calculate line position
	float line_y_start =
		base_text_pos.y + (static_cast<float>(cursor_line) * line_height);
	float line_y_end = line_y_start + line_height;

	// Get window bounds for full-width highlight (shifted 6px to the right)
	ImVec2 window_pos = ImGui::GetWindowPos();
	float window_width = ImGui::GetWindowWidth();
	float hl_x_start = window_pos.x + 6.0f;
	float hl_x_end = window_pos.x + window_width;

	// Subtle highlight color - light grey tint
	const ImU32 highlight_color =
		ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 0.08f));

	// Draw the highlight rectangle
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(hl_x_start, line_y_start),
											  ImVec2(hl_x_end, line_y_end),
											  highlight_color);
}

void EditorRender::renderText()
{
	ImVec2 base_text_pos = editor_state.text_pos; // Base screen position for text area

	const float scroll_x = ImGui::GetScrollX(); // Current horizontal scroll
	const float scroll_y = ImGui::GetScrollY(); // Current vertical scroll
	// Use ImGui::GetContentRegionAvail().y for window_height if you are inside
	// a child window or ImGui::GetWindowHeight() if 'this' is the main editor
	// window. For a child window like "##editor", ImGui::GetWindowHeight()
	// refers to child window height.
	const float window_height = ImGui::GetWindowHeight();
	const float window_width = ImGui::GetWindowWidth();
	const float line_height = editor_state.line_height;

	if (line_height <= 0.0f || editor_state.editor_content_lines.empty())
	{
		return; // Nothing to render or invalid state
	}

	// 1. Calculate the range of line numbers that are visible.
	//    `start_line_idx` is the 0-based index for editor_content_lines.
	const int VIRTUAL_RENDER_BUFFER_LINES = 2; // Render a few lines above/below viewport
	int start_line_idx = static_cast<int>(scroll_y / line_height);
	start_line_idx = std::max(0, start_line_idx - VIRTUAL_RENDER_BUFFER_LINES);

	int end_line_idx = static_cast<int>((scroll_y + window_height) / line_height);
	end_line_idx =
		std::min(static_cast<int>(editor_state.editor_content_lines.size() - 1),
				 end_line_idx + VIRTUAL_RENDER_BUFFER_LINES);

	if (start_line_idx > end_line_idx)
	{
		return; // No lines in the visible range
	}

	// Horizontal culling values (for characters on a visible line)
	const float visible_x_start_cull =
		scroll_x - 100.0f; // Cull chars starting before this
	const float visible_x_end_cull =
		scroll_x + window_width + 100.0f; // Cull chars starting after this

	ImVec2 current_draw_pos; // Will be set for each line

	// 2. Iterate *only* through the visible lines using editor_content_lines.
	for (int line_num = start_line_idx; line_num <= end_line_idx; ++line_num)
	{
		// Determine the character range for the current line_num
		size_t line_char_start_idx = editor_state.editor_content_lines[line_num];
		size_t line_char_end_idx; // Exclusive: points to the newline or end of file

		if (static_cast<size_t>(line_num + 1) < editor_state.editor_content_lines.size())
		{
			line_char_end_idx = editor_state.editor_content_lines[line_num + 1];
		} else // This is the last line of the file
		{
			line_char_end_idx = editor_state.fileContent.size();
		}

		// Set the drawing position for the start of this line.
		// The Y position is relative to the top of the document, ImGui handles
		// scrolling it into view.
		current_draw_pos.x = base_text_pos.x;
		current_draw_pos.y =
			base_text_pos.y + (static_cast<float>(line_num) * line_height);

		// 3. Iterate through characters *of this specific line*.
		for (size_t char_idx_in_file = line_char_start_idx;
			 char_idx_in_file < line_char_end_idx;)
		{
			// Skip continuation bytes of multi-byte characters
			if (char_idx_in_file < editor_state.fileContent.size() &&
				(editor_state.fileContent[char_idx_in_file] & 0xC0) == 0x80)
			{
				char_idx_in_file++;
				continue;
			}

			// This character is (at least partially) horizontally visible.
			renderCharacterAndSelection(
				char_idx_in_file,
				editor_state.selection_start,
				editor_state.selection_end,
				current_draw_pos); // This function advances current_draw_pos.x

			if (editor_state.fileContent[char_idx_in_file] == '\n')
			{
				break; // Reached end of current line's content (before
					   // line_char_end_idx if line_char_end_idx pointed to
					   // newline itself)
			}

			// Advance to next character, handling multi-byte UTF-8 characters
			if (char_idx_in_file < editor_state.fileContent.size())
			{
				const char *current_char = &editor_state.fileContent[char_idx_in_file];
				if ((*current_char & 0x80) == 0)
				{
					// Single byte character
					char_idx_in_file++;
				} else
				{
					// Multi-byte character, find the end
					while (char_idx_in_file < editor_state.fileContent.size() &&
						   (editor_state.fileContent[char_idx_in_file] & 0xC0) == 0x80)
					{
						char_idx_in_file++;
					}
					char_idx_in_file++; // Move past the last byte of the character
				}
			} else
			{
				char_idx_in_file++;
			}
		}
		// If the line ended without a newline char (e.g., last line of file),
		// the inner loop completes when char_idx_in_file == line_char_end_idx.
	}
}