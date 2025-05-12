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

void EditorSelection::selectAllText(const std::string &text)
{
	const size_t MAX_SELECTION_SIZE = 100000; // Limit for very large files
	editor_state.selection_active = true;
	editor_state.selection_start = 0;
	editor_state.cursor_index = std::min(text.size(), MAX_SELECTION_SIZE);
	editor_state.selection_end = editor_state.cursor_index;
	editor_state.full_text_selected = true;
}
