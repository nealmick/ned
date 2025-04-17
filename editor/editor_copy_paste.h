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
};

// Global instance
extern EditorCopyPaste gEditorCopyPaste;
