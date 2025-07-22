#include "../files/files.h"

#include "editor.h"
#include "editor_highlight.h"
#include "editor_indentation.h"

#include "editor_tree_sitter.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
// Global instance
EditorIndentation gEditorIndentation;

int EditorIndentation::getSelectionStart()
{
	return std::min(editor_state.selection_start, editor_state.selection_end);
}

int EditorIndentation::getSelectionEnd()
{
	return std::max(editor_state.selection_start, editor_state.selection_end);
}

int EditorIndentation::findLineStart(int position)
{
	int lineStart = position;
	while (lineStart > 0 && editor_state.fileContent[lineStart - 1] != '\n')
	{
		lineStart--;
	}
	return lineStart;
}

int EditorIndentation::findLineEnd(int position)
{
	int lineEnd = position;
	while (lineEnd < editor_state.fileContent.length() &&
		   editor_state.fileContent[lineEnd] != '\n')
	{
		lineEnd++;
	}
	return lineEnd;
}

void EditorIndentation::handleTabKey()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		if (editor_state.selection_start != editor_state.selection_end)
		{
			handleMultiLineIndentation();
		} else
		{
			editor_state.selection_active = false;
			handleSingleLineIndentation();
		}

		finishIndentationChange();
	}
}

void EditorIndentation::handleMultiLineIndentation()
{
	// Determine the range to process
	int start = getSelectionStart();
	int end = getSelectionEnd();

	// Find the start of the first line
	int firstLineStart = findLineStart(start);

	// Find the end of the last line
	int lastLineEnd = findLineEnd(end);

	int totalTabsInserted = 0;
	std::string newText;
	newText.reserve(editor_state.fileContent.length());

	// Copy text before the affected lines
	newText.append(editor_state.fileContent.substr(0, firstLineStart));

	// Process each line
	size_t lineStart = firstLineStart;
	while (lineStart <= lastLineEnd)
	{
		size_t lineEnd = editor_state.fileContent.find('\n', lineStart);
		if (lineEnd == std::string::npos || lineEnd > lastLineEnd)
			lineEnd = lastLineEnd;

		// Add tab at the beginning of the line
		newText.push_back('\t');
		totalTabsInserted++;

		// Copy the rest of the line
		newText.append(editor_state.fileContent.substr(lineStart, lineEnd - lineStart));

		if (lineEnd < lastLineEnd)
			newText.push_back('\n');

		lineStart = lineEnd + 1;
	}

	// Copy text after the affected lines
	newText.append(editor_state.fileContent.substr(lastLineEnd));

	// Update text
	editor_state.fileContent = std::move(newText);

	// Update selection and cursor positions
	if (editor_state.selection_start < editor_state.selection_end)
	{
		editor_state.selection_start += 1;
		editor_state.selection_end += totalTabsInserted;
		editor_state.cursor_index += totalTabsInserted;
	} else
	{
		editor_state.selection_start += totalTabsInserted;
		editor_state.selection_end += 1;
		editor_state.cursor_index += 1;
	}

	// Update colors vector
	auto &colors = editor_state.fileColors;
	colors.erase(colors.begin() + firstLineStart, colors.begin() + lastLineEnd);

	// Get the proper default text color from the theme
	TreeSitter::updateThemeColors();
	ImVec4 defaultColor = TreeSitter::cachedColors.text;

	colors.insert(colors.begin() + firstLineStart,
				  lastLineEnd - firstLineStart + totalTabsInserted,
				  defaultColor);
}

void EditorIndentation::handleSingleLineIndentation()
{
	std::set<int> unique_cursor_positions;
	editor_state.selection_active = false;
	editor_state.selection_start = editor_state.cursor_index;
	editor_state.selection_end = editor_state.cursor_index;
	unique_cursor_positions.insert(editor_state.cursor_index); // Add primary
	for (int mc_idx : editor_state.multi_cursor_indices)	   // Add multi-cursors
	{
		unique_cursor_positions.insert(mc_idx);
	}

	const char TAB_CHAR = '\t';
	const size_t INSERT_LEN = 1;

	int cumulative_offset = 0;
	std::vector<int> new_final_cursor_positions;

	for (int original_pos : unique_cursor_positions)
	{
		int actual_insert_pos = original_pos + cumulative_offset;

		actual_insert_pos =
			std::max(0,
					 std::min(actual_insert_pos,
							  static_cast<int>(editor_state.fileContent.length())));

		editor_state.fileContent.insert(actual_insert_pos, 1, TAB_CHAR);

		// Get the proper default text color from the theme
		TreeSitter::updateThemeColors();
		ImVec4 defaultColor = TreeSitter::cachedColors.text;

		if (static_cast<size_t>(actual_insert_pos) <= editor_state.fileColors.size())
		{
			editor_state.fileColors.insert(editor_state.fileColors.begin() +
											   actual_insert_pos,
										   INSERT_LEN,
										   defaultColor);
		} else
		{
			editor_state.fileColors.push_back(defaultColor);
		}

		new_final_cursor_positions.push_back(actual_insert_pos + INSERT_LEN);

		cumulative_offset += INSERT_LEN;
	}

	if (!new_final_cursor_positions.empty())
	{
		// Assign the first new position to the primary cursor, the rest to
		// multi-cursors
		editor_state.cursor_index = new_final_cursor_positions[0];
		editor_state.multi_cursor_indices.assign(new_final_cursor_positions.begin() + 1,
												 new_final_cursor_positions.end());
	}

	editor_state.multi_selections.clear();
	editor_state.selection_active = false;
	editor_state.selection_start = editor_state.cursor_index;
	editor_state.selection_end = editor_state.cursor_index;
	// Reset preferred columns, as tab insertion explicitly changes horizontal
	// (visual) position.
	editor_state.cursor_column_prefered = 0;
	editor_state.multi_cursor_prefered_columns.assign(
		editor_state.multi_cursor_indices.size(), 0);
}

void EditorIndentation::finishIndentationChange()
{
	// Mark text as changed and update
	editor_state.text_changed = true;
	gEditor.updateLineStarts();
	gFileExplorer._unsavedChanges = true;

	// Trigger syntax highlighting for the affected area
	gEditorHighlight.highlightContent();
}

bool EditorIndentation::processIndentRemoval()
{
	// If Shift+Tab is pressed, remove indentation and exit early.
	if (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
	{
		removeIndentation();
		editor_state.text_changed = true;
		editor_state.ensure_cursor_visible.horizontal = true;
		editor_state.ensure_cursor_visible.vertical = true;
		ImGui::SetKeyboardFocusHere(-1); // Prevent default tab behavior
		return true;
	}
	return false;
}

void EditorIndentation::removeIndentation()
{
	// Determine the range to process
	int start, end;
	if (editor_state.selection_active)
	{
		start = getSelectionStart();
		end = getSelectionEnd();
	} else
	{
		// If no selection, work on the current line
		start = end = editor_state.cursor_index;
	}

	// Find the start of the first line
	int firstLineStart = findLineStart(start);

	// Find the end of the last line
	int lastLineEnd = findLineEnd(end);

	int totalSpacesRemoved = 0;
	std::string newText;
	newText.reserve(editor_state.fileContent.length());

	// Copy text before the affected lines
	newText.append(editor_state.fileContent.substr(0, firstLineStart));

	// Process each line
	size_t lineStart = firstLineStart;
	while (lineStart <= lastLineEnd)
	{
		size_t lineEnd = editor_state.fileContent.find('\n', lineStart);
		if (lineEnd == std::string::npos || lineEnd > lastLineEnd)
			lineEnd = lastLineEnd;

		// Check for spaces or tab at the beginning of the line
		int spacesToRemove = 0;
		if (lineStart + 4 <= editor_state.fileContent.length() &&
			editor_state.fileContent.substr(lineStart, 4) == "    ")
		{
			spacesToRemove = 4;
		} else if (lineStart < editor_state.fileContent.length() &&
				   editor_state.fileContent[lineStart] == '\t')
		{
			spacesToRemove = 1;
		}

		newText.append(editor_state.fileContent.substr(
			lineStart + spacesToRemove, lineEnd - lineStart - spacesToRemove));

		if (lineEnd < lastLineEnd)
			newText.push_back('\n');

		totalSpacesRemoved += spacesToRemove;
		lineStart = lineEnd + 1;
	}

	// Copy text after the affected lines
	newText.append(editor_state.fileContent.substr(lastLineEnd));

	// Update text
	editor_state.fileContent = std::move(newText);
	if (totalSpacesRemoved > 0)
	{
		if (editor_state.selection_end > editor_state.selection_start)
		{
			editor_state.selection_start -= 1;
			editor_state.selection_end -= totalSpacesRemoved;
			editor_state.cursor_index -= totalSpacesRemoved;

		} else
		{
			editor_state.selection_start -= totalSpacesRemoved;
			editor_state.selection_end -= 1;
			editor_state.cursor_index -= 1;
			std::cout << "moved cursor back 1" << std::endl;
		}
	}

	// Update colors vector
	auto &colors = editor_state.fileColors;
	colors.erase(colors.begin() + firstLineStart, colors.begin() + lastLineEnd);

	// Get the proper default text color from the theme
	TreeSitter::updateThemeColors();
	ImVec4 defaultColor = TreeSitter::cachedColors.text;

	colors.insert(colors.begin() + firstLineStart,
				  lastLineEnd - firstLineStart - totalSpacesRemoved,
				  defaultColor);

	// Update line starts
	gEditor.updateLineStarts();

	// Mark text as changed
	gFileExplorer._unsavedChanges = true;

	// Trigger syntax highlighting for the affected area
	gEditorHighlight.highlightContent();
}
