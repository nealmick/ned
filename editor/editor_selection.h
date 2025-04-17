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
	void startSelection();
	void updateSelection();
	void endSelection();

	// Selection utilities
	int getSelectionStart();
	int getSelectionEnd();

	// Special selection operations
	void selectAllText(const std::string &text);

	// Selection status checks
	bool hasSelection();
	int getSelectionLength();
};

// Global instance
extern EditorSelection gEditorSelection;