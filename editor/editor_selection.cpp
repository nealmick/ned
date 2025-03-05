/*
    File: editor_selection.cpp
    Description: Handles text selection operations in the editor.
*/

#include "editor_selection.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorSelection gEditorSelection;

void EditorSelection::startSelection(EditorState &state)
{
    state.selection_active = true;
    state.selection_start = state.cursor_column;
    state.selection_end = state.cursor_column;
}

void EditorSelection::updateSelection(EditorState &state)
{
    if (state.selection_active) {
        state.selection_end = state.cursor_column;
    }
}

void EditorSelection::endSelection(EditorState &state) { state.selection_active = false; }

int EditorSelection::getSelectionStart(const EditorState &state) { return std::min(state.selection_start, state.selection_end); }

int EditorSelection::getSelectionEnd(const EditorState &state) { return std::max(state.selection_start, state.selection_end); }

void EditorSelection::selectAllText(EditorState &state, const std::string &text)
{
    const size_t MAX_SELECTION_SIZE = 100000; // Limit for very large files
    state.selection_active = true;
    state.selection_start = 0;
    state.cursor_column = std::min(text.size(), MAX_SELECTION_SIZE);
    state.selection_end = state.cursor_column;
    state.full_text_selected = true;
}

bool EditorSelection::hasSelection(const EditorState &state) { return state.selection_active && (state.selection_start != state.selection_end); }

int EditorSelection::getSelectionLength(const EditorState &state) { return getSelectionEnd(state) - getSelectionStart(state); }