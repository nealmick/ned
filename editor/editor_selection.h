/*
    File: editor_selection.h
    Description: Handles text selection operations in the editor.
*/

#pragma once
#include "editor_types.h"
#include <string>

// Forward declarations
class EditorSelection;
extern EditorSelection gEditorSelection;

class EditorSelection
{
  public:
    EditorSelection() = default;
    ~EditorSelection() = default;

    // Core selection operations
    void startSelection(EditorState &state);
    void updateSelection(EditorState &state);
    void endSelection(EditorState &state);

    // Selection utilities
    int getSelectionStart(const EditorState &state);
    int getSelectionEnd(const EditorState &state);

    // Special selection operations
    void selectAllText(EditorState &state, const std::string &text);

    // Selection status checks
    bool hasSelection(const EditorState &state);
    int getSelectionLength(const EditorState &state);
};

// Global instance
extern EditorSelection gEditorSelection;