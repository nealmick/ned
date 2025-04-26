#include "editor_cursor.h"
#include "../util/settings.h"
#include "editor.h"
#include "editor_utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <string>
// Global instance
EditorCursor gEditorCursor;

EditorCursor::EditorCursor() {}
void EditorCursor::renderCursor()
{
	int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
	float cursor_x = getCursorXPosition(editor_state.text_pos,
										editor_state.fileContent,
										editor_state.cursor_index);

	ImVec2 cursor_start_pos = editor_state.text_pos;
	cursor_start_pos.x = cursor_x;
	cursor_start_pos.y = editor_state.text_pos.y + cursor_line * editor_state.line_height;

	ImVec2 cursor_end_pos =
		ImVec2(cursor_start_pos.x, cursor_start_pos.y + editor_state.line_height - 1);

	float blink_alpha = (sinf(editor_state.cursor_blink_time * 4.0f) + 1.0f) * 0.5f;
	ImU32 cursor_color;
	bool rainbow_mode = gSettings.getRainbowMode();

	if (rainbow_mode)
	{
		ImVec4 rainbow = EditorUtils::GetRainbowColor();
		cursor_color = ImGui::ColorConvertFloat4ToU32(rainbow);
	} else
	{
		cursor_color = IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
	}

	ImDrawList *draw_list = ImGui::GetWindowDrawList();

	const float cursor_thickness = 2.0f;

	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		draw_list->AddLine(cursor_start_pos, cursor_end_pos, cursor_color, cursor_thickness);
	}
}

// Cursor time management
void EditorCursor::updateBlinkTime() { editor_state.cursor_blink_time += ImGui::GetIO().DeltaTime; }

// Implementation of helper functions
void EditorCursor::calculateVisualColumn()
{
	const int TAB_WIDTH = 4; // Tab width in spaces
	int visual_column = 0;
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[current_line];

	for (int i = line_start; i < editor_state.cursor_index && i < editor_state.fileContent.length();
		 i++)
	{
		if (editor_state.fileContent[i] == '\t')
		{
			// Calculate next tab stop (tabs align to positions divisible by
			// TAB_WIDTH)
			visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
		} else
		{
			visual_column++;
		}
	}
	editor_state.cursor_column_prefered = visual_column;
}

void EditorCursor::findPositionFromVisualColumn(int line_start, int line_end)
{
	const int TAB_WIDTH = 4; // Tab width in spaces
	int current_visual_column = 0;
	int pos = line_start;

	while (pos < line_end && current_visual_column < editor_state.cursor_column_prefered &&
		   pos < editor_state.fileContent.length())
	{
		if (editor_state.fileContent[pos] == '\t')
		{
			// Calculate next tab stop
			int next_tab_stop = ((current_visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;

			// If going to or past the next tab stop would exceed our target,
			// we've found the closest position
			if (next_tab_stop > editor_state.cursor_column_prefered)
			{
				break;
			}

			current_visual_column = next_tab_stop;
		} else
		{
			current_visual_column++;
		}

		pos++;
	}
	editor_state.cursor_index = pos;
}

void EditorCursor::cursorLeft()
{
	if (editor_state.cursor_index > 0)
	{
		editor_state.cursor_index--;

		// We can't update the preferred visual column here since we don't have
		// access to text It will be updated next time cursor moves vertically
		int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
															editor_state.cursor_index);
		editor_state.cursor_column_prefered =
			editor_state.cursor_index - editor_state.editor_content_lines[current_line];
	}
}

// Fix for cursorRight
void EditorCursor::cursorRight()
{
	if (editor_state.cursor_index < editor_state.fileContent.size())
	{
		editor_state.cursor_index++;
		calculateVisualColumn();
	}
}

void EditorCursor::cursorUp()
{
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);
	if (current_line > 0)
	{
		// Calculate current visual column if preferred_column hasn't been set
		// yet
		if (editor_state.cursor_column_prefered == 0)
		{
			calculateVisualColumn();
		}

		// Target the previous line
		int target_line = current_line - 1;
		int new_line_start = editor_state.editor_content_lines[target_line];
		int new_line_end = editor_state.editor_content_lines[current_line] - 1;

		findPositionFromVisualColumn(new_line_start, new_line_end);

		// Update scroll position through EditorScroll
		ImVec2 currentPos = gEditorScroll.getScrollPosition();
		gEditorScroll.setScrollPosition(
			ImVec2(currentPos.x, std::max(0.0f, currentPos.y - editor_state.line_height)));
	}
}

void EditorCursor::cursorDown()
{
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);
	if (current_line < editor_state.editor_content_lines.size() - 1)
	{
		// Calculate current visual column if preferred_column hasn't been set
		// yet
		if (editor_state.cursor_column_prefered == 0)
		{
			calculateVisualColumn();
		}

		// Target the next line
		int target_line = current_line + 1;
		int new_line_start = editor_state.editor_content_lines[target_line];
		int new_line_end = (target_line + 1 < editor_state.editor_content_lines.size())
							   ? editor_state.editor_content_lines[target_line + 1] - 1
							   : editor_state.fileContent.size();

		// Find position in new line that corresponds to our visual column
		findPositionFromVisualColumn(new_line_start, new_line_end);

		// Update scroll position through EditorScroll
		ImVec2 currentPos = gEditorScroll.getScrollPosition();
		gEditorScroll.setScrollPosition(
			ImVec2(currentPos.x, currentPos.y + editor_state.line_height));
	}
}

void EditorCursor::moveCursorVertically(std::string &text, int line_delta)
{
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);
	int target_line =
		std::max(0,
				 std::min(static_cast<int>(editor_state.editor_content_lines.size()) - 1,
						  current_line + line_delta));

	// Calculate current visual column if preferred_column hasn't been set yet
	if (editor_state.cursor_column_prefered == 0)
	{
		calculateVisualColumn();
	}

	// Set the new cursor position using preferred visual column
	int new_line_start = editor_state.editor_content_lines[target_line];
	int new_line_end = (target_line + 1 < editor_state.editor_content_lines.size())
						   ? editor_state.editor_content_lines[target_line + 1] - 1
						   : text.size();

	// Find position in new line that corresponds to our visual column
	findPositionFromVisualColumn(new_line_start, new_line_end);
}

void EditorCursor::moveWordForward(const std::string &text)
{
	size_t pos = editor_state.cursor_index;
	const size_t len = text.length();

	if (pos >= len)
	{
		return; // Already at or past the end
	}

	// Phase 1: Skip any non-word characters immediately at or after the cursor.
	// This ensures that if we start in whitespace like "word1 | word2",
	// we move past the whitespace first.
	while (pos < len && !isWordChar(text[pos]))
	{
		++pos;
	}

	// Phase 2: Skip the word characters of the word we just landed on
	// (or the word we were already in if Phase 1 didn't move).
	// This moves the cursor to the position *after* the end of the word.
	while (pos < len && isWordChar(text[pos]))
	{
		++pos;
	}

	// The final position 'pos' is where the cursor should land.
	editor_state.cursor_index = pos - 1;
}

void EditorCursor::moveWordBackward(const std::string &text)
{
	size_t pos = editor_state.cursor_index;
	// No need to check text.length() here, pos == 0 check handles empty string

	if (pos == 0)
	{
		return; // Already at the beginning
	}

	// Phase 1: Skip any non-word characters immediately *before* the cursor.
	// We look at text[pos - 1]. This ensures that if we start like
	// "word1 |word2" (cursor at '|') we skip the space first.
	while (pos > 0 && !isWordChar(text[pos - 1]))
	{
		--pos;
	}

	// Phase 2: Skip the word characters of the word we just landed before
	// (or the word we were already in if Phase 1 didn't move).
	// This moves the cursor to the position *at the beginning* of that word.
	while (pos > 0 && isWordChar(text[pos - 1]))
	{
		--pos;
	}

	// The final position 'pos' is the beginning of the word we skipped over.
	editor_state.cursor_index = pos + 1;
}

float EditorCursor::getCursorYPosition(float line_height)
{
	int cursor_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
													   editor_state.cursor_index);
	return cursor_line * line_height;
}

float EditorCursor::getCursorXPosition(const ImVec2 &text_pos,
									   const std::string &text,
									   int cursor_pos)
{
	float x = text_pos.x;
	for (int i = 0; i < cursor_pos; i++)
	{
		if (text[i] == '\n')
		{
			x = text_pos.x;
		} else
		{
			x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
		}
	}
	return x;
}

void EditorCursor::handleCursorMovement(const std::string &text,
										const ImVec2 &text_pos,
										float line_height,
										float window_height,
										float window_width)
{
	bool shift_pressed = ImGui::GetIO().KeyShift;

	// Start a new selection only if Shift is pressed and we're not already
	// selecting
	if (shift_pressed && !editor_state.selection_active)
	{
		editor_state.selection_active = true;
		editor_state.selection_start = editor_state.cursor_index;
	}

	// Clear selection only if a movement key is pressed without Shift
	if (!shift_pressed &&
		(ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
		 ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)))
	{
		editor_state.selection_active = false;
		editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	}

	// Handle cursor movement based on arrow keys
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		cursorUp();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		cursorDown();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		cursorLeft();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		cursorRight();
	}

	// Update selection end if we're in selection mode
	if (editor_state.selection_active)
	{
		editor_state.selection_end = editor_state.cursor_index;
	}

	// Use the EditorScroll class to handle scroll adjustments
	gEditorScroll.handleCursorMovementScroll();
}
void EditorCursor::swapLines(int direction)
{
	// Handle line movement down
	if (direction == 1)
	{
		const int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
																  editor_state.cursor_index);

		// Boundary check: Can't move down from last line
		if (current_line >= editor_state.editor_content_lines.size() - 1)
			return;

		// Get current line boundaries and content
		const size_t line_start = editor_state.editor_content_lines[current_line];
		const size_t line_end = editor_state.editor_content_lines[current_line + 1];
		const std::string line_content =
			editor_state.fileContent.substr(line_start, line_end - line_start);
		const size_t cursor_offset = editor_state.cursor_index - line_start;

		// Calculate insertion point after next line
		size_t insert_pos = (current_line + 2 < editor_state.editor_content_lines.size())
								? editor_state.editor_content_lines[current_line + 2]
								: editor_state.fileContent.size();

		// Delete original line
		editor_state.fileContent.erase(line_start, line_end - line_start);
		editor_state.fileColors.erase(editor_state.fileColors.begin() + line_start,
									  editor_state.fileColors.begin() + line_end);

		// Adjust insertion position for the deleted content
		if (insert_pos > line_start)
			insert_pos -= (line_end - line_start);

		// Insert below next line
		editor_state.fileContent.insert(insert_pos, line_content);
		editor_state.fileColors.insert(editor_state.fileColors.begin() + insert_pos,
									   line_content.size(),
									   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		// Update line structure and cursor
		gEditor.updateLineStarts();
		const int new_line =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, insert_pos);
		const size_t new_line_start = editor_state.editor_content_lines[new_line];
		const size_t new_line_length =
			(new_line + 1 < editor_state.editor_content_lines.size())
				? editor_state.editor_content_lines[new_line + 1] - new_line_start
				: editor_state.fileContent.size() - new_line_start;

		editor_state.cursor_index = new_line_start + std::min(cursor_offset, new_line_length);
	}
	// Handle line movement up
	else if (direction == -1)
	{
		const int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
																  editor_state.cursor_index);
		const int target_line = current_line - 1;

		// Boundary check: Can't move up from first line
		if (target_line < 0)
			return;

		// Get current line boundaries and content
		const size_t line_start = editor_state.editor_content_lines[current_line];
		const size_t line_end = (current_line + 1 < editor_state.editor_content_lines.size())
									? editor_state.editor_content_lines[current_line + 1]
									: editor_state.fileContent.size();
		const std::string line_content =
			editor_state.fileContent.substr(line_start, line_end - line_start);
		const size_t cursor_offset = editor_state.cursor_index - line_start;

		// Delete original line
		editor_state.fileContent.erase(line_start, line_end - line_start);
		editor_state.fileColors.erase(editor_state.fileColors.begin() + line_start,
									  editor_state.fileColors.begin() + line_end);

		// Insert above target line
		const size_t insert_pos = editor_state.editor_content_lines[target_line];
		editor_state.fileContent.insert(insert_pos, line_content);
		editor_state.fileColors.insert(editor_state.fileColors.begin() + insert_pos,
									   line_content.size(),
									   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		// Update line structure and cursor
		gEditor.updateLineStarts();
		const size_t new_line_start = editor_state.editor_content_lines[target_line];
		const size_t new_line_length =
			(target_line + 1 < editor_state.editor_content_lines.size())
				? editor_state.editor_content_lines[target_line + 1] - new_line_start
				: editor_state.fileContent.size() - new_line_start;

		editor_state.cursor_index = new_line_start + std::min(cursor_offset, new_line_length);
	}

	// Common state updates
	editor_state.selection_active = false;
	editor_state.text_changed = true;
	editor_state.ensure_cursor_visible = {true, true};
}
void EditorCursor::processCursorJump(std::string &text, CursorVisibility &ensure_cursor_visible)
{
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
															editor_state.cursor_index);
		editor_state.cursor_index = editor_state.editor_content_lines[current_line] + 1;
		ensure_cursor_visible.horizontal = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
															editor_state.cursor_index);
		int next_line = current_line + 1;
		if (next_line < editor_state.editor_content_lines.size())
		{
			editor_state.cursor_index =
				editor_state.editor_content_lines[next_line] - 2; // Position before the newline
		} else
		{
			editor_state.cursor_index = text.size();
		}
		ensure_cursor_visible.horizontal = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		moveCursorVertically(text, -5);
		ensure_cursor_visible.vertical = true;
		ensure_cursor_visible.horizontal = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		moveCursorVertically(text, 5);
		ensure_cursor_visible.vertical = true;
		ensure_cursor_visible.horizontal = true;
	}
}

void EditorCursor::processWordMovement(std::string &text, CursorVisibility &ensure_cursor_visible)
{
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		moveWordBackward(text);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		moveWordForward(text);
	}
	ensure_cursor_visible.horizontal = true;
	ensure_cursor_visible.vertical = true;
}
