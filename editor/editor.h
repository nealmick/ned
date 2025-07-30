/*
	File: editor.h
	Description: Main editor coordinator that manages the text editing
   interface.
*/
#pragma once
#include "imgui.h"

#include "editor_copy_paste.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_indentation.h"
#include "editor_keyboard.h"
#include "editor_line_numbers.h"
#include "editor_mouse.h"
#include "editor_render.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;
extern EditorState editor_state;

class Editor
{
  public:
	void textEditor();

	void setupEditorDisplay();

	void processEditorInput();

	void updateLineStarts();

	int getLineFromPos(int pos);

	float calculateTextWidth();

	void renderEditor(ImFont *font, float editorWidth);
};
