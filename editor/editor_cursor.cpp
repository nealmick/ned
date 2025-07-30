#include "editor_cursor.h"
#include "../files/files.h"
#include "../lib/utfcpp/source/utf8.h"
#include "../util/settings.h"
#include "editor.h"
#include "editor/utf8_utils.h"
#include "editor_utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <string>

EditorCursor gEditorCursor;

EditorCursor::EditorCursor() {}
void EditorCursor::spawnCursorAbove()
{
	std::cout << "Spawning cursor above" << std::endl;
	// Store original cursor state
	const int original_cursor = editor_state.cursor_index;

	// Calculate character offset (not visual column)
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														original_cursor);
	int original_char_offset =
		original_cursor - editor_state.editor_content_lines[current_line];

	// Find current line
	if (current_line == 0)
		return;

	// Calculate new cursor position
	const int target_line = current_line - 1;
	const int target_start = editor_state.editor_content_lines[target_line];
	const int target_end = (target_line + 1 < editor_state.editor_content_lines.size())
							   ? editor_state.editor_content_lines[target_line + 1] - 1
							   : editor_state.fileContent.size();

	// Find position using CHARACTER OFFSET (not visual column)
	int new_cursor =
		target_start + std::min(original_char_offset, target_end - target_start);

	// Add original cursor to multi-cursors
	editor_state.multi_cursor_indices.push_back(original_cursor);
	editor_state.multi_cursor_prefered_columns.push_back(original_char_offset);

	// Update main cursor to new position
	editor_state.cursor_index = new_cursor;
	editor_state.cursor_column_prefered = new_cursor - target_start; // Character offset
}

void EditorCursor::spawnCursorBelow()
{
	// Store original cursor state
	const int original_cursor = editor_state.cursor_index;

	// Calculate character offset (not visual column)
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														original_cursor);
	int original_char_offset =
		original_cursor - editor_state.editor_content_lines[current_line];

	// Boundary check
	if (current_line >= editor_state.editor_content_lines.size() - 1)
		return;

	// Calculate new cursor position
	const int target_line = current_line + 1;
	const int target_start = editor_state.editor_content_lines[target_line];
	const int target_end = (target_line + 1 < editor_state.editor_content_lines.size())
							   ? editor_state.editor_content_lines[target_line + 1] - 1
							   : editor_state.fileContent.size();

	// Find position using CHARACTER OFFSET (not visual column)
	int new_cursor =
		target_start + std::min(original_char_offset, target_end - target_start);

	// Add original cursor to multi-cursors
	editor_state.multi_cursor_indices.push_back(original_cursor);
	editor_state.multi_cursor_prefered_columns.push_back(original_char_offset);

	// Update main cursor to new position
	editor_state.cursor_index = new_cursor;
	editor_state.cursor_column_prefered = new_cursor - target_start; // Character offset
}

void EditorCursor::renderCursor()
{
	// Hide cursor when input is blocked
	if (editor_state.block_input)
	{
		return;
	}

	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const float cursor_thickness = 2.0f;
	bool rainbow_mode = gSettings.getRainbowMode();
	bool has_multiple = !editor_state.multi_cursor_indices.empty();

	// Render main cursor first
	// Remove the window focus check since we want to show cursor even with
	// completion menu if
	// (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		// Main cursor calculations
		int main_cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
		float main_cursor_x = getCursorXPosition(editor_state.text_pos,
												 editor_state.fileContent,
												 editor_state.cursor_index);

		ImVec2 main_cursor_start = editor_state.text_pos;
		main_cursor_start.x = main_cursor_x;
		main_cursor_start.y += main_cursor_line * editor_state.line_height;
		ImVec2 main_cursor_end(main_cursor_start.x,
							   main_cursor_start.y + editor_state.line_height - 1);

		// Color calculations
		float blink_alpha = (sinf(editor_state.cursor_blink_time * 4.0f) + 1.0f) * 0.5f;

		// Main cursor color (red if multiple, else normal)
		ImU32 main_color =
			has_multiple
				? ImGui::ColorConvertFloat4ToU32(
					  EditorUtils::GetRainbowColor()) // main cursor
				: (rainbow_mode
					   ? ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor())
					   : IM_COL32(255, 255, 255, (int)(blink_alpha * 255)));

		// Multi-cursor color (always rainbow or white based on setting)
		ImU32 multi_color =
			rainbow_mode ? ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor())
						 : IM_COL32(255, 255, 255, (int)(blink_alpha * 255));

		// Draw main cursor
		draw_list->AddLine(
			main_cursor_start, main_cursor_end, main_color, cursor_thickness);

		// Render multi-cursors
		for (int cursor_idx : editor_state.multi_cursor_indices)
		{
			int cursor_line = gEditor.getLineFromPos(cursor_idx);
			float cursor_x = getCursorXPosition(editor_state.text_pos,
												editor_state.fileContent,
												cursor_idx);

			ImVec2 cursor_start = editor_state.text_pos;
			cursor_start.x = cursor_x;
			cursor_start.y += cursor_line * editor_state.line_height;
			ImVec2 cursor_end(cursor_start.x,
							  cursor_start.y + editor_state.line_height - 1);

			draw_list->AddLine(cursor_start, cursor_end, multi_color, cursor_thickness);
		}
	}
}

// Cursor time management
void EditorCursor::updateBlinkTime()
{
	editor_state.cursor_blink_time += ImGui::GetIO().DeltaTime;
}

// Implementation of helper functions
void EditorCursor::calculateVisualColumn()
{
	const int TAB_WIDTH = 4; // Tab width in spaces
	int visual_column = 0;
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[current_line];

	for (int i = line_start;
		 i < editor_state.cursor_index && i < editor_state.fileContent.length();
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

	while (pos < line_end &&
		   current_visual_column < editor_state.cursor_column_prefered &&
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
	// Main cursor
	if (editor_state.cursor_index > 0)
	{
		auto it = editor_state.fileContent.begin() + editor_state.cursor_index;
		if (it != editor_state.fileContent.begin())
		{
			utf8::unchecked::prior(it);
		}
		editor_state.cursor_index = std::distance(editor_state.fileContent.begin(), it);
	}
	// Multi-cursors
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		if (editor_state.multi_cursor_indices[i] > 0)
		{
			auto it =
				editor_state.fileContent.begin() + editor_state.multi_cursor_indices[i];
			if (it != editor_state.fileContent.begin())
			{
				utf8::unchecked::prior(it);
			}
			editor_state.multi_cursor_indices[i] =
				std::distance(editor_state.fileContent.begin(), it);
		}
	}
	calculateVisualColumn();
}
void EditorCursor::cursorRight()
{
	// Main cursor
	if (editor_state.cursor_index < editor_state.fileContent.size())
	{
		auto it = editor_state.fileContent.begin() + editor_state.cursor_index;
		if (it != editor_state.fileContent.end())
		{
			utf8::unchecked::next(it);
		}
		editor_state.cursor_index = std::distance(editor_state.fileContent.begin(), it);
	}
	// Multi-cursors
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		if (editor_state.multi_cursor_indices[i] < editor_state.fileContent.size())
		{
			auto it =
				editor_state.fileContent.begin() + editor_state.multi_cursor_indices[i];
			if (it != editor_state.fileContent.end())
			{
				utf8::unchecked::next(it);
			}
			editor_state.multi_cursor_indices[i] =
				std::distance(editor_state.fileContent.begin(), it);
		}
	}
	calculateVisualColumn();
}

void EditorCursor::cursorUp()
{
	// --- Main Cursor ---
	int main_current_line_num =
		EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
										 editor_state.cursor_index);
	if (main_current_line_num > 0)
	{
		if (editor_state.cursor_column_prefered == 0 &&
			editor_state.cursor_index !=
				editor_state.editor_content_lines[main_current_line_num])
		{
			calculateVisualColumn();
		}
		int target_line_idx = main_current_line_num - 1;
		int new_line_start = editor_state.editor_content_lines[target_line_idx];
		int new_line_end = editor_state.editor_content_lines[main_current_line_num] - 1;
		findPositionFromVisualColumn(new_line_start, new_line_end);

		ImVec2 currentPos = gEditorScroll.getScrollPosition();
		gEditorScroll.setScrollPosition(
			ImVec2(currentPos.x, std::max(0.0f, currentPos.y - editor_state.line_height)));
	}

	// editor_state.cursor_column_prefered
	int original_main_cursor_index = editor_state.cursor_index;
	int original_main_cursor_pref_col = editor_state.cursor_column_prefered;

	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		int current_multi_idx = editor_state.multi_cursor_indices[i];
		int multi_cursor_pref_col =
			editor_state.multi_cursor_prefered_columns[i]; // Use this multi-cursor's
														   // own preferred column

		int multi_current_line_num =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 current_multi_idx);
		if (multi_current_line_num > 0)
		{
			if (multi_cursor_pref_col == 0 &&
				current_multi_idx !=
					editor_state.editor_content_lines[multi_current_line_num])
			{
				// Using simple char offset for this "on-the-fly" calculation to
				// match mouse click
				multi_cursor_pref_col =
					current_multi_idx -
					editor_state.editor_content_lines[multi_current_line_num];
				editor_state.multi_cursor_prefered_columns[i] =
					multi_cursor_pref_col; // Store it
			}

			int target_line_idx = multi_current_line_num - 1;
			int new_line_start = editor_state.editor_content_lines[target_line_idx];
			int new_line_end =
				editor_state.editor_content_lines[multi_current_line_num] - 1;

			// Temporarily set global state for findPositionFromVisualColumn to
			// work for this multi-cursor
			editor_state.cursor_index =
				current_multi_idx; // Context for where the cursor *is*
			editor_state.cursor_column_prefered =
				multi_cursor_pref_col; // The column it *wants* to be at

			findPositionFromVisualColumn(
				new_line_start,
				new_line_end); // This will update editor_state.cursor_index
			editor_state.multi_cursor_indices[i] =
				editor_state.cursor_index; // Store the new position
		}
	}
	// Restore main cursor's actual state
	editor_state.cursor_index = original_main_cursor_index;
	editor_state.cursor_column_prefered = original_main_cursor_pref_col;

	// Snap to UTF-8 boundary
	editor_state.cursor_index =
		snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		editor_state.multi_cursor_indices[i] =
			snapToUtf8CharBoundary(editor_state.fileContent,
								   editor_state.multi_cursor_indices[i]);

	// Update undo manager's pendingFinalCursor for first edit logic
	if (gFileExplorer.currentUndoManager)
	{
		gFileExplorer.currentUndoManager->updatePendingFinalCursor(
			editor_state.cursor_index);
	}
}

void EditorCursor::cursorDown()
{
	// --- Main Cursor ---
	int main_current_line_num =
		EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
										 editor_state.cursor_index);
	if (main_current_line_num < editor_state.editor_content_lines.size() - 1)
	{
		if (editor_state.cursor_column_prefered == 0 &&
			editor_state.cursor_index !=
				editor_state.editor_content_lines[main_current_line_num])
		{
			calculateVisualColumn();
		}
		int target_line_idx = main_current_line_num + 1;
		int new_line_start = editor_state.editor_content_lines[target_line_idx];
		int new_line_end =
			(target_line_idx + 1 < editor_state.editor_content_lines.size())
				? editor_state.editor_content_lines[target_line_idx + 1] - 1
				: editor_state.fileContent.size();
		findPositionFromVisualColumn(new_line_start, new_line_end);

		ImVec2 currentPos = gEditorScroll.getScrollPosition();
		gEditorScroll.setScrollPosition(
			ImVec2(currentPos.x, currentPos.y + editor_state.line_height));
	}

	// --- Multi-cursors ---
	int original_main_cursor_index = editor_state.cursor_index;
	int original_main_cursor_pref_col = editor_state.cursor_column_prefered;

	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		int current_multi_idx = editor_state.multi_cursor_indices[i];
		int multi_cursor_pref_col = editor_state.multi_cursor_prefered_columns[i];

		int multi_current_line_num =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 current_multi_idx);
		if (multi_current_line_num < editor_state.editor_content_lines.size() - 1)
		{
			if (multi_cursor_pref_col == 0 &&
				current_multi_idx !=
					editor_state.editor_content_lines[multi_current_line_num])
			{
				multi_cursor_pref_col =
					current_multi_idx -
					editor_state.editor_content_lines[multi_current_line_num];
				editor_state.multi_cursor_prefered_columns[i] = multi_cursor_pref_col;
			}

			int target_line_idx = multi_current_line_num + 1;
			int new_line_start = editor_state.editor_content_lines[target_line_idx];
			int new_line_end =
				(target_line_idx + 1 < editor_state.editor_content_lines.size())
					? editor_state.editor_content_lines[target_line_idx + 1] - 1
					: editor_state.fileContent.size();

			editor_state.cursor_index = current_multi_idx;
			editor_state.cursor_column_prefered = multi_cursor_pref_col;

			findPositionFromVisualColumn(new_line_start, new_line_end);
			editor_state.multi_cursor_indices[i] = editor_state.cursor_index;
		}
	}
	editor_state.cursor_index = original_main_cursor_index;
	editor_state.cursor_column_prefered = original_main_cursor_pref_col;

	// Snap to UTF-8 boundary
	editor_state.cursor_index =
		snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		editor_state.multi_cursor_indices[i] =
			snapToUtf8CharBoundary(editor_state.fileContent,
								   editor_state.multi_cursor_indices[i]);

	// Update undo manager's pendingFinalCursor for first edit logic
	if (gFileExplorer.currentUndoManager)
	{
		gFileExplorer.currentUndoManager->updatePendingFinalCursor(
			editor_state.cursor_index);
	}
}

void EditorCursor::moveCursorVertically(std::string &text, int line_delta)
{
	int main_current_line_num =
		EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
										 editor_state.cursor_index);
	int main_target_line_num =
		std::max(0,
				 std::min(static_cast<int>(editor_state.editor_content_lines.size()) - 1,
						  main_current_line_num + line_delta));

	if (editor_state.cursor_column_prefered == 0 &&
		editor_state.cursor_index !=
			editor_state.editor_content_lines[main_current_line_num])
	{
		calculateVisualColumn(); // Sets editor_state.cursor_column_prefered for
								 // main cursor
	}

	if (main_target_line_num >= 0 &&
		main_target_line_num < editor_state.editor_content_lines.size())
	{
		int new_line_start = editor_state.editor_content_lines[main_target_line_num];
		int new_line_end =
			(main_target_line_num + 1 < editor_state.editor_content_lines.size())
				? editor_state.editor_content_lines[main_target_line_num + 1] - 1
				: text.size(); // Use text.size() for the very last line

		findPositionFromVisualColumn(new_line_start, new_line_end);
	}
	int original_main_cursor_index = editor_state.cursor_index;
	int original_main_cursor_pref_col = editor_state.cursor_column_prefered;

	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		int current_multi_idx = editor_state.multi_cursor_indices[i];
		int multi_cursor_pref_col = editor_state.multi_cursor_prefered_columns[i];

		int multi_current_line_num =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 current_multi_idx);
		int multi_target_line_num = std::max(
			0,
			std::min(static_cast<int>(editor_state.editor_content_lines.size()) - 1,
					 multi_current_line_num + line_delta));

		// If this multi-cursor's preferred column is 0 and it's not at the
		// start of its line, calculate it.
		if (multi_cursor_pref_col == 0 &&
			current_multi_idx != editor_state.editor_content_lines[multi_current_line_num])
		{
			// Simple character offset based preferred column, consistent with
			// other multi-cursor logic
			multi_cursor_pref_col =
				current_multi_idx -
				editor_state.editor_content_lines[multi_current_line_num];
			editor_state.multi_cursor_prefered_columns[i] =
				multi_cursor_pref_col; // Store it
		}

		if (multi_target_line_num >= 0 &&
			multi_target_line_num < editor_state.editor_content_lines.size())
		{
			int new_line_start = editor_state.editor_content_lines[multi_target_line_num];
			int new_line_end =
				(multi_target_line_num + 1 < editor_state.editor_content_lines.size())
					? editor_state.editor_content_lines[multi_target_line_num + 1] - 1
					: text.size();

			// Temporarily set global state for findPositionFromVisualColumn
			editor_state.cursor_index =
				current_multi_idx; // Context for where the cursor *is*
			editor_state.cursor_column_prefered =
				multi_cursor_pref_col; // The column it *wants* to be at

			findPositionFromVisualColumn(
				new_line_start,
				new_line_end); // Updates editor_state.cursor_index
			editor_state.multi_cursor_indices[i] =
				editor_state.cursor_index; // Store the new position for this
										   // multi-cursor
		}
	}

	// Restore main cursor's actual state
	editor_state.cursor_index = original_main_cursor_index;
	editor_state.cursor_column_prefered = original_main_cursor_pref_col;
}

void EditorCursor::moveWordForward(const std::string &text)
{
	const size_t len = text.length();

	// --- Main Cursor ---
	size_t current_main_idx = editor_state.cursor_index;
	size_t next_main_idx = current_main_idx; // Default to no change

	if (current_main_idx < len) // Only process if not at or past the end
	{
		size_t pos = current_main_idx;
		// Skip non-word characters to find the start of the current/next word
		while (pos < len && !isWordChar(text[pos]))
		{
			++pos;
		}
		// Skip word characters to find the end of the current word
		while (pos < len && isWordChar(text[pos]))
		{
			++pos;
		}
		if (pos > 0)
		{ // Ensure pos didn't somehow become 0 if current_main_idx was 0 and
		  // text was empty/all non-word
			next_main_idx = pos - 1;
		} else if (len > 0)
		{ // current_main_idx was 0, pos is 0, but text has content (e.g. "a")
			next_main_idx = 0; // Stay at 0 if it's a single char word at start,
							   // or all non-word
		} else
		{ // len is 0
			next_main_idx = 0;
		}
	}

	if (editor_state.cursor_index != next_main_idx)
	{ // If position actually changed
		editor_state.cursor_index = next_main_idx;
		int current_line =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 editor_state.cursor_index);
		editor_state.cursor_column_prefered =
			editor_state.cursor_index - editor_state.editor_content_lines[current_line];
	}

	// --- Multi-cursors ---
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		size_t current_multi_idx = editor_state.multi_cursor_indices[i];
		size_t next_multi_idx = current_multi_idx; // Default to no change

		if (current_multi_idx < len) // Only process if not at or past the end
		{
			size_t pos = current_multi_idx;
			while (pos < len && !isWordChar(text[pos]))
			{
				++pos;
			}
			while (pos < len && isWordChar(text[pos]))
			{
				++pos;
			}
			if (pos > 0)
			{
				next_multi_idx = pos - 1;
			} else if (len > 0)
			{
				next_multi_idx = 0;
			} else
			{
				next_multi_idx = 0;
			}
		}

		if (editor_state.multi_cursor_indices[i] != next_multi_idx)
		{ // If position actually changed
			editor_state.multi_cursor_indices[i] = next_multi_idx;
			int mc_line =
				EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												 editor_state.multi_cursor_indices[i]);
			editor_state.multi_cursor_prefered_columns[i] =
				editor_state.multi_cursor_indices[i] -
				editor_state.editor_content_lines[mc_line];
		}
	}
}

void EditorCursor::moveWordBackward(const std::string &text)
{
	// --- Main Cursor ---
	size_t current_main_idx = editor_state.cursor_index;
	size_t next_main_idx = current_main_idx; // Default to no change

	if (current_main_idx > 0) // Only process if not at the beginning
	{
		size_t pos = current_main_idx;
		// Skip non-word characters immediately to the left of the cursor
		while (pos > 0 && !isWordChar(text[pos - 1]))
		{
			--pos;
		}
		// Skip word characters to find the start of the current/previous word
		while (pos > 0 && isWordChar(text[pos - 1]))
		{
			--pos;
		}
		next_main_idx = pos + 1;
		if (next_main_idx > current_main_idx && pos == current_main_idx)
		{ // No actual movement, but pos+1 overshot
			next_main_idx = current_main_idx;
		}
		next_main_idx = pos + 1; // Adhering to original.
	}

	if (editor_state.cursor_index != next_main_idx)
	{ // If position actually changed
		editor_state.cursor_index = next_main_idx;
		int current_line =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 editor_state.cursor_index);
		editor_state.cursor_column_prefered =
			editor_state.cursor_index - editor_state.editor_content_lines[current_line];
		// Or: calculateVisualColumn();
	}

	// --- Multi-cursors ---
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
	{
		size_t current_multi_idx = editor_state.multi_cursor_indices[i];
		size_t next_multi_idx = current_multi_idx; // Default to no change

		if (current_multi_idx > 0) // Only process if not at the beginning
		{
			size_t pos = current_multi_idx;
			while (pos > 0 && !isWordChar(text[pos - 1]))
			{
				--pos;
			}
			while (pos > 0 && isWordChar(text[pos - 1]))
			{
				--pos;
			}
			next_multi_idx = pos + 1; // Adhering to original logic for pos + 1
		}

		if (editor_state.multi_cursor_indices[i] != next_multi_idx)
		{ // If position actually changed
			editor_state.multi_cursor_indices[i] = next_multi_idx;
			int mc_line =
				EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												 editor_state.multi_cursor_indices[i]);
			editor_state.multi_cursor_prefered_columns[i] =
				editor_state.multi_cursor_indices[i] -
				editor_state.editor_content_lines[mc_line];
		}
	}
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
	for (int i = 0; i < cursor_pos;)
	{
		if (text[i] == '\n')
		{
			x = text_pos.x;
			i++;
		} else
		{
			// Skip continuation bytes of multi-byte characters (same logic as rendering)
			if ((text[i] & 0xC0) == 0x80)
			{
				i++;
				continue;
			}

			// Handle UTF-8 characters properly (same logic as renderCharacterAndSelection)
			const char *char_start = &text[i];
			const char *char_end = (i + 1 < text.size()) ? &text[i + 1] : nullptr;

			// For multi-byte characters, find the end
			if (char_end && (*char_start & 0x80))
			{
				while (char_end < &text[text.size()] && (*char_end & 0xC0) == 0x80)
				{
					char_end++;
				}
			}

			x += ImGui::CalcTextSize(char_start, char_end).x;

			// Advance by the character length (same logic as rendering loop)
			if ((*char_start & 0x80) == 0)
			{
				i++; // Single byte character
			} else
			{
				// Multi-byte character, find the end
				while (i < text.size() && (text[i] & 0xC0) == 0x80)
				{
					i++;
				}
				i++; // Move past the last byte of the character
			}
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
		// Ensure multi_selections is correctly sized.
		if (editor_state.multi_selections.size() !=
			editor_state.multi_cursor_indices.size())
		{
			editor_state.multi_selections.resize(editor_state.multi_cursor_indices.size());
		}
		for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		{
			editor_state.multi_selections[i].start_index =
				editor_state.multi_cursor_indices[i];
			// Initialize end_index to the same as start_index initially
			// editor_state.multi_selections[i].end_index =
			// editor_state.multi_cursor_indices[i];
		}
	}

	// Clear selection only if a movement key is pressed without Shift
	if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
						   ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
						   ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
						   ImGui::IsKeyPressed(ImGuiKey_RightArrow)))
	{
		editor_state.selection_active = false;
		editor_state.selection_start = editor_state.selection_end =
			editor_state.cursor_index;
		editor_state.multi_selections.clear();
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
		for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		{
			editor_state.multi_selections[i].end_index =
				editor_state.multi_cursor_indices[i];
		}
	}

	// Use the EditorScroll class to handle scroll adjustments
	gEditorScroll.handleCursorMovementScroll();

	// At the end of all movement, snap to UTF-8 boundary
	editor_state.cursor_index =
		snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		editor_state.multi_cursor_indices[i] =
			snapToUtf8CharBoundary(editor_state.fileContent,
								   editor_state.multi_cursor_indices[i]);
}
void EditorCursor::swapLines(int direction)
{
	// Handle line movement down
	if (direction == 1)
	{
		const int current_line =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
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
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 insert_pos);
		const size_t new_line_start = editor_state.editor_content_lines[new_line];
		const size_t new_line_length =
			(new_line + 1 < editor_state.editor_content_lines.size())
				? editor_state.editor_content_lines[new_line + 1] - new_line_start
				: editor_state.fileContent.size() - new_line_start;

		editor_state.cursor_index =
			new_line_start + std::min(cursor_offset, new_line_length);
	}
	// Handle line movement up
	else if (direction == -1)
	{
		const int current_line =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 editor_state.cursor_index);
		const int target_line = current_line - 1;

		// Boundary check: Can't move up from first line
		if (target_line < 0)
			return;

		// Get current line boundaries and content
		const size_t line_start = editor_state.editor_content_lines[current_line];
		const size_t line_end =
			(current_line + 1 < editor_state.editor_content_lines.size())
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

		editor_state.cursor_index =
			new_line_start + std::min(cursor_offset, new_line_length);
	}

	// Common state updates
	editor_state.selection_active = false;
	editor_state.text_changed = true;
	editor_state.ensure_cursor_visible = {true, true};
}

void EditorCursor::processCursorJump(std::string &text,
									 CursorVisibility &ensure_cursor_visible)
{
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		// --- Main Cursor ---
		int main_current_line_num =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 editor_state.cursor_index);
		if (main_current_line_num >= 0 &&
			main_current_line_num <
				editor_state.editor_content_lines.size()) // Basic bounds check
		{
			// Your original logic for main cursor:
			editor_state.cursor_index =
				editor_state.editor_content_lines[main_current_line_num] + 1;
			// Ensure it doesn't go past the start of the next line (if not the
			// last line) or past the end of the text (if it is the last line)
			size_t next_line_start_main =
				(main_current_line_num + 1 < editor_state.editor_content_lines.size())
					? editor_state.editor_content_lines[main_current_line_num + 1]
					: text.length();
			if (editor_state.cursor_index >= next_line_start_main &&
				next_line_start_main >
					editor_state.editor_content_lines[main_current_line_num])
			{ // if next_line_start is not same as current (empty line)
				editor_state.cursor_index =
					next_line_start_main - 1; // Go to end of current line if overshot
			}
			if (editor_state.cursor_index > text.length())
			{ // General safety
				editor_state.cursor_index = text.length();
			}

			// Update preferred column for main cursor
			editor_state.cursor_column_prefered =
				editor_state.cursor_index -
				editor_state.editor_content_lines[main_current_line_num];
		}
		ensure_cursor_visible.horizontal = true;

		// --- Multi-cursors ---
		for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		{
			int mc_current_line_num =
				EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												 editor_state.multi_cursor_indices[i]);
			if (mc_current_line_num >= 0 &&
				mc_current_line_num <
					editor_state.editor_content_lines.size()) // Basic bounds check
			{
				// Apply your original logic to each multi-cursor
				editor_state.multi_cursor_indices[i] =
					editor_state.editor_content_lines[mc_current_line_num] + 1;

				size_t next_line_start_mc =
					(mc_current_line_num + 1 < editor_state.editor_content_lines.size())
						? editor_state.editor_content_lines[mc_current_line_num + 1]
						: text.length();
				if (editor_state.multi_cursor_indices[i] >= next_line_start_mc &&
					next_line_start_mc >
						editor_state.editor_content_lines[mc_current_line_num])
				{
					editor_state.multi_cursor_indices[i] = next_line_start_mc - 1;
				}
				if (editor_state.multi_cursor_indices[i] > text.length())
				{
					editor_state.multi_cursor_indices[i] = text.length();
				}

				// Update preferred column for this multi-cursor
				editor_state.multi_cursor_prefered_columns[i] =
					editor_state.multi_cursor_indices[i] -
					editor_state.editor_content_lines[mc_current_line_num];
			}
		}
	} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		// --- Main Cursor ---
		int main_current_line_num =
			EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
											 editor_state.cursor_index);
		if (main_current_line_num >= 0 &&
			main_current_line_num <
				editor_state.editor_content_lines.size()) // Basic bounds check
		{
			// Your original logic for main cursor:
			int main_next_line_num = main_current_line_num + 1;
			if (main_next_line_num < editor_state.editor_content_lines.size())
			{
				editor_state.cursor_index =
					editor_state.editor_content_lines[main_next_line_num] - 2;
			} else
			{
				editor_state.cursor_index = text.size();
			}
			// Ensure cursor index is not less than the start of its current line
			if (editor_state.cursor_index <
				editor_state.editor_content_lines[main_current_line_num])
			{
				editor_state.cursor_index =
					editor_state.editor_content_lines[main_current_line_num];
			}

			// Update preferred column for main cursor
			editor_state.cursor_column_prefered =
				editor_state.cursor_index -
				editor_state.editor_content_lines[main_current_line_num];
		}
		ensure_cursor_visible.horizontal = true;

		// --- Multi-cursors ---
		for (size_t i = 0; i < editor_state.multi_cursor_indices.size(); ++i)
		{
			int mc_current_line_num =
				EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												 editor_state.multi_cursor_indices[i]);
			if (mc_current_line_num >= 0 &&
				mc_current_line_num <
					editor_state.editor_content_lines.size()) // Basic bounds check
			{
				// Apply your original logic to each multi-cursor
				int mc_next_line_num = mc_current_line_num + 1;
				if (mc_next_line_num < editor_state.editor_content_lines.size())
				{
					editor_state.multi_cursor_indices[i] =
						editor_state.editor_content_lines[mc_next_line_num] - 2;
				} else
				{
					editor_state.multi_cursor_indices[i] = text.size();
				}
				if (editor_state.multi_cursor_indices[i] <
					editor_state.editor_content_lines[mc_current_line_num])
				{
					editor_state.multi_cursor_indices[i] =
						editor_state.editor_content_lines[mc_current_line_num];
				}

				// Update preferred column for this multi-cursor
				editor_state.multi_cursor_prefered_columns[i] =
					editor_state.multi_cursor_indices[i] -
					editor_state.editor_content_lines[mc_current_line_num];
			}
		}
	} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		// This part remains unchanged as per your instruction,
		// assuming moveCursorVertically handles all cursors correctly.
		moveCursorVertically(text, -5);
		ensure_cursor_visible.vertical = true;
		ensure_cursor_visible.horizontal = true;
	} else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		// This part remains unchanged as per your instruction,
		// assuming moveCursorVertically handles all cursors correctly.
		moveCursorVertically(text, 5);
		ensure_cursor_visible.vertical = true;
		ensure_cursor_visible.horizontal = true;
	}
}
void EditorCursor::processWordMovement(std::string &text,
									   CursorVisibility &ensure_cursor_visible)
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
int EditorCursor::CalculateVisualColumnForPosition(int position,
												   const std::string &content,
												   const std::vector<int> &content_lines)
{
	const int TAB_WIDTH = 4;
	int visual_column = 0;
	int current_line = EditorUtils::GetLineFromPosition(content_lines, position);
	int line_start = content_lines[current_line];

	for (int i = line_start; i < position && i < content.length(); i++)
	{
		if (content[i] == '\t')
		{
			visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
		} else
		{
			visual_column++;
		}
	}
	return visual_column;
}
