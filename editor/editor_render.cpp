/*
	File: editor_render.cpp
	Description: Handles rendering for the editor, separating rendering logic
   from the main editor class.
*/

#include "editor_render.h"
#include "../files/file_finder.h"
#include "../lsp/lsp.h"
#include "../lsp/lsp_autocomplete.h"
#include "../lsp/lsp_goto_def.h"
#include "../lsp/lsp_goto_ref.h"
#include "../lsp/lsp_symbol_info.h"
#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_utils.h"

#include <iostream>

EditorRender gEditorRender;

void EditorRender::renderEditorFrame()
{
	gEditorRender.renderEditorContent();

	gFileFinder.renderWindow();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + editor_state.total_height +
						 editor_state.editor_top_margin);

	// Get final scroll positions from ImGui
	float scrollY = ImGui::GetScrollY();
	float scrollX = ImGui::GetScrollX();

	// Update scroll manager with final positions
	gEditorScroll.setScrollPosition(ImVec2(scrollX, scrollY));
	gEditorScroll.setScrollX(scrollX);

	if (gLSPGotoDef.hasDefinitionOptions())
	{
		gLSPGotoDef.renderDefinitionOptions();
	}
	if (gLSPGotoRef.hasReferenceOptions())
	{
		gLSPGotoRef.renderReferenceOptions();
	}

	gLSPSymbolInfo.renderSymbolInfo();
	// End the editor child window
	ImGui::EndChild();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(4);

	// Render line numbers with proper clipping
	ImGui::PushClipRect(editor_state.line_numbers_pos,
						ImVec2(editor_state.line_numbers_pos.x + editor_state.line_number_width,
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
	if (editor_state.fileColors.size() != editor_state.fileContent.size())
	{
		std::cout << "Warning: colors vector size (" << editor_state.fileColors.size()
				  << ") does not match text size (" << editor_state.fileContent.size()
				  << "). Resizing." << std::endl;
		editor_state.fileColors.resize(editor_state.fileContent.size(),
									   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
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
	gLineJump.renderLineJumpWindow();

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
		editor_state.selection_active && editor_state.selection_start != editor_state.selection_end)
	{
		return; // Not the cursor line
	}
	if (line_num < start_visible_line || line_num > end_visible_line)
	{
		return; // Line is not visible
	}

	const ImU32 highlight_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.18f, 0.18f, 0.3f));
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
	float char_width = ImGui::CalcTextSize(char_start, char_end).x;

	bool is_selected = (static_cast<int>(char_index) >= selection_start &&
						static_cast<int>(char_index) < selection_end);
	if (is_selected)
	{
		ImVec2 sel_start_pos = current_draw_pos;
		ImVec2 sel_end_pos =
			ImVec2(sel_start_pos.x + char_width, sel_start_pos.y + editor_state.line_height);
		const ImU32 selection_color =
			ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
		ImGui::GetWindowDrawList()->AddRectFilled(sel_start_pos, sel_end_pos, selection_color);
	}

	char current_char = editor_state.fileContent[char_index];
	char buf[2] = {current_char, '\0'};
	ImU32 text_color = ImGui::ColorConvertFloat4ToU32(editor_state.fileColors[char_index]);
	ImGui::GetWindowDrawList()->AddText(current_draw_pos, text_color, buf);

	current_draw_pos.x += char_width;
}

bool EditorRender::skipLineIfAboveVisible(size_t &char_index,
										  int line_num,
										  int start_visible_line,
										  ImVec2 &current_draw_pos)
{
	if (line_num < start_visible_line)
	{
		// Fast-forward index 'i' to the end of this line
		while (char_index < editor_state.fileContent.size() &&
			   editor_state.fileContent[char_index] != '\n')
		{
			char_index++;
		}
		// Note: The main loop's i++ will handle moving past the newline or end
		// of file. We just need to adjust the draw pos and line num for the
		// *next* iteration. The actual update happens after the loop check or
		// newline handling in the main function.
		return true;
	}
	return false;
}
void EditorRender::renderText()
{
	ImVec2 base_text_pos = editor_state.text_pos; // Base screen position for text area

	const float scroll_x = ImGui::GetScrollX(); // Current horizontal scroll
	const float scroll_y = ImGui::GetScrollY(); // Current vertical scroll
	// Use ImGui::GetContentRegionAvail().y for window_height if you are inside a child window
	// or ImGui::GetWindowHeight() if 'this' is the main editor window.
	// For a child window like "##editor", ImGui::GetWindowHeight() refers to child window height.
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
	end_line_idx = std::min(static_cast<int>(editor_state.editor_content_lines.size() - 1),
							end_line_idx + VIRTUAL_RENDER_BUFFER_LINES);

	if (start_line_idx > end_line_idx)
	{
		return; // No lines in the visible range
	}

	// Horizontal culling values (for characters on a visible line)
	const float visible_x_start_cull = scroll_x - 100.0f; // Cull chars starting before this
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
		// The Y position is relative to the top of the document, ImGui handles scrolling it into
		// view.
		current_draw_pos.x = base_text_pos.x;
		current_draw_pos.y = base_text_pos.y + (static_cast<float>(line_num) * line_height);

		// 3. Iterate through characters *of this specific line*.
		for (size_t char_idx_in_file = line_char_start_idx; char_idx_in_file < line_char_end_idx;
			 ++char_idx_in_file)
		{

			// This character is (at least partially) horizontally visible.
			renderCharacterAndSelection(
				char_idx_in_file,
				gEditorSelection.getSelectionStart(),
				gEditorSelection.getSelectionEnd(),
				current_draw_pos); // This function advances current_draw_pos.x

			if (editor_state.fileContent[char_idx_in_file] == '\n')
			{
				break; // Reached end of current line's content (before line_char_end_idx if
					   // line_char_end_idx pointed to newline itself)
			}
		}
		// If the line ended without a newline char (e.g., last line of file),
		// the inner loop completes when char_idx_in_file == line_char_end_idx.
	}
}