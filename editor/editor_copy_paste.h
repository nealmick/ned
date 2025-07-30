#pragma once
#include "editor_scroll.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorCopyPaste
{
  public:
	EditorCopyPaste() = default;
	~EditorCopyPaste() = default;

	// Copy operation
	void copySelectedText(const std::string &text);

	// Cut operations
	void cutSelectedText();
	void cutWholeLine();

	// Paste operation
	void pasteText();
	void processClipboardShortcuts();

  private:
	// Helper methods
	// Get selection bounds (helper methods)
	int getSelectionStart() const;
	int getSelectionEnd() const;

	// Indentation handling methods
	// Returns true for tabs, false for spaces
	bool checkIndentationType() const;
	// Convert 4 spaces to 1 tab
	std::string convertSpacesToTabs(const std::string &text) const;
	// Convert 1 tab to 4 spaces
	std::string convertTabsToSpaces(const std::string &text) const;
};

// Global instance
extern EditorCopyPaste gEditorCopyPaste;
