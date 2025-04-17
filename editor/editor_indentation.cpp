#include "../files/files.h"

#include "editor.h"
#include "editor_highlight.h"
#include "editor_indentation.h"

#include <algorithm>
#include <iostream>

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
	while (lineEnd < editor_state.fileContent.length() && editor_state.fileContent[lineEnd] != '\n')
	{
		lineEnd++;
	}
	return lineEnd;
}

void EditorIndentation::handleTabKey()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		if (editor_state.selection_active)
		{
			handleMultiLineIndentation();
		} else
		{
			handleSingleLineIndentation();
		}

		finishIndentationChange();
	}
}

void EditorIndentation::handleMultiLineIndentation()
{
	int start = getSelectionStart();
	int end = getSelectionEnd();

	// Find the start of the first line
	int firstLineStart = findLineStart(start);

	// Find the end of the last line
	int lastLineEnd = findLineEnd(end);

	int tabsInserted = 0;
	int lineStart = firstLineStart;

	// Insert tabs at the beginning of each selected line
	while (lineStart < lastLineEnd)
	{
		// Insert tab at the beginning of the line
		editor_state.fileContent.insert(lineStart, 1, '\t');
		editor_state.fileColors.insert(editor_state.fileColors.begin() + lineStart,
									   1,
									   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		tabsInserted++;

		// Move to the next line
		lineStart = editor_state.fileContent.find('\n', lineStart) + 1;
		if (lineStart == 0)
			break; // If we've reached the end of the text
	}

	// Adjust cursor and selection positions
	editor_state.cursor_index += (editor_state.cursor_index >= start) ? tabsInserted : 0;
	editor_state.selection_start += (editor_state.selection_start > start) ? tabsInserted : 0;
	editor_state.selection_end += tabsInserted;
}

void EditorIndentation::handleSingleLineIndentation()
{
	// Insert a single tab character at cursor position
	editor_state.fileContent.insert(editor_state.cursor_index, 1, '\t');
	editor_state.fileColors.insert(editor_state.fileColors.begin() + editor_state.cursor_index,
								   1,
								   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	editor_state.cursor_index++;
	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
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

		processLineIndentRemoval(newText, lineStart, lineEnd, lastLineEnd, totalSpacesRemoved);
		lineStart = lineEnd + 1;
	}

	// Copy text after the affected lines
	newText.append(editor_state.fileContent.substr(lastLineEnd));

	// Update text and adjust cursor and selection
	updateStateAfterIndentRemoval(newText, firstLineStart, lastLineEnd, totalSpacesRemoved);

	// Update colors and trigger highlighting
	updateColorsAfterIndentRemoval(firstLineStart, lastLineEnd, totalSpacesRemoved);
}

void EditorIndentation::processLineIndentRemoval(std::string &newText,
												 size_t lineStart,
												 size_t lineEnd,
												 size_t lastLineEnd,
												 int &totalSpacesRemoved)
{
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

	// Append the line without leading indentation
	newText.append(editor_state.fileContent.substr(lineStart + spacesToRemove,
												   lineEnd - lineStart - spacesToRemove));

	// Add newline if not the last line
	if (lineEnd < lastLineEnd)
		newText.push_back('\n');
}

void EditorIndentation::updateStateAfterIndentRemoval(std::string &newText,
													  int firstLineStart,
													  int lastLineEnd,
													  int totalSpacesRemoved)
{
	// Update text
	editor_state.fileContent = std::move(newText);

	// Adjust cursor position
	editor_state.cursor_index =
		std::max(editor_state.cursor_index - totalSpacesRemoved, firstLineStart);

	// Adjust selection if one exists
	if (editor_state.selection_active)
	{
		editor_state.selection_start =
			std::max(editor_state.selection_start - totalSpacesRemoved, firstLineStart);
		editor_state.selection_end =
			std::max(editor_state.selection_end - totalSpacesRemoved, firstLineStart);
	} else
	{
		editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	}
}

void EditorIndentation::updateColorsAfterIndentRemoval(int firstLineStart,
													   int lastLineEnd,
													   int totalSpacesRemoved)
{
	// Update colors vector
	auto &colors = editor_state.fileColors;
	colors.erase(colors.begin() + firstLineStart, colors.begin() + lastLineEnd);
	colors.insert(colors.begin() + firstLineStart,
				  lastLineEnd - firstLineStart - totalSpacesRemoved,
				  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Insert default color

	// Update line starts
	gEditor.updateLineStarts();

	// Mark text as changed
	gFileExplorer._unsavedChanges = true;

	// Trigger syntax highlighting for the affected area
	gEditorHighlight.highlightContent();
}
