/*
	File: editor_render.h
	Description: Handles rendering for the editor, separating rendering logic
   from the main editor class.
*/

#pragma once
#include "editor_types.h"
#include "imgui.h"

#include <string>
#include <vector>

// Forward declarations
class EditorRender;
extern EditorRender gEditorRender;

class EditorRender
{
  public:
	void renderEditorFrame();
	void renderEditorContent();
	void renderText();
	void renderWhitespaceGuides();
	void renderCurrentLineHighlight();
	bool validateAndResizeColors();
	void setupEditorWindow(const char *label);
	void beginTextEditorChild(const char *label,
							  float remaining_width,
							  float content_width,
							  float content_height);

  private:
	void renderLineBackground(int line_num,
							  int start_visible_line,
							  int end_visible_line,
							  size_t cursor_line,
							  const ImVec2 &line_start_draw_pos);
	void renderCharacterAndSelection(size_t char_index,
									 int selection_start,
									 int selection_end,
									 ImVec2 &current_draw_pos);
	bool skipLineIfAboveVisible(size_t &char_index,
								int line_num,
								int start_visible_line,
								ImVec2 &current_draw_pos);
};