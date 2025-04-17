/*
	File: editor_selection.cpp
	Description: Handles text selection operations in the editor.
*/

#include "editor_selection.h"
#include "editor.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorSelection gEditorSelection;

void EditorSelection::startSelection()
{
	editor_state.selection_active = true;
	editor_state.selection_start = editor_state.cursor_index;
	editor_state.selection_end = editor_state.cursor_index;
}

void EditorSelection::updateSelection()
{
	if (editor_state.selection_active)
	{
		editor_state.selection_end = editor_state.cursor_index;
	}
}

void EditorSelection::endSelection() { editor_state.selection_active = false; }

int EditorSelection::getSelectionStart()
{
	return std::min(editor_state.selection_start, editor_state.selection_end);
}

int EditorSelection::getSelectionEnd()
{
	return std::max(editor_state.selection_start, editor_state.selection_end);
}

void EditorSelection::selectAllText(const std::string &text)
{
	const size_t MAX_SELECTION_SIZE = 100000; // Limit for very large files
	editor_state.selection_active = true;
	editor_state.selection_start = 0;
	editor_state.cursor_index = std::min(text.size(), MAX_SELECTION_SIZE);
	editor_state.selection_end = editor_state.cursor_index;
	editor_state.full_text_selected = true;
}

bool EditorSelection::hasSelection()
{
	return editor_state.selection_active &&
		   (editor_state.selection_start != editor_state.selection_end);
}

int EditorSelection::getSelectionLength() { return getSelectionEnd() - getSelectionStart(); }