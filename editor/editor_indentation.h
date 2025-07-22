#pragma once
#include "editor_scroll.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorIndentation
{
  public:
	EditorIndentation() = default;
	~EditorIndentation() = default;

	// Main API functions
	void handleTabKey();
	void removeIndentation();
	bool processIndentRemoval();

  private:
	// Selection helper methods
	int getSelectionStart();
	int getSelectionEnd();

	// Tab indentation helpers
	void handleMultiLineIndentation();
	void handleSingleLineIndentation();
	void finishIndentationChange();

	// Line finding helpers
	int findLineStart(int position);
	int findLineEnd(int position);

	// Indentation removal helpers
	void processLineIndentRemoval(std::string &newText,
								  size_t lineStart,
								  size_t lineEnd,
								  size_t lastLineEnd,
								  int &totalSpacesRemoved);
	void updateStateAfterIndentRemoval(std::string &newText,
									   int firstLineStart,
									   int lastLineEnd,
									   int totalSpacesRemoved);
	void updateColorsAfterIndentRemoval(int firstLineStart,
										int lastLineEnd,
										int totalSpacesRemoved);
};

// Global instance
extern EditorIndentation gEditorIndentation;