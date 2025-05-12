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

	// Special selection operations
	void selectAllText(const std::string &text);
};

// Global instance
extern EditorSelection gEditorSelection;