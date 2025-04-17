#pragma once
#include "editor_cursor.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorKeyboard
{
  public:
	EditorKeyboard();
	~EditorKeyboard() = default;

	void handleCharacterInput();
	void handleEnterKey();
	void handleDeleteKey();
	void handleBackspaceKey();
	void handleTextInput();

	void processFontSizeAdjustment();
	void processSelectAll();

	void processTextEditorInput();

	void handleEditorKeyboardInput();

	void handleArrowKeyVisibility();

	void updateCursorVisibilityOnTextChange();

	void processUndoRedo();
};

// Global instance
extern EditorKeyboard gEditorKeyboard;